#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100

int main() {

    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    printf("=== Child Process and Command Execution ===\n");
    printf("Enter command: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        perror("fgets");
        return 1;
    }

    input[strcspn(input, "\n")] = '\0';

    int argc = 0;

    char *token = strtok(input, " ");

    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }

    args[argc] = NULL;

    if (argc == 0) {
        printf("No command entered.\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {

        perror("fork failed");
        return 1;

    } else if (pid == 0) {

        printf("\nChild Process\n");
        printf("Child PID: %d\n", getpid());
        printf("Parent PID: %d\n", getppid());

        execvp(args[0], args);

        /*
         * This executes only when execvp fails.
         */
        perror("Execution failed");

        exit(EXIT_FAILURE);

    } else {

        int status;

        printf("\nParent Process\n");
        printf("Parent PID : %d\n", getpid());
        printf("Child PID  : %d\n", pid);

        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {

            printf("\nChild terminated normally.\n");
            printf("Exit status: %d\n",
                   WEXITSTATUS(status));

        } else if (WIFSIGNALED(status)) {

            printf("\nChild terminated by signal %d\n",
                   WTERMSIG(status));
        }
    }

    return 0;
}
