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

void execute_pipe(struct cmd *cmd) {
	// "in" is the input of the current command
	// "fd" is the file descriptor used to create the pipe
    int in = 0, fd[2];  
    struct pipes *currentCmd = cmd->head;
    struct pipes *nextCmd = currentCmd->next;

    while (currentCmd != NULL) {
        // If there is a next command, create the pipe
        if (nextCmd != NULL) {
            if (pipe(fd) == -1) {
                perror("Pipe creation failed");
                exit(EXIT_FAILURE);
            }
        }

        // Child process
        if (fork() == 0) {
            // Input and output redirection using redirection()
            redirection(in, (nextCmd != NULL) ? fd[1] : 1, cmd);

            // execute command
            execvp(currentCmd->args[0], currentCmd->args);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }

        // 父進程：等待子進程完成，並管理管道
        if (in != 0) {
            close(in);  // 父進程關閉已經重定向過的讀端
        }
        if (nextCmd != NULL) {
            close(fd[1]);  // 父進程關閉寫端，下一個命令將使用讀端
        }

        in = fd[0];  // 下一個命令的輸入變成當前命令的輸出

        currentCmd = nextCmd;
        if (nextCmd != NULL) {
            nextCmd = nextCmd->next;
        }
    }

    // // 等待所有子進程結束
    // for (int i = 0; i < cmd->count; i++) {
    //     wait(NULL);
    // }
}

/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * @param in Stdin redirected object
 * @param out Stdout redirected object
 * @param cmd Command struct
 */
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


/**
 * @brief Execute external command
 * 
 * @param p Pipe format
 * @return int 
 * Return execution status
 */
int execute(struct pipes *p)
{
	return execvp(p->args[0], p->args);
}

/**
 * @brief Execute external command
 * 
 * @note 
 * The external command is mainly divided into the following two steps:
 * 1. Create child process
 * 2. Call execute to " execute() " the corresponding executable file
 * @param in Stdin redirected object
 * @param out Stdout redirected object
 * @param cmd Command struct
 * @param p Pipe struct
 * @return int 
 * Return execution status
 */
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

/**
 * @brief Call "spawn_proc()" in order according to the number of pipes
 * 
 * @param cmd 
 * @param currentCmd 
 * @return int 
 */
int fork_pipes(struct cmd *cmd)
{
  	int in = 0, fd[2];
	struct pipes *temp = cmd->head;
  	while (temp->next != NULL) {
      	pipe(fd);
      	spawn_proc(in, fd[1], cmd, temp);
      	close(fd[1]);
      	in = fd[0];
      	temp = temp->next;
  	}
  	if (in != 0) {
    	spawn_proc(in, 1, cmd, temp);
    	return 1;
  	}

  	spawn_proc(0, 1, cmd, cmd->head);
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
		// Only a single command
		if(cmd->head->next == NULL){
			status = searchBuiltInCommand(cmd);
			printf("status  : %d\n",status);
			if (status != -1){
				int fd, in = dup(stdin), out = dup(stdout);
				printf("in  : %d\n",in);
				printf("out : %d\n",out);
				// redirection(in,out,temp);
				status = execBuiltInCommand(status,cmd->head);

				// recover shell stdin and stdout
				if (cmd->in_file)  dup2(in, 0);
				if (cmd->out_file) dup2(out, 1);
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(0, 1, cmd, cmd->head);
			}
			test_pipe_struct(cmd->head);
		}
		//If use pipe
		else{
			status = fork_pipes(cmd);
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
