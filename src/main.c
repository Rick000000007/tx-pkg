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

    if (strcmp(argv[1], "search") == 0) {
        printf("Search is not implemented yet.\n");
        return 0;
    }

    if (strcmp(argv[1], "info") == 0) {
        printf("Info is not implemented yet.\n");
        return 0;
    }

    if (strcmp(argv[1], "install") == 0) {
        printf("Install is not implemented yet.\n");
        return 0;
    }

    if (strcmp(argv[1], "remove") == 0) {
        printf("Remove is not implemented yet.\n");
        return 0;
    }

    if (strcmp(argv[1], "upgrade") == 0) {
        printf("Upgrade is not implemented yet.\n");
        return 0;
    }

    printf("Unknown command: %s\n", argv[1]);
    return 1;
}
