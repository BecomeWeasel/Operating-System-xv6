#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"

void sys_monopolize(int password){
#ifdef MLFQ_SCHED
	argint(0,&password);
	monopolize(password);
#endif
	return;
}