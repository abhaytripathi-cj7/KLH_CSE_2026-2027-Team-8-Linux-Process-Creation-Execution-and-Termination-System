#ifndef OSSP_EXECUTOR_H
#define OSSP_EXECUTOR_H
#include "shell.h"
int execute_pipeline(Shell *sh, Pipeline *pipeline);
int foreground_job(Shell *sh, Job *job, int resume);
int background_job(Shell *sh, Job *job);
#endif
