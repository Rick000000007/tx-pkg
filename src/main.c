#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void usage(void)
{
    printf("TX Package Manager\n");
    printf("==================\n\n");

    printf("Usage:\n");
    printf("  pkg update\n");
    printf("  pkg search <package>\n");
    printf("  pkg info <package>\n");
    printf("  pkg install <package>\n");
    printf("  pkg remove <package>\n");
    printf("  pkg upgrade\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();
        return 0;
    }

    /* pkg update */

    if (strcmp(argv[1], "update") == 0) {

        printf("TX Package Manager\n");
        printf("==================\n\n");

        printf("Updating repository...\n\n");

        system("mkdir -p ~/.cache/tx-pkg");

        system("curl -L -o ~/.cache/tx-pkg/Packages.json https://rick000000007.github.io/tx-packages/repo/stable/Packages.json");

        printf("\nRepository downloaded successfully.\n");
        printf("Cache location:\n");
        printf("~/.cache/tx-pkg/Packages.json\n");

        return 0;
    }

    /* pkg search */

    if (strcmp(argv[1], "search") == 0) {

        printf("TX Package Manager\n");
        printf("==================\n\n");

        if (argc < 3) {
            printf("Usage: pkg search <package>\n");
            return 1;
        }

        printf("Searching for: %s\n\n", argv[2]);

        char command[512];

        snprintf(
            command,
            sizeof(command),
            "grep -i \"%s\" ~/.cache/tx-pkg/Packages.json",
            argv[2]
        );

        system(command);

        return 0;
    }

    /* pkg info */

    if (strcmp(argv[1], "info") == 0) {

        printf("TX Package Manager\n");
        printf("==================\n\n");

        if (argc < 3) {
            printf("Usage: pkg info <package>\n");
            return 1;
        }

        printf("Package information:\n\n");

        char command[512];

        snprintf(
            command,
            sizeof(command),
            "grep -A 5 -i \"\\\"name\\\": \\\"%s\\\"\" ~/.cache/tx-pkg/Packages.json",
            argv[2]
        );

        system(command);

        return 0;
    }

    /* pkg install */

    if (strcmp(argv[1], "install") == 0) {
        printf("Install command is not implemented yet.\n");
        return 0;
    }

    /* pkg remove */

    if (strcmp(argv[1], "remove") == 0) {
        printf("Remove command is not implemented yet.\n");
        return 0;
    }

    /* pkg upgrade */

    if (strcmp(argv[1], "upgrade") == 0) {
        printf("Upgrade command is not implemented yet.\n");
        return 0;
    }

    printf("Unknown command: %s\n", argv[1]);
    printf("Run 'pkg' to see available commands.\n");

    return 1;
}
