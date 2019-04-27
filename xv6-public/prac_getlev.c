#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"

int sys_getlev(void){
#ifdef MLFQ_SCHED
	return getlev();
#endif
	return 0;
}