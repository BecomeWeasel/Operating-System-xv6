#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"


void sys_setpriority(int pid,int priority){
//#ifdef MLFQ_SCHED
	struct proc* p;
	p=getprocbyid(pid);
	p->priority=priority;
//#endif
}