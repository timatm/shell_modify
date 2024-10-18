#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"


void redirection(int in ,int out ,struct cmd *cmd){
	int fd;
	printf("in : %d \n",in);
	printf("out: %d \n",out);
	test_cmd_struct(cmd);
	if (in != 0) {
          	dup2(in, 0);
          	close(in);
	} else {
		if (cmd->in_file) {
			fd = open(cmd->in_file, O_RDONLY);
			dup2(fd, 0);
			close(fd);
		}
	}
	if (out != 1) {
		dup2(out, 1);
		close(out);
	} else {
		if (cmd->out_file) {
			fd = open(cmd->out_file, O_RDWR | O_CREAT, 0644);
			dup2(fd, 1);
			close(fd);
		}
	}
}
int execute(struct pipes *p)
{
	return execvp(p->args[0], p->args);
}

int spawn_proc(int in, int out, struct cmd *cmd, struct pipes *p)
{
  	pid_t pid;
  	int status;
  	if ((pid = fork()) == 0) { //child process
		redirection(in,out,cmd);
    	if (execute(p) == -1)
       		perror("lsh");
    	exit(EXIT_FAILURE);
    } 
	else { //parent process
		waitpid(pid, &status, WUNTRACED);
		while (!WIFEXITED(status) && !WIFSIGNALED(status));
  	}
  	return 1;
}

int fork_pipes(struct cmd *cmd)
{
  	int in = 0, fd[2];
	struct pipes *temp = cmd->head;
	// ===================================
	// Requirement 2.3
  	spawn_proc(0, 1, cmd, cmd->head);
	// ===================================
	return 1;
}
void shell()
{
	while (1) {
		printf(">>> $ ");
		
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);

		int status = -1;

		// Determine whether it is a built-in command
		// if command is built-in command return function number
		// if command is external command return -1 
		struct pipes *temp = cmd->head;
		while(temp != NULL){
			status = searchBuiltInCommand(cmd);
			printf("status  : %d\n",status);
			if (status != -1){
				int fd, in = dup(stdin), out = dup(stdout);
				printf("in  : %d\n",in);
				printf("out : %d\n",out);
				// redirection(in,out,temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (cmd->in_file)  dup2(in, 0);
				if (cmd->out_file) dup2(out, 1);
				close(in);
				close(out);
			}
			else{
				//external command
				status = fork_pipes(temp);
			}
			test_pipe_struct(temp);
			temp = temp->next;
		}
		// free space
		while (cmd->head) {
			struct pipes *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
