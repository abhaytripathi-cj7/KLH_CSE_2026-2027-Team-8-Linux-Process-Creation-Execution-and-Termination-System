#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0)
    {
        printf("Child process\n");
        printf("Child PID : %d\n", getpid());
        printf("Child exiting...\n");
        exit(0);
    }
    else
    {
        printf("Parent process\n");
        printf("Parent PID : %d\n", getpid());
        printf("Child PID : %d\n", pid);

        wait(NULL);

        printf("Child process collected.\n");
        printf("No zombie process exists.\n");
    }

    return 0;
}
