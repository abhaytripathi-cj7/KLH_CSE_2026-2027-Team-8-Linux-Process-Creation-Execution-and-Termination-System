#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Educational simulation: no bank connection or real payment. */
static void fail(const char *what) { perror(what); exit(2); }

static void snapshot(const char *stage, pid_t pid) {
    char path[64], line[256];
    snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    FILE *fp = fopen(path, "r");
    printf("\n========== %s ==========\n", stage);
    if (!fp) { printf("PID=%ld: process record no longer exists.\n", (long)pid); return; }
    while (fgets(line, sizeof(line), fp)) {
        if (!strncmp(line, "Name:", 5) || !strncmp(line, "State:", 6) ||
            !strncmp(line, "Pid:", 4) || !strncmp(line, "PPid:", 5) ||
            !strncmp(line, "Uid:", 4) || !strncmp(line, "Threads:", 8) ||
            !strncmp(line, "VmRSS:", 6)) fputs(line, stdout);
    }
    fclose(fp);
    errno = 0;
    int nice_value = getpriority(PRIO_PROCESS, (id_t)pid);
    printf("PGID=%ld | SID=%ld", (long)getpgid(pid), (long)getsid(pid));
    if (!errno) printf(" | nice=%d", nice_value);
    puts("");
}

static char state_of(pid_t pid) {
    char path[64], line[256], state = '?';
    snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return state;
    while (fgets(line, sizeof(line), fp))
        if (sscanf(line, "State: %c", &state) == 1) break;
    fclose(fp);
    return state;
}

static void observe_sleep(pid_t pid) {
    const struct timespec pause = {0, 10000000L};
    for (int i = 0; i < 100; ++i) {
        if (state_of(pid) == 'S') return;
        nanosleep(&pause, NULL);
    }
}

