#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid1, pid2;

    // Create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    // Create first child for ls -l
    pid1 = fork();

    if (pid1 < 0) {
        perror("fork");
        exit(1);
    }

    if (pid1 == 0) {
        // Child 1: ls -l

        // Redirect stdout to pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Close unused pipe ends
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute ls -l
        execlp("ls", "ls", "-l", (char *)NULL);

        // If exec fails
        perror("execlp ls");
        exit(1);
    }

    // Create second child for grep ".c"
    pid2 = fork();

    if (pid2 < 0) {
        perror("fork");
        exit(1);
    }

    if (pid2 == 0) {
        // Child 2: grep ".c"

        // Redirect stdin from pipe
        dup2(pipefd[0], STDIN_FILENO);

        // Close unused pipe ends
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute grep ".c"
        execlp("grep", "grep", ".c", (char *)NULL);

        // If exec fails
        perror("execlp grep");
        exit(1);
    }

    // Parent closes both pipe ends
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
