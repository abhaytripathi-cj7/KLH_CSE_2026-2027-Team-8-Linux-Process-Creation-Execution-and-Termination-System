#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER, b = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t barrier;
static int ordered;

static void *worker(void *arg) {
    long id = (long)arg;
    pthread_mutex_t *first = id == 0 || ordered ? &a : &b;
    pthread_mutex_t *second = id == 0 || ordered ? &b : &a;
    pthread_mutex_lock(first);
    if (!ordered) pthread_barrier_wait(&barrier);
    pthread_mutex_lock(second);
    printf("thread %ld acquired both resources\n", id);
    pthread_mutex_unlock(second);
    pthread_mutex_unlock(first);
    return NULL;
}

static int run_threads(int prevent) {
    ordered = prevent;
    pthread_t one, two;
    if (pthread_barrier_init(&barrier, NULL, 2) != 0) return 1;
    if (pthread_create(&one, NULL, worker, (void *)0L) != 0 ||
        pthread_create(&two, NULL, worker, (void *)1L) != 0) return 1;
    pthread_join(one, NULL);
    pthread_join(two, NULL);
    pthread_barrier_destroy(&barrier);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "deadlock") && strcmp(argv[1], "prevent")))
        return fprintf(stderr, "usage: %s deadlock|prevent\n", argv[0]), 2;
    if (strcmp(argv[1], "prevent") == 0) return run_threads(1);
    pid_t child = fork();
    if (child < 0) return perror("fork"), 1;
    if (child == 0) _exit(run_threads(0));
    for (int i = 0; i < 20; ++i) {
        int status;
        if (waitpid(child, &status, WNOHANG) == child) {
            fprintf(stderr, "child exited unexpectedly; deadlock not observed\n"); return 1;
        }
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 50000000};
        nanosleep(&delay, NULL);
    }
    puts("deadlock observed: both threads held one mutex and waited for the other");
    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    return 0;
}
