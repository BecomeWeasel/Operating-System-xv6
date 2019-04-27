#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"


void sys_setpriority(int pid,int priority){
#ifdef MLFQ_SCHED
	acquire(&ptalbe.lock);
	struct proc* p;
	struct proc* targetP;
	for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
		if(p->pid==pid){
			targetP=p;
			break;
		}
	}
	release(&ptable.lock);
	targetP->priority=priority;
#endif
}