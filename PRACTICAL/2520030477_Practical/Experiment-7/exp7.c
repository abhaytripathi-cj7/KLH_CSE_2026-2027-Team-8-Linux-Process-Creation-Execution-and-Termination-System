#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int global_initialized = 10;
int global_uninitialized;

static int static_initialized = 20;
static int static_uninitialized;

void function(void) {}

int main(void)
{
    int stack_variable = 30;
    static int local_static = 40;
    int *heap_variable = malloc(sizeof *heap_variable);

    if (heap_variable == NULL) {
        perror("malloc");
        return 1;
    }

    *heap_variable = 50;

    printf("PID: %ld\n", (long)getpid());
    printf("Code address:          %p\n", (void *)function);
    printf("Initialized global:    %p\n", (void *)&global_initialized);
    printf("Uninitialized global:  %p\n", (void *)&global_uninitialized);
    printf("Initialized static:    %p\n", (void *)&static_initialized);
    printf("Uninitialized static:  %p\n", (void *)&static_uninitialized);
    printf("Local static:          %p\n", (void *)&local_static);
    printf("Heap address:          %p\n", (void *)heap_variable);
    printf("Stack address:         %p\n", (void *)&stack_variable);

    FILE *file = fopen("/proc/self/maps", "r");
    if (file == NULL) {
        perror("fopen");
        free(heap_variable);
        return 1;
    }

    char line[1024];
    puts("\nProcess memory mappings:");

    while (fgets(line, sizeof line, file))
        fputs(line, stdout);

    fclose(file);

    printf("\nIn another terminal: pmap -x %ld\n", (long)getpid());
    puts("Press Enter to exit.");
    getchar();

    free(heap_variable);
    return 0;
}