static int read_line(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) return 0;
    if (!strchr(buf, '\n') && !feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        buf[0] = '\0'; /* Treat overlong input as invalid. */
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

static int worker(int input_fd, int ready_fd, const char *mode, const char *amount) {
    snapshot("EXECUTION: same child PID after exec()", getpid());
    printf("Payment worker: mode=%s | amount=%s (demo units)\n", mode, amount);
    puts("WAITING FOR OTP: worker blocks in read(pipe). Demo OTP: 123456");
    if (write(ready_fd, "R", 1) != 1) fail("ready write");
    close(ready_fd);
    char otp[32] = {0};
    size_t used = 0;
    while (used < sizeof(otp) - 1) {
        char c;
        ssize_t n = read(input_fd, &c, 1);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) fail("OTP read");
        if (n == 0) { close(input_fd); return 3; }
        if (c == '\n') break;
        otp[used++] = c;
    }
    close(input_fd);
    snapshot("OTP RECEIVED: worker resumes execution", getpid());
    if (!strcmp(otp, "123456")) {
        puts("PAYMENT SUCCESSFUL: exit(0), normal process exit.");
        return 0;
    }
    puts("PAYMENT FAILED: incorrect OTP, exit(1), normal nonzero exit.");
    return 1;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 6 && !strcmp(argv[1], "--worker"))
        return worker(atoi(argv[2]), atoi(argv[3]), argv[4], argv[5]);

    const char *demo = NULL;
    if (argc == 3 && !strcmp(argv[1], "--demo") &&
        (!strcmp(argv[2], "success") || !strcmp(argv[2], "decline") ||
         !strcmp(argv[2], "wrong-otp"))) demo = argv[2];
    else if (argc != 1) {
        fprintf(stderr, "Usage: %s [--demo success|decline|wrong-otp]\n", argv[0]);
        return 2;
    }

    puts("LINUX PROCESS LIFECYCLE THROUGH A PAYMENT\nEducational simulation only.");
    snapshot("PARENT: payment application", getpid());
    char selection[32], amount[32] = "500";
    const char *mode = "UPI";
    if (!demo) {
        puts("\nChoose payment mode: 1. UPI   2. Card   3. Net banking");
        printf("Mode: ");
        if (!read_line(selection, sizeof(selection))) return 2;
        if (!strcmp(selection, "2")) mode = "Card";
        else if (!strcmp(selection, "3")) mode = "Net banking";
        else if (strcmp(selection, "1")) { puts("Invalid mode."); return 2; }
        printf("Amount (whole demo units, 1..1000000): ");
        if (!read_line(amount, sizeof(amount))) return 2;
        char *end;
        errno = 0;
        long value = strtol(amount, &end, 10);
        if (errno || end == amount || *end || value < 1 || value > 1000000) {
            puts("Invalid amount."); return 2;
        }
        printf("Enter pay to create the payment process: ");
        if (!read_line(selection, sizeof(selection)) || strcmp(selection, "pay")) {
            puts("Payment not started. No payment child created."); return 2;
        }
    }

    int otp_pipe[2], ready_pipe[2];
    if (pipe(otp_pipe) == -1 || pipe(ready_pipe) == -1) fail("pipe");
    printf("\nPAY: fork() creates a new payment process. Parent PID=%ld\n", (long)getpid());
    pid_t child = fork();
    if (child == -1) fail("fork");
    if (child == 0) {
        close(otp_pipe[1]); close(ready_pipe[0]);
        snapshot("CREATION: child before exec()", getpid());
        char input_fd[24], ready_fd[24];
        snprintf(input_fd, sizeof(input_fd), "%d", otp_pipe[0]);
        snprintf(ready_fd, sizeof(ready_fd), "%d", ready_pipe[1]);
        puts("exec(): replace child program image with payment worker; PID stays the same.");
        execl("/proc/self/exe", "payment", "--worker", input_fd, ready_fd, mode, amount, (char *)NULL);
        perror("exec"); _exit(127);
    }
    close(otp_pipe[0]); close(ready_pipe[1]);
    char ready;
    ssize_t count;
    do { count = read(ready_pipe[0], &ready, 1); } while (count < 0 && errno == EINTR);
    close(ready_pipe[0]);
    if (count == 1 && ready == 'R') {
        observe_sleep(child);
        snapshot("OTP WAIT: parent inspects payment child", child);
        printf("Relationship: parent PID=%ld -> child PID=%ld; child PPID=%ld\n",
               (long)getpid(), (long)child, (long)getpid());
        char otp[32];
        int decline = 0;
        if (demo) {
            decline = !strcmp(demo, "decline");
            strcpy(otp, !strcmp(demo, "wrong-otp") ? "000000" : "123456");
            printf("DEMO ACTION: %s\n", demo);
        } else {
            printf("Enter OTP (123456), or decline to terminate: ");
            if (!read_line(otp, sizeof(otp))) decline = 1;
            else decline = !strcmp(otp, "decline");
        }
        if (decline) {
            printf("DECLINED: kill(%ld, SIGTERM); signal=%d\n", (long)child, SIGTERM);
            if (kill(child, SIGTERM) == -1) perror("kill");
        } else {
            /* Ignore SIGPIPE so a dead worker cannot kill the parent. */
            signal(SIGPIPE, SIG_IGN);
            size_t len = strlen(otp);
            otp[len++] = '\n';
            size_t sent = 0;
            while (sent < len) {
                ssize_t n = write(otp_pipe[1], otp + sent, len - sent);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) { perror("OTP write"); break; }
                sent += (size_t)n;
            }
        }
    } else puts("Worker failed before reporting ready.");
    close(otp_pipe[1]);

    puts("\nPARENT WAIT: waitid(WEXITED | WNOWAIT) waits for child termination.");
    siginfo_t info;
    int result;
    do { result = waitid(P_PID, (id_t)child, &info, WEXITED | WNOWAIT); }
    while (result == -1 && errno == EINTR);
    if (result == -1) fail("waitid");
    snapshot("TERMINATED: zombie before parent reaps it", child);
    int status;
    pid_t reaped;
    do { reaped = waitpid(child, &status, 0); } while (reaped == -1 && errno == EINTR);
    if (reaped == -1) fail("waitpid");
    printf("\nREAPED: waitpid() returned PID=%ld; parent PID=%ld; raw status=%d\n",
           (long)reaped, (long)getpid(), status);
    if (WIFEXITED(status))
        printf("WIFEXITED=true | WEXITSTATUS=%d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("WIFSIGNALED=true | WTERMSIG=%d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
    puts("Child reaped: its process-table entry is released; parent remains alive.");
    /* The parent exits normally even when the simulated payment is declined. */
    return 0;
}
