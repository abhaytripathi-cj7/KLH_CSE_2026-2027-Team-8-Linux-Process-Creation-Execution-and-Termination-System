#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include "executor.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int valid_name(const char *name, size_t length) {
    if (!length || !(isalpha((unsigned char)name[0]) || name[0] == '_')) return 0;
    for (size_t i = 1; i < length; ++i)
        if (!(isalnum((unsigned char)name[i]) || name[i] == '_')) return 0;
    return 1;
}

int is_builtin(const char *name) {
    const char *names[] = {"cd", "pwd", "exit", "export", "history", "jobs", "fg", "bg", "help"};
    if (!name) return 0;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; ++i)
        if (strcmp(name, names[i]) == 0) return 1;
    return 0;
}

static Job *job_arg(Shell *sh, Command *cmd) {
    if (cmd->argc == 1) {
        Job *latest = NULL;
        for (int i = 0; i < MAX_JOBS; ++i)
            if (sh->jobs[i].id && !sh->jobs[i].done && (!latest || sh->jobs[i].id > latest->id)) latest = &sh->jobs[i];
        return latest;
    }
    const char *arg = cmd->argv[1];
    if (*arg == '%') arg++;
    char *end;
    long id = strtol(arg, &end, 10);
    if (*end || id <= 0) return NULL;
    return shell_find_job(sh, (int)id);
}

int run_builtin(Shell *sh, Command *cmd) {
    const char *name = cmd->argv[0];
    if (strcmp(name, "cd") == 0) {
        if (cmd->argc > 2) return fprintf(stderr, "cd: too many arguments\n"), 1;
        const char *target = cmd->argc == 1 ? getenv("HOME") : cmd->argv[1];
        if (target && strcmp(target, "-") == 0) target = sh->previous_dir;
        if (!target) return fprintf(stderr, "cd: directory unavailable\n"), 1;
        char *old = getcwd(NULL, 0);
        if (chdir(target) < 0) { perror("cd"); free(old); return 1; }
        free(sh->previous_dir); sh->previous_dir = old;
        if (cmd->argc > 1 && strcmp(cmd->argv[1], "-") == 0) {
            char *now = getcwd(NULL, 0); if (now) { puts(now); free(now); }
        }
        return 0;
    }
    if (strcmp(name, "pwd") == 0) {
        char *cwd = getcwd(NULL, 0);
        if (!cwd) return perror("pwd"), 1;
        puts(cwd); free(cwd); return 0;
    }
    if (strcmp(name, "exit") == 0) {
        int code = sh->exit_status;
        if (cmd->argc > 2) return fprintf(stderr, "exit: too many arguments\n"), 1;
        if (cmd->argc == 2) {
            char *end; long n = strtol(cmd->argv[1], &end, 10);
            if (*end) return fprintf(stderr, "exit: numeric argument required\n"), 2;
            code = (unsigned char)n;
        }
        sh->running = 0; sh->exit_status = code; return code;
    }
    if (strcmp(name, "export") == 0) {
        if (cmd->argc != 2) return fprintf(stderr, "usage: export NAME=VALUE\n"), 2;
        char *eq = strchr(cmd->argv[1], '=');
        if (!eq || !valid_name(cmd->argv[1], (size_t)(eq - cmd->argv[1])))
            return fprintf(stderr, "export: invalid variable name\n"), 2;
        char *key = strndup(cmd->argv[1], (size_t)(eq - cmd->argv[1]));
        if (!key) return 1;
        int result = setenv(key, eq + 1, 1);
        free(key); if (result < 0) return perror("export"), 1;
        return 0;
    }
    if (strcmp(name, "history") == 0) {
        for (int i = 0; i < sh->history_count; ++i)
            printf("%d %s\n", i + 1, sh->history[(sh->history_start + i) % HISTORY_CAPACITY]);
        return 0;
    }
    if (strcmp(name, "jobs") == 0) {
        shell_update_jobs(sh);
        for (int i = 0; i < MAX_JOBS; ++i) {
            Job *j = &sh->jobs[i];
            if (!j->id) continue;
            printf("[%d] %-7s %s\n", j->id, j->done ? "Done" : j->stopped ? "Stopped" : "Running", j->text);
            if (j->done) shell_remove_job(j);
        }
        return 0;
    }
    if (strcmp(name, "fg") == 0 || strcmp(name, "bg") == 0) {
        shell_update_jobs(sh);
        Job *job = job_arg(sh, cmd);
        if (!job || job->done) return fprintf(stderr, "%s: job not found\n", name), 1;
        return strcmp(name, "fg") == 0 ? foreground_job(sh, job, 1) : background_job(sh, job);
    }
    if (strcmp(name, "help") == 0) {
        puts("Builtins: cd pwd export history jobs fg bg exit help");
        puts("Operators: | < > >> 2> 2>> 2>&1 & ; quotes and $NAME/${NAME} expansion");
        return 0;
    }
    return 127;
}
