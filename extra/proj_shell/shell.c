#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#define BUF_SIZE 1000
#define LIMIT_NUM_CMD 50
#define LIMIT_NUM_CMD_OPTION 10
#define ERR_CODE -1
#define CMD_TOKEN ";"
#define CMD_OPTION_TOKEN " "


int main(int argc, char *argv[])
{


	if (argc == 1)
	{
		while (1)
		{
			int num_of_cmd = 0;
			int k = 0;
			char *all_cmd_sets[LIMIT_NUM_CMD][LIMIT_NUM_CMD_OPTION] = {};
			printf("> ");
			char raw_input[BUF_SIZE];


			// ctrl-D check
			if (fgets(raw_input, BUF_SIZE, stdin) == NULL)
				_exit(0);

			// if user type no command and press enter
			if (!strcmp(raw_input, "\n")) continue;

			printf("Your current command is %s", raw_input);
			printf("Working...\n");


			// replace new line into null
			char *newline_pointer = strchr(raw_input, '\n');
			if (newline_pointer != NULL) *newline_pointer = '\0';


			char *temp_for_tokenized_cmd;
			temp_for_tokenized_cmd = strtok(raw_input, CMD_TOKEN);

			//ok

			char *cmd_sets[LIMIT_NUM_CMD] = {};

			int i = 0;

			while (temp_for_tokenized_cmd != NULL)
			{
				cmd_sets[i] = temp_for_tokenized_cmd;
				temp_for_tokenized_cmd = strtok(NULL, CMD_TOKEN);
				i++;
//				if (temp_for_tokenized_cmd != NULL)num_of_cmd++;
			}


			char *temp_for_separated_cmd;
			for (k = 0; k < LIMIT_NUM_CMD && cmd_sets[k] != NULL; k++)
			{
				int ii = 0;

				char *cmd_with_options[LIMIT_NUM_CMD_OPTION] = {};

				temp_for_separated_cmd = strtok(cmd_sets[k], CMD_OPTION_TOKEN);
				while (temp_for_separated_cmd != NULL)
				{
					cmd_with_options[ii++] = temp_for_separated_cmd;
					temp_for_separated_cmd = strtok(NULL, CMD_OPTION_TOKEN);
				}


				if (cmd_with_options[0] != NULL && !strcmp(cmd_with_options[0], "quit"))
				{
					_exit(0);
				}


				// limit num of cmd options to 8,
				// 0 for cmd ,1~8 for options,9 for null
				for (int j = 0;
				     j < LIMIT_NUM_CMD_OPTION - 1
				     && cmd_with_options[j] != NULL; j++)
				{
					all_cmd_sets[k][j] = cmd_with_options[j];
				}

			}



			//execvp section
			pid_t child_pid, wait_pid;
			int status_child = 0;
			for (int id = 0; id < k; id++)
			{
				child_pid = fork();
				if (child_pid == 0)
				{
					//child section
					int execvp_return = execvp(all_cmd_sets[id][0], all_cmd_sets[id]);
					if (execvp_return == ERR_CODE)
					{
						switch (errno)
						{
							case ECHILD :
								perror("Unknown Command\n");
								break;
							case ENOENT :
								perror("Unknown Command please try valid command\n");
								break;
							default :
								printf("errno:%d\n", errno);
								break;
						}
						_exit(errno);
					}


				}
				else if (child_pid < 0)
				{
					fprintf(stderr, "Fork failed");
					exit(-1);
				}
			}

			int wait_cnt = 0;
			while ((wait_pid = wait(&status_child)) > 0
			       && ++wait_cnt < k);

			switch (status_child)
			{
				case ECHILD :
					printf("Unknown Command\n");
					break;
				case ENOENT :
					printf("Unknown Command\n");
					break;
				default :
//					printf("errno:%d\n", errno);
					break;
			}
		}


	}
	else if (argc == 2)
	{
		// batch mode
		FILE *batch_file = fopen(argv[1], "r");


		if (batch_file == NULL)
		{
			exit(0);
		}
		else
		{
			int num_of_cmd = 0;

			while (!feof(batch_file))
			{
				char batch_line[BUF_SIZE] = {};
				if (fgets(batch_line, BUF_SIZE, batch_file) == NULL)
				{
					exit(0);
				}

				if (!strcmp(batch_line, "\n")) continue;


				printf("Your current command is %s", batch_line);
				printf("Working...\n");

//				 replace \n into null char
				char *newline_pointer = strchr(batch_line, '\n');
				if (newline_pointer != NULL) *newline_pointer = '\0';

				int num_of_cmd_in_oneline = 0;


				// semicolon tokenize
				char *temp_for_tokenized_cmd;
				temp_for_tokenized_cmd = strtok(batch_line, CMD_TOKEN);

				char *cmd_sets[LIMIT_NUM_CMD] = {};

				while (temp_for_tokenized_cmd != NULL)
				{
					cmd_sets[num_of_cmd_in_oneline++] = temp_for_tokenized_cmd;
					temp_for_tokenized_cmd = strtok(NULL, CMD_TOKEN);
				}

				char *temp_for_separated_cmd;
				char *cmd_with_options[LIMIT_NUM_CMD][LIMIT_NUM_CMD_OPTION] = {};
				for (int p = 0; p < num_of_cmd_in_oneline; p++)
				{
					temp_for_separated_cmd = strtok(cmd_sets[p], CMD_OPTION_TOKEN);
					for (int temp_target = 0;
					     temp_target < LIMIT_NUM_CMD_OPTION - 1
					     && temp_for_separated_cmd != NULL; temp_target++)
					{
						cmd_with_options[p][temp_target] = temp_for_separated_cmd;
						temp_for_separated_cmd = strtok(NULL, CMD_OPTION_TOKEN);
					}
				}


				pid_t child_pid, wait_pid;
				int status_child = 0;

				for (int fork_count = 0; fork_count < num_of_cmd_in_oneline; fork_count++)
				{
					if (!strcmp(cmd_with_options[fork_count][0], "quit"))
					{
						//quit command
						printf("quit command encountered\n");
						_exit(0);
					}
					child_pid = fork();
					if (child_pid == 0)
					{
						// child section

						int execvp_return = execvp(cmd_with_options[fork_count][0],
						                           cmd_with_options[fork_count]);
						if (execvp_return == ERR_CODE)
						{
							switch (errno)
							{
								case ECHILD :
									perror("Unknown Command\n");
									break;
								case ENOENT :
									perror("Unknown Command please try valid command\n");
									break;
								default :
									printf("errno:%d\n", errno);
									break;
							}
							_exit(errno);
						}

					}
					else if (child_pid < 0)
					{
						fprintf(stderr, "Fork failed");
						exit(-1);
					}
				}

				int wait_cnt = 0; // count how many times wait function called


				while ((wait_pid = wait(&status_child)) > 0
				       && ++wait_cnt < num_of_cmd_in_oneline)
					switch (errno)
					{
						case ECHILD :
							printf("Unknown Command\n");
							break;
						case ENOENT :
							printf("Unknown Command\n");
							break;
						default :
//							printf("errno:%d\n", errno);
							break;
					}
			}
			fclose(batch_file);

		}


	}
	else if (argc >= 3)
	{
		printf("only single batch file accepted\n");
		return 0;
	}
}

