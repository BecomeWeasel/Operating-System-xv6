#include "types.h"
#include "param.h"
#include "defs.h"
#include "mmu.h"
#include "proc.h"


// ppid system call
int getppid(void){
//  cprintf("%s\n",str);
  return myproc()->parent->pid;
}

// wrapper
int sys_getppid(void){
  char *str;
  if(argstr(0,&str)<0)
    return -1;
  return getppid();
  
}
