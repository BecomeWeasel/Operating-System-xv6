#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 2097152
#define LIMIT_NUM_CMD 50
#define LIMIT_NUM_CMD_OPTION 10
#define CMD_TOKEN ";"
#define CMD_OPTION_TOKEN " "


int main(int argc, char *argv[]) {
	if (argc == 1) {
		while (1) {
      char *all_cmd_sets[LIMIT_NUM_CMD][LIMIT_NUM_CMD_OPTION]={};
			printf("> ");
			char raw_input[BUF_SIZE];


			// ctrl-D check
			if (fgets(raw_input, BUF_SIZE, stdin) == NULL)
				_exit(0);

			char *temp_for_tokenized_cmd;
			temp_for_tokenized_cmd = strtok(raw_input, CMD_TOKEN);

			//ok

			char *cmd_sets[LIMIT_NUM_CMD] = {};

			int i = 0;

			while (temp_for_tokenized_cmd != NULL) {
				cmd_sets[i] = temp_for_tokenized_cmd;
				temp_for_tokenized_cmd = strtok(NULL, CMD_TOKEN);
				i++;
			}


			char *temp_for_separted_cmd;
			for (int k = 0; k < LIMIT_NUM_CMD && cmd_sets[k] != NULL; k++) {
				int ii = 0;

				// replace new line into null
				char *newline_pointer = strchr(cmd_sets[k], '\n');
				if (newline_pointer != NULL) *newline_pointer = '\0';


				char *cmd_with_options[LIMIT_NUM_CMD_OPTION] = {};

				temp_for_separted_cmd = strtok(cmd_sets[k], CMD_OPTION_TOKEN);
				while (temp_for_separted_cmd != NULL) {
					cmd_with_options[ii++] = temp_for_separted_cmd;
					temp_for_separted_cmd = strtok(NULL, CMD_OPTION_TOKEN);
				}
				//execvp section
				pid_t child_pid;
				int child_status;

				if (cmd_with_options[0] != NULL && !strcmp(cmd_with_options[0], "quit")) {
					_exit(0);
				}

        for(int i=0;i<LIMIT_NUM_CMD;i++){
          for(int j=0;j<LIMIT_NUM_CMD_OPTION&&
              cmd_with_options[j]!=NULL;j++)
          {
            all_cmd_sets[i][j]=cmd_with_options[j];
          }
        }


				/*child_pid = fork();
				if (child_pid == 0) {
					// child process section
					// if command is quit, end this child process.

					execvp(cmd_with_options[0], cmd_with_options);

					perror("Unknown Command\n");
				} else {
					while (wait(&child_status) < 0);

					printf("Finished\n");
				}*/
			}
		}




	} else {
		// batch mode
	}
}

