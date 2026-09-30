#define _POSIX_C_SOURCE 200809L
#include "executor.h"
#include "builtin.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static int redirect_command(Command *cmd) {
    for (int i = 0; i < cmd->nredirs; ++i) {
        Redir *r = &cmd->redirs[i];
        if (r->type == R_ERR_TO_OUT) {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) return perror("dup2"), -1;
            continue;
        }
        int target = r->type == R_IN ? STDIN_FILENO :
            (r->type == R_ERR || r->type == R_ERR_APPEND ? STDERR_FILENO : STDOUT_FILENO);
        int flags = r->type == R_IN ? O_RDONLY : O_WRONLY | O_CREAT |
            (r->type == R_APPEND || r->type == R_ERR_APPEND ? O_APPEND : O_TRUNC);
        int fd = open(r->path, flags, 0644);
        if (fd < 0) return perror(r->path), -1;
        if (dup2(fd, target) < 0) { perror("dup2"); close(fd); return -1; }
        close(fd);
    }
    return 0;
}

static int run_parent_builtin(Shell *sh, Command *cmd) {
    int saved[3];
    fflush(NULL);
    for (int i = 0; i < 3; ++i) {
        saved[i] = dup(i);
        if (saved[i] < 0) { perror("dup"); return 1; }
    }
    int result = redirect_command(cmd) == 0 ? run_builtin(sh, cmd) : 1;
    fflush(NULL);
    for (int i = 0; i < 3; ++i) { dup2(saved[i], i); close(saved[i]); }
    return result;
}

static void child_signals(void) {
    signal(SIGINT, SIG_DFL); signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL); signal(SIGTTIN, SIG_DFL); signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

static void note_status(Job *job, pid_t pid, int status, int *last_status) {
    if (WIFSTOPPED(status)) { job->stopped = 1; return; }
    if (WIFCONTINUED(status)) { job->stopped = 0; return; }
    for (int i = 0; i < job->count; ++i) {
        if (job->pids[i] == pid) {
            job->pids[i] = 0;
            job->remaining--;
            if (i == job->count - 1 && last_status)
                *last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            break;
        }
    }
    if (job->remaining == 0) job->done = 1;
}

int foreground_job(Shell *sh, Job *job, int resume) {
    if (sh->interactive && tcsetpgrp(STDIN_FILENO, job->pgid) < 0) perror("tcsetpgrp job");
    if (resume) { job->stopped = 0; kill(-job->pgid, SIGCONT); }
    int result = 0;
    while (job->remaining > 0 && !job->stopped) {
        int status;
        pid_t pid = waitpid(-job->pgid, &status, WUNTRACED | WCONTINUED);
        if (pid < 0) { if (errno == EINTR) continue; break; }
        note_status(job, pid, status, &result);
    }
    if (sh->interactive && tcsetpgrp(STDIN_FILENO, sh->shell_pgid) < 0) perror("tcsetpgrp shell");
    if (job->stopped) printf("[%d] Stopped %s\n", job->id, job->text);
    else if (job->done) shell_remove_job(job);
    return result;
}

int background_job(Shell *sh, Job *job) {
    (void)sh;
    if (!job || job->done) return fprintf(stderr, "bg: job unavailable\n"), 1;
    if (kill(-job->pgid, SIGCONT) < 0) return perror("bg"), 1;
    job->stopped = 0;
    printf("[%d] Running %s\n", job->id, job->text);
    return 0;
}

void shell_update_jobs(Shell *sh) {
    for (;;) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid <= 0) break;
        for (int j = 0; j < MAX_JOBS; ++j) {
            Job *job = &sh->jobs[j];
            for (int k = 0; k < job->count; ++k)
                if (job->pids[k] == pid) { note_status(job, pid, status, NULL); goto next; }
        }
next: ;
    }
}

int execute_pipeline(Shell *sh, Pipeline *p) {
    if (p->count == 0) return 0;
    if (p->count == 1 && !p->background && is_builtin(p->commands[0].argv[0]))
        return run_parent_builtin(sh, &p->commands[0]);
    Job *job = NULL;
    for (int i = 0; i < MAX_JOBS; ++i) if (sh->jobs[i].id == 0) { job = &sh->jobs[i]; break; }
    if (!job) return fprintf(stderr, "job table full\n"), 1;
    job->id = ++sh->next_job_id;
    job->text = strdup(p->text);
    if (!job->text) { shell_remove_job(job); return 1; }
    int previous = -1;
    for (int i = 0; i < p->count; ++i) {
        int pipefd[2] = {-1, -1};
        if (i < p->count - 1 && pipe(pipefd) < 0) { perror("pipe"); goto failed; }
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); if (pipefd[0] >= 0) { close(pipefd[0]); close(pipefd[1]); } goto failed; }
        if (pid == 0) {
            pid_t group = job->pgid ? job->pgid : getpid();
            setpgid(0, group); child_signals();
            if (previous >= 0) { dup2(previous, STDIN_FILENO); close(previous); }
            if (pipefd[1] >= 0) { dup2(pipefd[1], STDOUT_FILENO); close(pipefd[1]); close(pipefd[0]); }
            if (redirect_command(&p->commands[i]) != 0) _exit(1);
            Command *cmd = &p->commands[i];
            if (is_builtin(cmd->argv[0])) {
                int result = run_builtin(sh, cmd);
                fflush(NULL);
                _exit(result);
            }
            execvp(cmd->argv[0], cmd->argv);
            fprintf(stderr, "%s: %s\n", cmd->argv[0], strerror(errno)); _exit(127);
        }
        if (!job->pgid) job->pgid = pid;
        setpgid(pid, job->pgid);
        job->pids[job->count++] = pid; job->remaining++;
        if (previous >= 0) close(previous);
        if (pipefd[1] >= 0) close(pipefd[1]);
        previous = pipefd[0];
    }
    if (previous >= 0) close(previous);
    if (p->background) {
        printf("[%d] %ld\n", job->id, (long)job->pgid);
        return 0;
    }
    return foreground_job(sh, job, 0);
failed:
    if (previous >= 0) close(previous);
    if (job->pgid) { kill(-job->pgid, SIGTERM); while (waitpid(-job->pgid, NULL, 0) > 0) {} }
    shell_remove_job(job); return 1;
}
