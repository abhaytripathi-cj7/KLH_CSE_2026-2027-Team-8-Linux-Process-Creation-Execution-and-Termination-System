#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_memory(const char *stage)
{
    printf("\n%s\n", stage);

    FILE *file = fopen("/proc/self/status", "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    char line[256];

    while (fgets(line, sizeof line, file)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0)
            fputs(line, stdout);
    }

    fclose(file);
}

void intentional_leak(void)
{
    void *ptr = malloc(4096);

    if (ptr == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memset(ptr, 1, 4096);
    printf("Intentional leak: %p (4096 bytes)\n", ptr);

    /* Uncomment to fix the leak:
       free(ptr);
    */
}

int main(int argc, char *argv[])
{
    size_t size = 4 * 1024 * 1024;

    show_memory("Before allocation:");

    unsigned char *a = malloc(size);
    unsigned char *b = calloc(size, sizeof *b);

    if (a == NULL || b == NULL) {
        perror("allocation");
        free(a);
        free(b);
        return 1;
    }

    printf("malloc address: %p\n", (void *)a);
    printf("calloc address: %p\n", (void *)b);
    printf("calloc initial value: %u\n", (unsigned)b[0]);

    show_memory("After allocation:");

    memset(a, 10, size);
    memset(b, 20, size);

    show_memory("After writing to allocated memory:");

    unsigned char *temp = realloc(a, 2 * size);

    if (temp == NULL) {
        perror("realloc");
        free(a);
        free(b);
        return 1;
    }

    a = temp;
    printf("realloc address: %p\n", (void *)a);

    for (size_t i = 0; i < size; i++) {
        if (a[i] != 10) {
            fprintf(stderr, "Data preservation failed\n");
            free(a);
            free(b);
            return 1;
        }
    }

    memset(a + size, 30, size);

    show_memory("After realloc and writing:");

    free(a);
    free(b);

    show_memory("After free:");

    if (argc > 1 && strcmp(argv[1], "--leak") == 0)
        intentional_leak();

    return 0;
}
