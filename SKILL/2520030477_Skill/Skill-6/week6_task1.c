#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_TOKEN 256
#define MAX_TOKENS 100

int main() {

    char input[MAX_INPUT];
    char tokens[MAX_TOKENS][MAX_TOKEN];

    int tokenCount = 0;
    int charIndex = 0;
    int escaped = 0;

    printf("=== Escape Sequence Parser ===\n");
    printf("Enter input: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Input error.\n");
        return 1;
    }

    input[strcspn(input, "\n")] = '\0';

    for (int i = 0; input[i] != '\0'; i++) {

        if (escaped) {

            switch (input[i]) {

                case 'n':
                    tokens[tokenCount][charIndex++] = '\n';
                    break;

                case 't':
                    tokens[tokenCount][charIndex++] = '\t';
                    break;

                case '\\':
                    tokens[tokenCount][charIndex++] = '\\';
                    break;

                case ' ':
                    tokens[tokenCount][charIndex++] = ' ';
                    break;

                default:
                    /* Preserve escaped special symbols */
                    tokens[tokenCount][charIndex++] = input[i];
            }

            escaped = 0;
            continue;
        }

        if (input[i] == '\\') {
            escaped = 1;
            continue;
        }

        if (isspace((unsigned char)input[i])) {

            if (charIndex > 0) {
                tokens[tokenCount][charIndex] = '\0';
                tokenCount++;
                charIndex = 0;
            }

        } else {

            if (charIndex < MAX_TOKEN - 1) {
                tokens[tokenCount][charIndex++] = input[i];
            }
        }
    }

    if (escaped) {
        printf("\nError: Incomplete escape sequence.\n");
        return 1;
    }

    if (charIndex > 0) {
        tokens[tokenCount][charIndex] = '\0';
        tokenCount++;
    }

    printf("\nParser Output\n");
    printf("------------------------\n");

    for (int i = 0; i < tokenCount; i++) {
        printf("Token %d: [%s]\n", i + 1, tokens[i]);
    }

    return 0;
}
