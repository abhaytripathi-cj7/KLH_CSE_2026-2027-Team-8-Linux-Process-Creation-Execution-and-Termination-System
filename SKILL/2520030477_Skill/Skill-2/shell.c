#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char input[100];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        }

        if (strlen(input) == 0) {
            continue;
        }

        system(input);
    }

    return 0;
}
