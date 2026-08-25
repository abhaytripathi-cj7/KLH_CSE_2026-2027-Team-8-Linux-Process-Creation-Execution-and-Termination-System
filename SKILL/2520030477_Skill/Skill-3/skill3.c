#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int main() {
    char *history[20];
    int count = 0, pos = 0;

    struct termios old, new;
    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &new);

    while (1) {
        char *cmd = malloc(100);
        int i = 0;
        printf("shell> ");
        fflush(stdout);

        while (1) {
            char c;
            read(0, &c, 1);

            if (c == '\n') {
                cmd[i] = '\0';
                printf("\n");
                break;
            }

            if (c == 27) {
                char a, b;
                read(0, &a, 1);
                read(0, &b, 1);

                if (b == 'A' && count > 0) {
                    if (pos > 0) pos--;
                    printf("\r\033[Kshell> %s", history[pos]);
                    strcpy(cmd, history[pos]);
                    i = strlen(cmd);
                }

                if (b == 'B' && pos < count - 1) {
                    pos++;
                    printf("\r\033[Kshell> %s", history[pos]);
                    strcpy(cmd, history[pos]);
                    i = strlen(cmd);
                }
            }
            else {
                cmd[i++] = c;
                printf("%c", c);
            }
        }

        if (strcmp(cmd, "exit") == 0) {
            free(cmd);
            break;
        }

        history[count] = strdup(cmd);
        count++;
        pos = count;

        system(cmd);

        free(cmd);
    }

    for (int i = 0; i < count; i++)
        free(history[i]);

    tcsetattr(0, TCSANOW, &old);

    return 0;
}
