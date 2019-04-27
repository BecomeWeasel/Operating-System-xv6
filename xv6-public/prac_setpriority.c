#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"


void sys_setpriority(int pid,int priority){
#ifdef MLFQ_SCHED
	argint(0,&pid);
	argint(1,&priority);
	setprocpriority(pid,priority);
#endif
}