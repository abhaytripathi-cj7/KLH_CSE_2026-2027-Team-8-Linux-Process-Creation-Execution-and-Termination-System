#ifndef OSSP_SHELL_H
#define OSSP_SHELL_H
#include <sys/types.h>
#define MAX_ARGS 64
#define MAX_COMMANDS 16
#define MAX_REDIRS 16
#define MAX_JOBS 64
#define HISTORY_CAPACITY 100

typedef enum { R_IN, R_OUT, R_APPEND, R_ERR, R_ERR_APPEND, R_ERR_TO_OUT } RedirType;
typedef struct { RedirType type; char *path; } Redir;
typedef struct { char *argv[MAX_ARGS]; int argc; Redir redirs[MAX_REDIRS]; int nredirs; } Command;
typedef struct { Command commands[MAX_COMMANDS]; int count; int background; char *text; } Pipeline;
typedef struct {
    int id, count, remaining, stopped, done;
    pid_t pgid, pids[MAX_COMMANDS];
    char *text;
} Job;
typedef struct {
    Job jobs[MAX_JOBS];
    int next_job_id, running, exit_status, interactive;
    pid_t shell_pgid;
    char *previous_dir;
    char *history[HISTORY_CAPACITY];
    int history_start, history_count;
} Shell;

void shell_add_history(Shell *sh, const char *line);
void shell_update_jobs(Shell *sh);
Job *shell_find_job(Shell *sh, int id);
void shell_remove_job(Job *job);
#endif
