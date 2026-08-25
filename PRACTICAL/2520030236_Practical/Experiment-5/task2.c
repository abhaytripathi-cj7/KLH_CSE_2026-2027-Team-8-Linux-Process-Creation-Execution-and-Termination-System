#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    int pipefd[2];
    pid_t p1, p2;

    // Create pipe
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(1);
    }

    // Create first child
    p1 = fork();

    if (p1 == 0)
    {
        // Child 1: execute ls -l

        // Redirect stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Close unused pipe ends
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute ls -l
        execlp("ls", "ls", "-l", NULL);

        perror("execlp");
        exit(1);
    }

    // Create second child
    p2 = fork();

    if (p2 == 0)
    {
        // Child 2: execute grep ".c"

        // Redirect stdin from pipe
        dup2(pipefd[0], STDIN_FILENO);

        // Close unused pipe ends
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute grep ".c"
        execlp("grep", "grep", ".c", NULL);

        perror("execlp");
        exit(1);
    }

    // Parent closes pipe
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both child processes
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);

    return 0;
}
