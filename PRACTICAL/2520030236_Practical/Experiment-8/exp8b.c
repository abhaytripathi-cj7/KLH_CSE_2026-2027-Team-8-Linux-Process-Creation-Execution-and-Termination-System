#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

void fail(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void send_signal(int fd)
{
    ssize_t result;

    do {
        result = write(fd, "x", 1);
    } while (result < 0 && errno == EINTR);

    if (result != 1)
        exit(EXIT_FAILURE);
}

void wait_signal(int fd)
{
    char value;
    ssize_t result;

    do {
        result = read(fd, &value, 1);
    } while (result < 0 && errno == EINTR);

    if (result != 1)
        exit(EXIT_FAILURE);
}

void show_memory(pid_t pid, void *address, const char *stage)
{
    char path[64], line[512];
    unsigned long start, end;
    int selected = 0;

    snprintf(path, sizeof path, "/proc/%ld/smaps", (long)pid);

    FILE *file = fopen(path, "r");
    if (file == NULL)
        fail("fopen");

    printf("\n%s: PID=%ld, address=%p\n",
           stage, (long)pid, address);

    while (fgets(line, sizeof line, file)) {
        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            selected = (uintptr_t)address >= start &&
                       (uintptr_t)address < end;
        } else if (selected &&
                   (strncmp(line, "Rss:", 4) == 0 ||
                    strncmp(line, "Pss:", 4) == 0 ||
                    strncmp(line, "Shared_Dirty:", 13) == 0 ||
                    strncmp(line, "Private_Dirty:", 14) == 0)) {
            fputs(line, stdout);
        }
    }

    fclose(file);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
        return 1;

    size_t page = (size_t)page_size;
    size_t size = ((64UL * 1024 * 1024 + page - 1) / page) * page;

    /* Guard pages isolate the measured mapping. */
    unsigned char *base = mmap(NULL, size + 2 * page,
                              PROT_NONE,
                              MAP_PRIVATE | MAP_ANONYMOUS,
                              -1, 0);

    if (base == MAP_FAILED)
        fail("mmap");

    unsigned char *data = base + page;

    if (mprotect(data, size, PROT_READ | PROT_WRITE) == -1)
        fail("mprotect");

    if (madvise(data, size, MADV_NOHUGEPAGE) == -1)
        perror("madvise");

    memset(data, 'A', size);

    show_memory(getpid(), data, "Before fork");

    int commands[2], completed[2];

    if (pipe(commands) == -1 || pipe(completed) == -1)
        fail("pipe");

    pid_t child = fork();

    if (child == -1)
        fail("fork");

    if (child == 0) {
        close(commands[1]);
        close(completed[0]);

        wait_signal(commands[0]);

        struct rusage before, after;

        if (getrusage(RUSAGE_SELF, &before) == -1)
            fail("getrusage");

        for (size_t i = 0; i < size; i += page)
            data[i] = 'B';

        if (getrusage(RUSAGE_SELF, &after) == -1)
            fail("getrusage");

        printf("\nChild minor page faults: %ld\n",
               after.ru_minflt - before.ru_minflt);
        printf("Child first byte: %c\n", data[0]);

        send_signal(completed[1]);
        wait_signal(commands[0]);

        close(commands[0]);
        close(completed[1]);
        munmap(base, size + 2 * page);

        _exit(0);
    }

    close(commands[0]);
    close(completed[1]);

    show_memory(getpid(), data, "Parent before child writes");
    show_memory(child, data, "Child before writes");

    send_signal(commands[1]);
    wait_signal(completed[0]);

    show_memory(getpid(), data, "Parent after child writes");
    show_memory(child, data, "Child after writes");

    int unchanged = 1;

    for (size_t i = 0; i < size; i++) {
        if (data[i] != 'A') {
            unchanged = 0;
            break;
        }
    }

    printf("\nParent first byte: %c\n", data[0]);
    printf("Parent data unchanged: %s\n",
           unchanged ? "YES" : "NO");

    send_signal(commands[1]);

    close(commands[1]);
    close(completed[0]);

    int status;
    pid_t result;

    do {
        result = waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result == -1)
        fail("waitpid");

    munmap(base, size + 2 * page);

    return unchanged && WIFEXITED(status) &&
           WEXITSTATUS(status) == 0 ? 0 : 1;
}
