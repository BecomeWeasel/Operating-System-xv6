#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"


void sys_setpriority(int pid,int priority){
//#ifdef MLFQ_SCHED
	int result=setprocpriority(pid,priority);
	if(result)
		cprintf("%d 's priority set to %d",pid,priority);
//#endif
}