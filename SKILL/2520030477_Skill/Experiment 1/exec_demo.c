#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    char command[100];

    printf("Enter a command: ");
    scanf("%99s", command);

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Fork failed!\n");
        return 1;
    }
    else if (pid == 0)
    {
        // Child Process executes the command
        char *args[] = {command, NULL};

        execvp(command, args);

        // Executes only if execvp() fails
        printf("Invalid command!\n");
        exit(1);
    }
    else
    {
        // Parent waits for child process
        wait(NULL);
        printf("Parent Process: Child execution completed.\n");
    }

    return 0;
}
