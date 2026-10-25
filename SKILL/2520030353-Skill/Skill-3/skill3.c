#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY 10
#define BUFFER_SIZE 100

int main(void)
{
    char history[MAX_HISTORY][BUFFER_SIZE];
    char input[BUFFER_SIZE] = "";
    int count = 0, position = 0;

    puts("Adi Reddy - Experiment 3: Command History");

    while (1) {
        puts("\n1. Enter command\n2. Show history\n3. Previous command");
        puts("4. Next command\n5. Clear input buffer\n6. Exit");
        printf("Choice: ");

        if (!fgets(input, sizeof input, stdin))
            break;

        int choice = atoi(input);

        if (choice == 1) {
            printf("Adi Reddy> ");
            if (!fgets(input, sizeof input, stdin))
                break;

            if (!strchr(input, '\n')) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
            }
            input[strcspn(input, "\n")] = '\0';
            if (!input[0])
                continue;

            if (count == MAX_HISTORY) {
                memmove(history, history + 1,
                        (MAX_HISTORY - 1) * sizeof history[0]);
                count--;
            }
            strcpy(history[count++], input);
            position = count;
        } else if (choice == 2) {
            for (int i = 0; i < count; i++)
                printf("%d. %s\n", i + 1, history[i]);
            if (count == 0)
                puts("History is empty.");
        } else if (choice == 3 || choice == 4) {
            if (count == 0) {
                puts("History is empty.");
                continue;
            }

            if (choice == 3 && position > 0)
                position--;
            else if (choice == 4 && position < count)
                position++;

            if (position < count)
                printf("Command: %s\n", history[position]);
            else
                puts("At the new-command position.");
        } else if (choice == 5) {
            memset(input, 0, sizeof input);
            puts("Input buffer cleared.");
        } else if (choice == 6) {
            break;
        } else {
            puts("Invalid choice.");
        }
    }

    return 0;
}
