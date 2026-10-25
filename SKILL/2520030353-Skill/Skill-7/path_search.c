#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int executable(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0 &&
           S_ISREG(info.st_mode) &&
           access(path, X_OK) == 0;
}

int main(int argc, char *argv[])
{
    puts("Adi Reddy - Experiment 7: PATH Search");
    const char *command = argc > 1 ? argv[1] : "ls";

    if (!*command) {
        puts("Empty command.");
        return 1;
    }

    if (strchr(command, '/')) {
        if (executable(command)) {
            printf("Executable: %s\n", command);
            return 0;
        }
        puts("Path is not an executable regular file.");
        return 1;
    }

    const char *path = getenv("PATH");
    if (!path) {
        puts("PATH is not set.");
        return 1;
    }

    printf("PATH = %s\n", path);
    char *copy = strdup(path);
    if (!copy) {
        perror("strdup");
        return 1;
    }

    char *directory = copy;

    while (directory) {
        char *separator = strchr(directory, ':');
        if (separator)
            *separator = '\0';

        const char *folder = *directory ? directory : ".";
        size_t length = strlen(folder) + strlen(command) + 2;
        char *candidate = malloc(length);

        if (!candidate) {
            perror("malloc");
            free(copy);
            return 1;
        }

        snprintf(candidate, length, "%s/%s", folder, command);
        printf("Checking: %s\n", candidate);

        if (executable(candidate)) {
            printf("Executable found: %s\n", candidate);
            free(candidate);
            free(copy);
            return 0;
        }

        free(candidate);
        directory = separator ? separator + 1 : NULL;
    }

    printf("Command not found: %s\n", command);
    free(copy);
    return 1;
}
