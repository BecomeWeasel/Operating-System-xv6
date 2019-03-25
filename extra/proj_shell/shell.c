#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define BUF_SIZE 2097152
#define NUM_CMD 50
#define LEN_CMD 100
#define CMD_TOKEN ";"
#define CMD_OPTION_TOKEN " "

int count_char(char*,char);

int main(int argc,char * argv[]){
  if(argc ==1)
  {
    while(1){
      printf("> ");
      char statement[100];
      fgets(statement,100,stdin);
      

      const char s[2]=";";

      char *temp;
      temp=strtok(statement,CMD_TOKEN);
      printf("%s l28\n",temp);

      //ok

      char cmd_sets[50][50]={};
      printf("%s\n",cmd_sets[0]);

      int i=0;
      printf("here l42\n");
      while(temp!=NULL){
        strcpy(cmd_sets[i],temp);
        temp=strtok(NULL,CMD_TOKEN);
        i++;
      }

      int j=0;
      int k=1;
      int num_cmd=0;
      char *cmd_sets_options[50][100]={};


      while(j<NUM_CMD&&cmd_sets_options[j]!=NULL){
        char *temp2=strtok(cmd_sets[j],CMD_OPTION_TOKEN);
        strcpy(cmd_sets_options[j][0],temp2);
       while(temp2!=NULL){
         temp2=strtok(NULL,CMD_OPTION_TOKEN);
         strcpy(cmd_sets_options[j][k++],temp2);
         printf("%s\n",cmd_sets_options[j][k]);
       }
       num_cmd++;
       cmd_sets_options[j++][49]=NULL;
      }

      int p=0;
      for(;p<num_cmd;p++){
      pid_t child_pid;
      int child_status;

      child_pid=fork();
      if(child_pid==0){
        // child process doing work
        execvp(cmd_sets_options[p][0],cmd_sets_options[p]);

        perror("Unknown Command\n");
      }
      else
      {
        while(wait(&child_status)<0);
      }

      }

    }
  }
  else
  {
    // batch mode
  }
}

int count_char(char* line,char target){
  int i=0;
  int count=0;
  while(line[i]!='\0'){
    if(line[i++]==target)
      count++;
  }
  return count;
}
