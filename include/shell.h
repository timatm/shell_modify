#ifndef SHELL_H
#define SHELL_H

#include "command.h"

int execute(struct pipes *);
int spawn_proc(int, int, struct cmd *, struct pipes *);
int fork_pipes(struct cmd *cmd);
void execute_pipe(struct cmd *cmd);
void shell();

#endif
