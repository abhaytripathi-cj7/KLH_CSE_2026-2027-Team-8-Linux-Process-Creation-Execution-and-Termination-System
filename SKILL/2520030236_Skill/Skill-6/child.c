#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    puts("Adi Reddy - Experiment 6: Child Process");
    fflush(stdout);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("Adi Reddy's child PID: %ld\n", (long)getpid());
        fflush(stdout);

        if (argc > 1)
            execvp(argv[1], &argv[1]);
        else
            execlp("ls", "ls", "-l", (char *)NULL);

        perror("exec");
        _exit(127);
    }

    int status;
    pid_t result;
    do {
        result = waitpid(pid, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        perror("waitpid");
        return 1;
    }

    printf("Adi Reddy's parent PID: %ld\n", (long)getpid());
    if (WIFEXITED(status)) {
        printf("Child exit status: %d\n", WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status))
        printf("Child terminated by signal %d\n", WTERMSIG(status));

    return 1;
}
