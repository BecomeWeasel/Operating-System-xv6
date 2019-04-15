#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char * argv[]){
  int current_id=getpid();
  int parent_id=getppid();

  printf(1,"My pid is %d\n",current_id);
  printf(1,"My ppid is %d\n",parent_id);
  exit();
}
