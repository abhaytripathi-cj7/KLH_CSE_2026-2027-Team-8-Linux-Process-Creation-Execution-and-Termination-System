#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char input[200];

    while (1) {
        printf("parser> ");
        fgets(input, sizeof(input), stdin);

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0)
            break;

        if (strlen(input) == 0) {
            printf("Empty command\n");
            continue;
        }

        if (input[0] == '|' ||
            input[strlen(input)-1] == '|') {
            printf("Syntax Error\n");
            continue;
        }

        char *token = strtok(input, " ");

        printf("\nTokens:\n");

        while (token != NULL) {
            if (strcmp(token, "|") == 0)
                printf("PIPE\n");
            else
                printf("WORD: %s\n", token);

            token = strtok(NULL, " ");
        }

        printf("Parsing successful\n\n");
    }

    return 0;
}
