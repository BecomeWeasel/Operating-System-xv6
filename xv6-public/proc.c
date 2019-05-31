#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

#define NULL 0

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  //cprintf("81\n");
  acquire2(&ptable.lock,81);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->ctime=ticks;
  p->pid = nextpid++;

  p->isThread=0;
  p->numOfThread=0;
  p->nextThreadId=0;
  p->tid=-1;

#ifdef MLFQ_SCHED
  p->lev=0;
  p->priority=0;
  p->monopolize=0;
#endif
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }

  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  p->ctime=ticks;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  //cprintf("165\n");
  acquire2(&ptable.lock,166);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  //cprintf("232\n");
  acquire2(&ptable.lock,233);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  //cprintf("268\n");
  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  //cprintf("298\n");
  acquire2(&ptable.lock,299);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    //cprintf("353\n");
    acquire(&ptable.lock);

#ifdef FCFS_SCHED
    struct proc *p;
		for(p=ptable.proc;p<&ptable.proc[NPROC];p++){

      if(p->state!=RUNNABLE) {
				continue;
			}
			else{ //CPU를 받을 상태가 되었을때
      p->stime=ticks;
		  c->proc=p; // 작업 변경
		  switchuvm(p);
		  p->state=RUNNING;

		  swtch(&(c->scheduler),p->context);
		  switchkvm();

		  c->proc=0;
		  break;
		}
  }


#elif DEFAULT
    struct proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

      if(p->state != RUNNABLE) // RUNNABLE==ready 상태인것,자원할당 가능한것 찾는것임
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p; // 현재 CPU 가져옴 그후 교체함
      switchuvm(p);
      p->state = RUNNING;


      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
#elif MLFQ_SCHED
    struct proc* priorityP=NULL;
	  struct proc* p;
	  int L0count=0;
	  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
	    if(p->lev==0&&p->state==RUNNABLE){
	      L0count=L0count+1;
	      break;
	    }
	  }
	  if(L0count>0){ // L0에서 탐색 후 RR수행
	    for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
	      if(p->state!=RUNNABLE)
	        continue;
	      if(p->lev==0){

		      c->proc=p; // 작업 변경
		      switchuvm(p);
		      p->state=RUNNING;
		      p->stime=ticks;
	        p->rtime=0;

		      swtch(&(c->scheduler),p->context);
		      switchkvm();

		      c->proc=0;

	      }
	    }
	  }
	  else{ // L0이 비어있으니 , L1에서 priority 낮은 거 탐색


	  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
	    if(p->state!=RUNNABLE)
	      continue;
	    if(p->lev==1){
	      if(priorityP!=NULL){
	        if(priorityP->priority<p->priority)
	          priorityP=p;
	        else if(priorityP->priority==p->priority){
	          if(priorityP>p)
	            priorityP=p;
	        }
	      }
	      else
	        priorityP=p;
	    }
	  }

	  if(priorityP!=NULL){
	    p=priorityP;
	    c->proc=p;
	    switchuvm(p);
	    p->state=RUNNING;

	    p->stime=ticks;
	    p->rtime=0;

	    swtch(&(c->scheduler),p->context);
	    switchkvm();

	    c->proc=0;
    }
	}
#else
#endif


    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  //cprintf("504\n");
  acquire2(&ptable.lock,505);  //DOC: yieldlock
  myproc()->state = RUNNABLE; // todo ptable rb값을 조절

  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    //cprintf("553\n");
    acquire2(&ptable.lock,553);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire2(lk,569);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
 // cprintf("591\n");
  acquire2(&ptable.lock,592);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  //cprintf("605\n");
  acquire2(&ptable.lock,606);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// pid에 매칭되는 process return

int setprocpriority(int pid,int priority){
  acquire(&ptable.lock);
  struct proc* p;
  struct proc* targetP=NULL;
  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
    if(p->pid==pid){
      targetP=p;
      break;
    }
  }
#ifdef MLFQ_SCHED
  if(targetP==NULL){
  release(&ptable.lock);
  return 0;
  }
  targetP->priority=priority;
//  cprinf("paramter is %d and targetP : %d",priority,targetP->priority);
  release(&ptable.lock);
  return 1;
#endif
  release(&ptable.lock);
  if(targetP==NULL)
    return 0;
  return 0;
}

#ifdef MLFQ_SCHED
int getlev(void){
  if(myproc()->monopolize==1){
    return 0;}
  return myproc()->lev;
}

void priboosting(void){
  acquire(&ptable.lock);
  struct proc* p;

  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
    if(p->lev==1){
      p->lev=0;
      p->priority=0;
    }
  }
  release(&ptable.lock);
}

