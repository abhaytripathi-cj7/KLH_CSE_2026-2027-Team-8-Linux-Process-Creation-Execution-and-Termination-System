#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int *buffer, capacity, total, head, tail;
static long produced_sum, consumed_sum;
static sem_t empty_slots, full_slots;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

static double seconds(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void *producer(void *unused) {
    (void)unused;
    for (int value = 1; value <= total; ++value) {
        sem_wait(&empty_slots);
        pthread_mutex_lock(&queue_lock);
        buffer[tail] = value;
        tail = (tail + 1) % capacity;
        produced_sum += value;
        pthread_mutex_unlock(&queue_lock);
        sem_post(&full_slots);
    }
    return NULL;
}

static void *consumer(void *unused) {
    (void)unused;
    for (int i = 0; i < total; ++i) {
        sem_wait(&full_slots);
        pthread_mutex_lock(&queue_lock);
        int value = buffer[head];
        head = (head + 1) % capacity;
        consumed_sum += value;
        pthread_mutex_unlock(&queue_lock);
        sem_post(&empty_slots);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) return fprintf(stderr, "usage: %s BUFFER_SIZE ITEMS\n", argv[0]), 2;
    char *end;
    long parsed = strtol(argv[1], &end, 10);
    if (*end || parsed < 1 || parsed > 1000000) return fprintf(stderr, "BUFFER_SIZE must be 1..1000000\n"), 2;
    capacity = (int)parsed;
    parsed = strtol(argv[2], &end, 10);
    if (*end || parsed < 1 || parsed > 100000000) return fprintf(stderr, "ITEMS must be 1..100000000\n"), 2;
    total = (int)parsed;
    buffer = calloc((size_t)capacity, sizeof *buffer);
    if (!buffer || sem_init(&empty_slots, 0, (unsigned)capacity) != 0 || sem_init(&full_slots, 0, 0) != 0) {
        perror("allocation/sem_init"); free(buffer); return 1;
    }
    pthread_t a, b;
    double start = seconds();
    if (pthread_create(&a, NULL, producer, NULL) != 0 || pthread_create(&b, NULL, consumer, NULL) != 0)
        return fprintf(stderr, "pthread_create failed\n"), 1;
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    double elapsed = seconds() - start;
    long expected = (long)total * (total + 1) / 2;
    printf("buffer=%d items=%d produced_sum=%ld consumed_sum=%ld expected=%ld\n", capacity, total, produced_sum, consumed_sum, expected);
    printf("elapsed=%.6f seconds throughput=%.0f items/second\n", elapsed, total / elapsed);
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&queue_lock);
    free(buffer);
    return produced_sum == expected && consumed_sum == expected ? 0 : 1;
}
