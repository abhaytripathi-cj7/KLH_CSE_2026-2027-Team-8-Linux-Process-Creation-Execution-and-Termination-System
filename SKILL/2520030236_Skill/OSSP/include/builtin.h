#ifndef OSSP_BUILTIN_H
#define OSSP_BUILTIN_H
#include "shell.h"
int is_builtin(const char *name);
int run_builtin(Shell *sh, Command *cmd);
#endif
