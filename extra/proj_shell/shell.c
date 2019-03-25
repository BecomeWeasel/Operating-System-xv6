#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 2097152
#define NUM_CMD 50
#define LEN_CMD 100
#define CMD_TOKEN ";"
#define CMD_OPTION_TOKEN " "


int main(int argc, char *argv[]) {
	if (argc == 1) {
		while (1) {
			printf("> ");
			char statement[BUF_SIZE];


      // ctrl-D check
			if(fgets(statement, BUF_SIZE, stdin)==NULL)
        return 0;
      // if command is quit, end shell
      if(strcmp(statement,"quit")){
        // end shell
        return 0;
      }

			char *temp;
			temp = strtok(statement, CMD_TOKEN);

			//ok

			char *cmd_sets[50] = {};

			int i = 0;

			while (temp != NULL) {
				cmd_sets[i] = temp;
				temp = strtok(NULL, CMD_TOKEN);
				i++;
			}


			char *temp3;
			for (int k = 0; k < 50 && cmd_sets[k] != NULL; k++) {
				int ii = 0;

				// replace new line into null
				char *p = strchr(cmd_sets[k], '\n');
				if (p != NULL) *p = '\0';


				char *cmd_options[6] = {};

				temp3 = strtok(cmd_sets[k], CMD_OPTION_TOKEN);
				while (temp3 != NULL) {
					cmd_options[ii++] = temp3;
					temp3 = strtok(NULL, CMD_OPTION_TOKEN);
//					*p = "";
				}
				//execvp section
				pid_t child_pid;
				int child_status;

				child_pid = fork();
				if (child_pid == 0) {
					execvp(cmd_options[0], cmd_options);

					perror("Unknown Command\n");
				} else {
					while (wait(&child_status) < 0);

					printf("Finished\n");
				}
			}

		}
	} else {
		// batch mode
	}
}

