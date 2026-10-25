#define _POSIX_C_SOURCE 200809L
#include "shell.h"
#include "parser.h"
#include "executor.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static volatile sig_atomic_t child_changed;
static void child_handler(int signal_number) { (void)signal_number; child_changed = 1; }

void shell_add_history(Shell *sh, const char *line) {
    if (!*line) return;
    int slot;
    if (sh->history_count == HISTORY_CAPACITY) {
        slot = sh->history_start;
        free(sh->history[slot]);
        sh->history_start = (sh->history_start + 1) % HISTORY_CAPACITY;
    } else {
        slot = (sh->history_start + sh->history_count++) % HISTORY_CAPACITY;
    }
    sh->history[slot] = strdup(line);
}

Job *shell_find_job(Shell *sh, int id) {
    for (int i = 0; i < MAX_JOBS; ++i) if (sh->jobs[i].id == id) return &sh->jobs[i];
    return NULL;
}

void shell_remove_job(Job *job) { free(job->text); memset(job, 0, sizeof *job); }

static void setup_signals(Shell *sh) {
    struct sigaction sa = {.sa_handler = child_handler};
    sigemptyset(&sa.sa_mask); sigaction(SIGCHLD, &sa, NULL);
    if (sh->interactive) {
        signal(SIGINT, SIG_IGN); signal(SIGTSTP, SIG_IGN);
        signal(SIGQUIT, SIG_IGN); signal(SIGTTIN, SIG_IGN); signal(SIGTTOU, SIG_IGN);
        sh->shell_pgid = getpid();
        if (setpgid(0, sh->shell_pgid) < 0 && errno != EACCES && errno != EPERM) perror("setpgid shell");
        if (tcsetpgrp(STDIN_FILENO, sh->shell_pgid) < 0) perror("tcsetpgrp shell");
    }
}

int shell_run(void) {
    Shell sh = {.next_job_id = 0, .running = 1, .interactive = isatty(STDIN_FILENO)};
    setup_signals(&sh);
    char *line = NULL; size_t size = 0;
    while (sh.running) {
        if (child_changed) { child_changed = 0; shell_update_jobs(&sh); }
        if (sh.interactive) { fputs("ossp> ", stdout); fflush(stdout); }
        ssize_t n = getline(&line, &size, stdin);
        if (n < 0) {
            if (errno == EINTR) { clearerr(stdin); continue; }
            break;
        }
        if (n && line[n - 1] == '\n') line[n - 1] = 0;
        if (!*line) continue;
        shell_add_history(&sh, line);
        Pipeline pipeline; char error[160] = "";
        if (parse_line(line, &pipeline, error, sizeof error) < 0) {
            fprintf(stderr, "syntax: %s\n", error); sh.exit_status = 2; continue;
        }
        if (pipeline.count) {
            int result = execute_pipeline(&sh, &pipeline);
            if (sh.running) sh.exit_status = result;
        }
        free_pipeline(&pipeline);
    }
    free(line); free(sh.previous_dir);
    for (int i = 0; i < sh.history_count; ++i) free(sh.history[(sh.history_start + i) % HISTORY_CAPACITY]);
    for (int i = 0; i < MAX_JOBS; ++i) if (sh.jobs[i].id) shell_remove_job(&sh.jobs[i]);
    return sh.exit_status;
}
