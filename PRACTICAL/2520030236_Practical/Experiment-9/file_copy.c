#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BLOCK 65536

static double seconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static int copy_syscall(const char *src, const char *dst) {
    int in = open(src, O_RDONLY), out = -1;
    if (in < 0) return perror("open source"), -1;
    off_t size = lseek(in, 0, SEEK_END);
    if (size < 0 || lseek(in, 0, SEEK_SET) < 0) {
        perror("lseek (regular file required)"); close(in); return -1;
    }
    out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { perror("open destination"); close(in); return -1; }
    char buffer[BLOCK];
    ssize_t n;
    int ok = 1;
    while ((n = read(in, buffer, sizeof buffer)) > 0) {
        for (ssize_t done = 0; done < n;) {
            ssize_t written = write(out, buffer + done, (size_t)(n - done));
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) { perror("write"); ok = 0; break; }
            done += written;
        }
        if (!ok) break;
    }
    if (n < 0) { perror("read"); ok = 0; }
    if (close(out) < 0) { perror("close destination"); ok = 0; }
    if (close(in) < 0) { perror("close source"); ok = 0; }
    if (ok) printf("system calls: %lld bytes\n", (long long)size);
    return ok ? 0 : -1;
}

static int copy_stdio(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return perror("fopen source"), -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { perror("fopen destination"); fclose(in); return -1; }
    char buffer[BLOCK];
    size_t n, total = 0;
    int ok = 1;
    while ((n = fread(buffer, 1, sizeof buffer, in)) > 0) {
        if (fwrite(buffer, 1, n, out) != n) { perror("fwrite"); ok = 0; break; }
        total += n;
    }
    if (ferror(in)) { perror("fread"); ok = 0; }
    if (fclose(out) != 0) { perror("fclose destination"); ok = 0; }
    if (fclose(in) != 0) { perror("fclose source"); ok = 0; }
    if (ok) printf("standard I/O: %zu bytes\n", total);
    return ok ? 0 : -1;
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "sys") && strcmp(argv[1], "stdio"))) {
        fprintf(stderr, "usage: %s sys|stdio SOURCE DESTINATION\n", argv[0]);
        return 2;
    }
    struct stat a, b;
    if (stat(argv[2], &a) != 0) return perror("stat source"), 1;
    if (!S_ISREG(a.st_mode)) { fputs("source must be a regular file\n", stderr); return 1; }
    if (stat(argv[3], &b) == 0 && a.st_dev == b.st_dev && a.st_ino == b.st_ino) {
        fputs("source and destination are the same file\n", stderr); return 1;
    }
    double start = seconds();
    int result = strcmp(argv[1], "sys") == 0 ? copy_syscall(argv[2], argv[3]) : copy_stdio(argv[2], argv[3]);
    printf("elapsed: %.6f seconds (page cache and disk state affect results)\n", seconds() - start);
    return result == 0 ? 0 : 1;
}
