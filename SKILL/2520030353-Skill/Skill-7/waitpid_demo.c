#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    puts("Adi Reddy - Experiment 7: waitpid");
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        puts("Adi Reddy's child process is running...");
        sleep(2);
        puts("Child process finished.");
        return 10;
    }

    puts("Adi Reddy's parent process is waiting...");

    int status;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        printf("Child exited with status: %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Child terminated by signal: %d\n", WTERMSIG(status));

    puts("Parent process finished.");
    return 0;
}
