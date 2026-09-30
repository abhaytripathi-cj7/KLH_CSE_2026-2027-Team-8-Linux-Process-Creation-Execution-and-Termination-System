#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile long counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t start_barrier;
static long iterations;
static int synchronized;

static void *worker(void *unused) {
    (void)unused;
    pthread_barrier_wait(&start_barrier);
    for (long i = 0; i < iterations; ++i) {
        if (synchronized) pthread_mutex_lock(&lock);
        counter = counter + 1;
        if (synchronized) pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "race") && strcmp(argv[1], "mutex"))) {
        fprintf(stderr, "usage: %s race|mutex THREADS ITERATIONS\n", argv[0]);
        return 2;
    }
    char *end;
    long threads = strtol(argv[2], &end, 10);
    if (*end || threads < 1 || threads > 128) return fprintf(stderr, "THREADS must be 1..128\n"), 2;
    iterations = strtol(argv[3], &end, 10);
    if (*end || iterations < 1 || iterations > 100000000) return fprintf(stderr, "ITERATIONS must be 1..100000000\n"), 2;
    synchronized = strcmp(argv[1], "mutex") == 0;
    pthread_t *ids = calloc((size_t)threads, sizeof *ids);
    if (!ids || pthread_barrier_init(&start_barrier, NULL, (unsigned)threads + 1) != 0) {
        perror("allocation/barrier"); free(ids); return 1;
    }
    for (long i = 0; i < threads; ++i)
        if (pthread_create(&ids[i], NULL, worker, NULL) != 0) {
            fprintf(stderr, "pthread_create failed\n"); return 1;
        }
    pthread_barrier_wait(&start_barrier);
    for (long i = 0; i < threads; ++i) pthread_join(ids[i], NULL);
    printf("mode=%s expected=%ld observed=%ld\n", argv[1], threads * iterations, counter);
    pthread_barrier_destroy(&start_barrier);
    pthread_mutex_destroy(&lock);
    free(ids);
    return synchronized && counter != threads * iterations ? 1 : 0;
}
