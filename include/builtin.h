#ifndef BUILTIN_H
#define BUILTIN_H
#include "../include/command.h"


int searchBuiltInCommand(struct cmd *cmd);
int execBuiltInCommand(int status,struct pipes *cmd);

int help(char **args);
int cd(char **args);
int echo(char **args);
int exit_shell(char **args);
int record(char **args);

extern const char *builtin_str[];

extern const int (*builtin_func[]) (char **);

extern int num_builtins();

#endif
