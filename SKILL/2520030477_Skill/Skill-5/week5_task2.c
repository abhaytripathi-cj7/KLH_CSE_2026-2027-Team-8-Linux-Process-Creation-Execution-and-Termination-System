#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_TOKENS 100
#define MAX_TOKEN_LEN 512

int main() {

    char input[MAX_INPUT];
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];

    int tokenCount = 0;
    int charIndex = 0;
    int inDoubleQuote = 0;

    printf("=== Double Quote and Variable Expansion Parser ===\n");
    printf("Enter command: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Input error.\n");
        return 1;
    }

    input[strcspn(input, "\n")] = '\0';

    for (int i = 0; input[i] != '\0'; i++) {

        if (input[i] == '"') {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }

        /* Variable expansion */
        if (input[i] == '$') {

            char variable[100];
            int v = 0;

            i++;

            while (input[i] != '\0' &&
                   (isalnum((unsigned char)input[i]) ||
                    input[i] == '_')) {

                variable[v++] = input[i++];
            }

            variable[v] = '\0';

            i--;

            if (v > 0) {
                char *value = getenv(variable);

                if (value != NULL) {
                    for (int j = 0;
                         value[j] != '\0' &&
                         charIndex < MAX_TOKEN_LEN - 1;
                         j++) {

                        tokens[tokenCount][charIndex++] = value[j];
                    }
                }
            }

            continue;
        }

        if (isspace((unsigned char)input[i]) && !inDoubleQuote) {

            if (charIndex > 0) {
                tokens[tokenCount][charIndex] = '\0';
                tokenCount++;
                charIndex = 0;
            }

        } else {

            if (charIndex < MAX_TOKEN_LEN - 1) {
                tokens[tokenCount][charIndex++] = input[i];
            }
        }
    }

    if (inDoubleQuote) {
        printf("\nError: Unmatched double quote.\n");
        return 1;
    }

    if (charIndex > 0) {
        tokens[tokenCount][charIndex] = '\0';
        tokenCount++;
    }

    printf("\nParsed Output\n");
    printf("--------------------------\n");

    for (int i = 0; i < tokenCount; i++) {
        printf("Token %d: [%s]\n", i + 1, tokens[i]);
    }

    return 0;
}
