#include <stdio.h>
#include <stdlib.h>

struct Node {
    int data;
    struct Node *next;
};

int main(void)
{
    int size = 2, new_size, nodes;
    int *array = malloc((size_t)size * sizeof *array);
    struct Node *head = NULL;
    int result = 1;

    puts("Adi Reddy - Experiment 4: Dynamic Memory");

    if (!array) {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < size; i++) {
        printf("Enter value %d: ", i + 1);
        if (scanf("%d", &array[i]) != 1)
            goto cleanup;
    }

    printf("Enter new array size (1-10000): ");
    if (scanf("%d", &new_size) != 1 || new_size < 1 || new_size > 10000)
        goto cleanup;

    int *temp = realloc(array, (size_t)new_size * sizeof *array);
    if (!temp) {
        perror("realloc");
        goto cleanup;
    }
    array = temp;

    for (int i = size; i < new_size; i++) {
        printf("Enter value %d: ", i + 1);
        if (scanf("%d", &array[i]) != 1)
            goto cleanup;
    }

    printf("Array: ");
    for (int i = 0; i < new_size; i++)
        printf("%d ", array[i]);
    puts("");

    printf("Number of linked-list nodes (0-10000): ");
    if (scanf("%d", &nodes) != 1 || nodes < 0 || nodes > 10000)
        goto cleanup;

    for (int i = 0; i < nodes; i++) {
        struct Node *node = malloc(sizeof *node);
        if (!node) {
            perror("malloc");
            goto cleanup;
        }

        printf("Enter node %d: ", i + 1);
        if (scanf("%d", &node->data) != 1) {
            free(node);
            goto cleanup;
        }

        node->next = head;
        head = node;
    }

    printf("Linked list: ");
    for (struct Node *node = head; node; node = node->next)
        printf("%d -> ", node->data);
    puts("NULL");
    result = 0;

cleanup:
    free(array);
    while (head) {
        struct Node *next = head->next;
        free(head);
        head = next;
    }

    puts(result == 0 ? "All memory released." : "Invalid input or allocation failure.");
    return result;
}
