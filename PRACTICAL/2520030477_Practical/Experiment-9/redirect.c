#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s INPUT OUTPUT\n", argv[0]);
        return 2;
    }
    int in = open(argv[1], O_RDONLY);
    if (in < 0) return perror("open input"), 1;
    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) return perror("open output"), close(in), 1;
    if (dup2(in, STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0)
        return perror("dup2"), close(in), close(out), 1;
    close(in);
    close(out);
    char buffer[4096];
    ssize_t n;
    while ((n = read(STDIN_FILENO, buffer, sizeof buffer)) > 0) {
        for (ssize_t used = 0; used < n;) {
            ssize_t written = write(STDOUT_FILENO, buffer + used, (size_t)(n - used));
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) return perror("write"), 1;
            used += written;
        }
    }
    if (n < 0) return perror("read"), 1;
    fprintf(stderr, "Copied redirected stdin to stdout; inspect %s\n", argv[2]);
    return 0;
}
