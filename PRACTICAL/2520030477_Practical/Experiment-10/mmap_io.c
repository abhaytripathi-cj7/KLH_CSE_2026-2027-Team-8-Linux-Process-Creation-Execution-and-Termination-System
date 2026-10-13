#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static double seconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "mmap") && strcmp(argv[1], "rw"))) {
        fprintf(stderr, "usage: %s mmap|rw SOURCE DESTINATION\n", argv[0]);
        return 2;
    }
    int in = open(argv[2], O_RDONLY);
    if (in < 0) return perror("open source"), 1;
    struct stat st, dst;
    if (fstat(in, &st) != 0 || !S_ISREG(st.st_mode)) {
        fputs("source must be a regular file\n", stderr); close(in); return 1;
    }
    if (stat(argv[3], &dst) == 0 && st.st_dev == dst.st_dev && st.st_ino == dst.st_ino) {
        fputs("source and destination are the same file\n", stderr); close(in); return 1;
    }
    int out = open(argv[3], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (out < 0) return perror("open destination"), close(in), 1;
    double start = seconds();
    int ok = 1;
    if (strcmp(argv[1], "mmap") == 0) {
        if (ftruncate(out, st.st_size) != 0) { perror("ftruncate"); ok = 0; }
        if (ok && st.st_size > 0) {
            void *src = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, in, 0);
            void *dest = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, out, 0);
            if (src == MAP_FAILED || dest == MAP_FAILED) { perror("mmap"); ok = 0; }
            else { memcpy(dest, src, (size_t)st.st_size); if (msync(dest, (size_t)st.st_size, MS_SYNC) != 0) { perror("msync"); ok = 0; } }
            if (src != MAP_FAILED) munmap(src, (size_t)st.st_size);
            if (dest != MAP_FAILED) munmap(dest, (size_t)st.st_size);
        }
    } else {
        char buffer[65536];
        ssize_t n;
        while ((n = read(in, buffer, sizeof buffer)) > 0) {
            for (ssize_t used = 0; used < n;) {
                ssize_t written = write(out, buffer + used, (size_t)(n - used));
                if (written < 0 && errno == EINTR) continue;
                if (written <= 0) { perror("write"); ok = 0; break; }
                used += written;
            }
            if (!ok) break;
        }
        if (n < 0) { perror("read"); ok = 0; }
        if (fsync(out) != 0) { perror("fsync"); ok = 0; }
    }
    printf("%s: %lld bytes in %.6f seconds\n", argv[1], (long long)st.st_size, seconds() - start);
    if (close(out) != 0) { perror("close destination"); ok = 0; }
    if (close(in) != 0) { perror("close source"); ok = 0; }
    return ok ? 0 : 1;
}
