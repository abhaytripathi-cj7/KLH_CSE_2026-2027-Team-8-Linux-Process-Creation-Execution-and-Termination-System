#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

int main() {

    char command[256];

    printf("=== PATH Command Resolver ===\n");

    char *path = getenv("PATH");

    if (path == NULL) {
        printf("Error: PATH environment variable not found.\n");
        return 1;
    }

    printf("\nPATH Variable:\n%s\n", path);

    printf("\nEnter command to locate: ");

    if (scanf("%255s", command) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    char *pathCopy = strdup(path);

    if (pathCopy == NULL) {
        perror("strdup");
        return 1;
    }

    char *directory = strtok(pathCopy, ":");

    char fullPath[PATH_MAX];

    int found = 0;

    printf("\nSearching PATH directories...\n\n");

    while (directory != NULL) {

        printf("Checking: %s\n", directory);

        snprintf(fullPath,
                 sizeof(fullPath),
                 "%s/%s",
                 directory,
                 command);

        struct stat fileInfo;

        if (stat(fullPath, &fileInfo) == 0 &&
            S_ISREG(fileInfo.st_mode) &&
            access(fullPath, X_OK) == 0) {

            printf("\nExecutable found!\n");
            printf("Command : %s\n", command);
            printf("Location: %s\n", fullPath);
            printf("Permission: Executable\n");

            found = 1;
            break;
        }

        directory = strtok(NULL, ":");
    }

    if (!found) {
        printf("\nCommand '%s' was not found in PATH.\n",
               command);
    }

    free(pathCopy);

    return 0;
}
