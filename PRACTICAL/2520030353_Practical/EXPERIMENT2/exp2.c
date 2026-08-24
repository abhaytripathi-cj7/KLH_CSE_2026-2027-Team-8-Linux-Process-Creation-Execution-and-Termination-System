#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    pid_t pid;

    printf("Parent Process Started\n");
    printf("Parent PID  : %d\n", getpid());
    printf("Parent PPID : %d\n\n", getppid());

    pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0)
    {
        // Child process
        printf("----- CHILD PROCESS -----\n");
        printf("Child PID  : %d\n", getpid());
        printf("Child PPID : %d\n", getppid());

        printf("Child is RUNNING...\n");

        // Keep child active
        for (volatile long long i = 0; i < 2000000000LL; i++);

        printf("Child is WAITING...\n");
        sleep(10);

        printf("Child is RUNNING again...\n");
        sleep(2);

        printf("Child is TERMINATING...\n");
        exit(0);
    }
    else
    {
        // Parent process
        printf("----- PARENT PROCESS -----\n");
        printf("Parent PID : %d\n", getpid());
        printf("Child PID  : %d\n", pid);

        printf("Parent is WAITING for child...\n");

        wait(NULL);

        printf("Child has TERMINATED.\n");
        printf("Parent is RUNNING again.\n");

        sleep(3);

        printf("Parent process TERMINATING...\n");
    }

    return 0;
}
