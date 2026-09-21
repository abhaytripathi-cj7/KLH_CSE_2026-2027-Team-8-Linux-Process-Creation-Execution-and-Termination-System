#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define MAX_TOKENS 100
#define MAX_TOKEN_LEN 256

int main() {
    char input[MAX_INPUT];
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];

    int tokenCount = 0;
    int charIndex = 0;
    int inSingleQuote = 0;

    printf("=== Single Quote Parser ===\n");
    printf("Enter command: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error reading input.\n");
        return 1;
    }

    input[strcspn(input, "\n")] = '\0';

    for (int i = 0; input[i] != '\0'; i++) {

        if (input[i] == '\'') {
            inSingleQuote = !inSingleQuote;
            continue;
        }

        if (isspace((unsigned char)input[i]) && !inSingleQuote) {

            if (charIndex > 0) {
                tokens[tokenCount][charIndex] = '\0';
                tokenCount++;
                charIndex = 0;
            }

        } else {

            if (tokenCount < MAX_TOKENS &&
                charIndex < MAX_TOKEN_LEN - 1) {

                tokens[tokenCount][charIndex++] = input[i];
            }
        }
    }

    if (inSingleQuote) {
        printf("\nError: Unmatched single quote detected.\n");
        return 1;
    }

    if (charIndex > 0) {
        tokens[tokenCount][charIndex] = '\0';
        tokenCount++;
    }

    printf("\nParsing successful.\n");
    printf("Total tokens: %d\n\n", tokenCount);

    for (int i = 0; i < tokenCount; i++) {
        printf("Token %d: [%s]\n", i + 1, tokens[i]);
    }

    return 0;
}
