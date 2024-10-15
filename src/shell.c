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

int execute(struct pipes *p)
{
	return execvp(p->args[0], p->args);
}

int spawn_proc(int in, int out, struct cmd *cmd, struct pipes *p)
{
  	pid_t pid;
  	int status, fd;
  	if ((pid = fork()) == 0) {
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
    	if (execute(p) == -1)
       		perror("lsh");
    	exit(EXIT_FAILURE);
    } 
	else {
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

		// ===================================
		// Requirement 2.1
		struct cmd *cmd = split_line(buffer);
		// ===================================
		int status = -1;

		// Determine whether it is a built-in command
		status = searchBuiltInCommand(cmd);
		if (status != -1){
			int fd, in = dup(0), out = dup(1);
			if (cmd->in_file) {
		        fd = open(cmd->in_file, O_RDONLY);
		        dup2(fd, 0);
		        close(fd);
		    }
			if (cmd->out_file) {
		        fd = open(cmd->out_file, O_RDWR | O_CREAT, 0644);
		        dup2(fd, 1);
		        close(fd);
			}
			execBuiltInCommand(status,cmd);
			if (cmd->in_file)  dup2(in, 0);
			if (cmd->out_file) dup2(out, 1);
			close(in);
			close(out);
		}
		else{
			//external command
			status = fork_pipes(cmd);
		}

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
