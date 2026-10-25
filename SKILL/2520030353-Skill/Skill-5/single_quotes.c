#include <stdio.h>
#include <string.h>

/* Parse one single-quoted token; preserve its contents literally. */
int parse_single(const char *input, char *output, size_t capacity)
{
    size_t length = strlen(input);

    if (length < 2 || input[0] != '\'' || input[length - 1] != '\'')
        return 0;

    if (length - 2 >= capacity)
        return 0;

    for (size_t i = 1; i < length - 1; i++)
        if (input[i] == '\'')
            return 0;

    memcpy(output, input + 1, length - 2);
    output[length - 2] = '\0';
    return 1;
}

int main(void)
{
    const char *name = "Adi Reddy";
    const char *tests[] = {
        "'Adi Reddy'",
        "'Hello $name'",
        "'Spaces   remain'",
        "''",
        "'Unclosed"
    };
    char output[256];

    printf("%s - Experiment 5: Single Quotes\n", name);

    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        printf("\nInput: %s\n", tests[i]);
        if (parse_single(tests[i], output, sizeof output))
            printf("Parsed: [%s]\n", output);
        else
            puts("Invalid quoted token.");
    }

    return 0;
}
