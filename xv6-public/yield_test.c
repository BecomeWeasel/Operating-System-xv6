#include "types.h"
#include "stat.h"
#include "user.h"
#include "mmu.h"
#include "x86.h"
#include "param.h"
#include "spinlock.h"
#include "memlayout.h"
#include "proc.h"

int main(int argc,char*argv[]){
  int pid;
  pid=fork();
  for(int i=0;i<10;i++){
   if(pid==0){
    printf(1,"Child\n");
    yield();
   }
   else{
     printf(1,"Parent\n");
     yield();
   }

  }
  return 0;
}
