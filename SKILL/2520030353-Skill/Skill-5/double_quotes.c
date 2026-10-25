#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Demonstration parser: expands $name and preserves spaces.
   This is not a complete shell parser. */
int parse_double(const char *input, const char *name,
                 char *output, size_t capacity)
{
    size_t length = strlen(input), used = 0;

    if (length < 2 || input[0] != '"' || input[length - 1] != '"')
        return 0;

    for (size_t i = 1; i < length - 1;) {
        if (input[i] == '"')
            return 0;

        if (strncmp(input + i, "$name", 5) == 0 &&
            !isalnum((unsigned char)input[i + 5]) &&
            input[i + 5] != '_') {
            size_t n = strlen(name);
            if (used + n >= capacity)
                return 0;
            memcpy(output + used, name, n);
            used += n;
            i += 5;
        } else {
            if (used + 1 >= capacity)
                return 0;
            output[used++] = input[i++];
        }
    }

    output[used] = '\0';
    return 1;
}

int main(void)
{
    const char *name = "Adi Reddy";
    const char *tests[] = {
        "\"Hello $name\"",
        "\"Spaces   remain\"",
        "\"Hello '$name'\"",
        "\"\"",
        "\"Unclosed"
    };
    char output[256];

    printf("%s - Experiment 5: Double Quotes\n", name);

    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        printf("\nInput: %s\n", tests[i]);
        if (parse_double(tests[i], name, output, sizeof output))
            printf("Parsed: [%s]\n", output);
        else
            puts("Invalid quoted token.");
    }

    return 0;
}
