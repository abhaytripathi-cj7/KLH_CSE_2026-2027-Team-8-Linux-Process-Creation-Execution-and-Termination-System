#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main()
{
    char command[100];

    while (1)
    {
        // Accept Linux command from the user
        printf("\nEnter a Linux command (or type 'exit' to quit): ");
        scanf("%99s", command);

        // Exit condition
        if (strcmp(command, "exit") == 0)
        {
            printf("Exiting program...\n");
            break;
        }

        // Create a child process
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Fork failed");
            return 1;
        }
        else if (pid == 0)
        {
            // Child Process
            printf("\n----- Child Process -----\n");
            printf("Child PID  : %d\n", getpid());
            printf("Parent PID : %d\n", getppid());

            printf("Executing command: %s\n\n", command);
 // Execute the command
            execlp(command, command, NULL);

            // If exec() fails
            perror("Execution failed");
            exit(1);
        }
        else
        {
            // Parent Process
            printf("\n----- Parent Process -----\n");
            printf("Parent PID : %d\n", getpid());
            printf("Child PID  : %d\n", pid);

            // Wait for child to finish
            wait(NULL);

            printf("\nCommand '%s' executed successfully.\n", command);
        }
    }

    return 0;
}