void monopolize(int password){
  acquire(&ptable.lock);

  if(myproc()->monopolize==1){// 현재 독점중일때
    if(password==2016026599){ // 독점중인데 비밀번호 맞을때
      myproc()->monopolize=0; // 독점해제
      myproc()->lev=0; // L0로 이동
      myproc()->priority=0; // PRI 조절
    }
    else{ //독점중인데 비밀번호 틀릴때
      cprintf("wrong password at calling monopolize\n");
      myproc()->monopolize=0; // 독점해제
      myproc()->killed=1;
      if(myproc()->state==SLEEPING) // kill함
        myproc()->state=RUNNABLE;
    }
  }
  else if(myproc()->monopolize==0){// 독점중이 아닐때
    if(password==2016026599){ // 독점중이 아닐때 비밀번호 맞을때
      myproc()->monopolize=1; // 독점시작
    }
    else{ // 독점중이 아닐때 비밀번호 틀릴때
      // kill the P
      cprintf("wrong password at calling monopolize\n");
      myproc()->killed=1;
      if(myproc()->state==SLEEPING) // 비정상접근,kill P
        myproc()->state=RUNNABLE;
    }
  }
    release(&ptable.lock);

}

#endif

static struct proc*
allocthread(void){
  struct proc *p;
  struct proc* curproc=myproc();
  char *sp;

  cprintf("747\n");
  acquire2(&ptable.lock,748);

  for(p=ptable.proc;p<&ptable.proc[NPROC];p++)
    if(p->state == UNUSED)
      goto found;
  
  release(&ptable.lock);
  return 0;

found:
  p->state=EMBRYO;
  p->ctime=ticks;
  p->pid=curproc->pid;

  p->isThread=1;
  p->numOfThread=0;

  curproc->numOfThread++;
  p->tid=curproc->nextThreadId++;
  
  release(&ptable.lock);
 
  if((p->kstack=kalloc())==0){
    p->state=UNUSED;
    return 0;
  }
  
  sp=p->kstack+KSTACKSIZE;

  sp-= sizeof *p->tf;
  p->tf=(struct trapframe*)sp;
  
  sp-=4;
  *(uint*)sp=(uint)trapret;
  
  sp-=sizeof *p->context;
  p->context=(struct context*)sp;
  memset(p->context,0,sizeof *p->context);
  p->context->eip=(uint)forkret;

  return p;
}

int thread_create(thread_t*thread, void*(*start_routine)(void*),void *arg){
  int i;
  uint tmpsz,sp,ustack[2];
  struct proc* np;
  struct proc* curproc=myproc();
  
  cprintf("at create %d\n",thread);
  // same as fork
  if((np=allocthread())==0){
    return -1;
  }

  np->pgdir=curproc->pgdir;
  
  tmpsz=curproc->sz;


  
 if((tmpsz=allocuvm(np->pgdir,tmpsz,tmpsz+2*PGSIZE))==0)
   goto bad;
 clearpteu(np->pgdir,(char*)(tmpsz-2*PGSIZE));
 sp=tmpsz;


 //argc=1;

 //ustack[]=0;

 ustack[0]=0xffffffff; // fake return PC
 ustack[1]=(uint)arg;// argv pointer

 sp-=(2)*4;

 cprintf("before \n");

 if(copyout(np->pgdir,sp,ustack,(2)*4)<0)
   goto bad;

 cprintf("after \n");

 np->sz=tmpsz;

 //cprintf("833\n");
 acquire2(&ptable.lock,834);

 struct proc* p;
 for(p=ptable.proc;p<&ptable.proc[NPROC];p++)
   if(p->pid==np->pid){
     p->sz=np->sz;
     p->pgdir=np->pgdir;
   }
 release(&ptable.lock);

 *np->tf=*curproc->tf;
 np->tf->eip=(uint)start_routine; // starting point
 cprintf("eip : %d\n",np->tf->eip);
 np->tf->esp=sp;
 np->tf->eax=0;
  *thread=np->tid;
  cprintf("%d\n",*thread);

  np->parent=curproc->parent;


  for(i=0;i<NOFILE;i++)
    if(curproc->ofile[i])
      np->ofile[i]=filedup(curproc->ofile[i]);
  np->cwd=idup(curproc->cwd);

  safestrcpy(np->name,curproc->name,sizeof(curproc->name));

  //cprintf("862\n");
  acquire2(&ptable.lock,863);

  np->state=RUNNABLE;

  release(&ptable.lock);

  switchuvm(np);

  cprintf("out\n");
  return 0;


bad:
  return -1;
}

void thread_exit(void *retval){
  
}

