#include <stdio.h>
#include <string.h>

/* Outside quotes, a backslash preserves the following character.
   A backslash followed by a newline is a line continuation. */
int parse_escapes(const char *input, char *output, size_t capacity)
{
    size_t used = 0;

    for (size_t i = 0; input[i]; i++) {
        if (input[i] == '\\') {
            i++;
            if (!input[i])
                return 0;
            if (input[i] == '\n')
                continue;
        }

        if (used + 1 >= capacity)
            return 0;
        output[used++] = input[i];
    }

    output[used] = '\0';
    return 1;
}

int main(void)
{
    char output[256];
    const char *tests[] = {
        "Adi\\ Reddy",
        "Price:\\$100",
        "hello\\ world\\!\\ \\&",
        "Backslash:\\\\",
        "unfinished\\"
    };

    puts("Adi Reddy - Experiment 6: Escape Sequences");
    printf("Name:\tAdi Reddy\n");
    printf("Double quotes: \"Adi Reddy\"\n");
    printf("Single quotes: 'Adi Reddy'\n");
    printf("Backslash: \\\n");

    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        printf("\nInput: %s\n", tests[i]);
        if (parse_escapes(tests[i], output, sizeof output))
            printf("Parsed: [%s]\n", output);
        else
            puts("Invalid escape sequence.");
    }

    return 0;
}
