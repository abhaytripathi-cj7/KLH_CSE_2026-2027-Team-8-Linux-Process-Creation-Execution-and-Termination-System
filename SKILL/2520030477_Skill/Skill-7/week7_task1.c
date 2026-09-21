#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {

    pid_t pid;
    int status;

    printf("=== waitpid() Child Process Monitoring ===\n");

    pid = fork();

    if (pid < 0) {

        perror("fork failed");
        return 1;

    } else if (pid == 0) {

        printf("\n[CHILD]\n");
        printf("Child PID : %d\n", getpid());
        printf("Parent PID: %d\n", getppid());

        printf("Child is performing work...\n");

        sleep(5);

        printf("Child finished the work.\n");

        exit(42);

    } else {

        printf("\n[PARENT]\n");
        printf("Parent PID: %d\n", getpid());
        printf("Monitoring Child PID: %d\n\n", pid);

        pid_t result;

        do {

            result = waitpid(pid, &status, WNOHANG);

            if (result == 0) {

                printf("Parent: Child is still running...\n");
                sleep(1);

            } else if (result == -1) {

                perror("waitpid");
                return 1;
            }

        } while (result == 0);

        printf("\nParent: Child process has terminated.\n");

        if (WIFEXITED(status)) {

            printf("Child exited normally.\n");
            printf("Exit status = %d\n",
                   WEXITSTATUS(status));

        } else if (WIFSIGNALED(status)) {

            printf("Child terminated by signal = %d\n",
                   WTERMSIG(status));
        }
    }

    printf("\nProgram completed.\n");

    return 0;
}
