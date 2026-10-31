#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* JSON frames contain escaped newlines. Each frame fits PIPE_BUF so writes
 * are atomic. stdin/stdout are supervisor control pipes; the two named FIFOs
 * are the actual peer-to-peer message transport. */
#define CAPACITY 8192
static volatile sig_atomic_t running = 1;
static void stop_peer(int sig) { (void)sig; running = 0; }

static void emit(const char *type, const char *payload) {
    printf("{\"type\":\"%s\",\"pid\":%ld,\"payload\":%s}\n",
           type, (long)getpid(), payload);
    fflush(stdout);
}

static int consume(int fd, int outgoing, int is_command,
                   char *buffer, size_t *used) {
    ssize_t count = read(fd, buffer + *used, CAPACITY - 1 - *used);
    if (count < 0) return (errno == EAGAIN || errno == EINTR) ? 0 : -1;
    if (count == 0) return is_command ? -1 : 0;
    *used += (size_t)count;
    buffer[*used] = '\0';
    char *begin = buffer;
    char *newline;
    while ((newline = strchr(begin, '\n')) != NULL) {
        size_t length = (size_t)(newline - begin) + 1;
        if (is_command) {
            long limit = fpathconf(outgoing, _PC_PIPE_BUF);
            ssize_t written = -1;
            if (limit > 0 && length <= (size_t)limit) {
                do { written = write(outgoing, begin, length); }
                while (written < 0 && errno == EINTR && running);
            }
            *newline = '\0';
            emit(written == (ssize_t)length ? "sent" : "failed", begin);
        } else {
            *newline = '\0';
            emit("received", begin);
        }
        begin = newline + 1;
    }
    size_t remaining = *used - (size_t)(begin - buffer);
    memmove(buffer, begin, remaining);
    *used = remaining;
    return remaining >= CAPACITY - 1 ? -1 : 0;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: pipetalk_peer NAME INCOMING_FIFO OUTGOING_FIFO\n");
        return 2;
    }
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = stop_peer;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    signal(SIGPIPE, SIG_IGN);
    /* Linux O_RDWR avoids startup ordering deadlocks. poll() drives reads. */
    int incoming = open(argv[2], O_RDWR | O_NONBLOCK);
    int outgoing = open(argv[3], O_RDWR | O_NONBLOCK);
    if (incoming < 0 || outgoing < 0) {
        perror("open FIFO");
        if (incoming >= 0) close(incoming);
        if (outgoing >= 0) close(outgoing);
        return 1;
    }
    emit("ready", "{}");
    struct pollfd descriptors[2] = {
        {STDIN_FILENO, POLLIN, 0}, {incoming, POLLIN, 0}
    };
    char command[CAPACITY], received[CAPACITY];
    size_t command_used = 0, received_used = 0;
    while (running) {
        int result = poll(descriptors, 2, 500);
        if (result < 0) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }
        if (descriptors[0].revents & POLLIN) {
            if (consume(STDIN_FILENO, outgoing, 1, command, &command_used) < 0) break;
        }
        if (descriptors[1].revents & POLLIN) {
            if (consume(incoming, outgoing, 0, received, &received_used) < 0) break;
        }
        if (descriptors[0].revents & (POLLHUP | POLLERR | POLLNVAL)) break;
    }
    close(incoming);
    close(outgoing);
    return 0;
}
