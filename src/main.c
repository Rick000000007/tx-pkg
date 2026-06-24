#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "db.h"
#include "list.h"
#include "install.h"
#include "search.h"
#include "info.h"

static void usage(void)
{
    printf("TX Package Manager\n");
    printf("==================\n\n");

    printf("Usage:\n");
    printf("  pkg update\n");
    printf("  pkg search <package>\n");
    printf("  pkg info <package>\n");
    printf("  pkg install <package>\n");
    printf("  pkg list\n");
    printf("  pkg remove <package>\n");
    printf("  pkg upgrade\n");
}

int main(int argc, char *argv[])
{
    db_init();
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
        return pkg_search(argc, argv);
    }
        if (strcmp(argv[1], "info") == 0) {
        return pkg_info(argc, argv);
    }
        if (strcmp(argv[1], "install") == 0) {
        return pkg_install(argc, argv);
    }
        if (strcmp(argv[1], "list") == 0) {
        return pkg_list();
    }
	if (strcmp(argv[1], "remove") == 0) {
        printf("Remove command is not implemented yet.\n");
        return 0;
    }

    if (strcmp(argv[1], "upgrade") == 0) {
        printf("Upgrade command is not implemented yet.\n");
        return 0;
    }

    printf("Unknown command: %s\n", argv[1]);
    printf("Run 'pkg' to see available commands.\n");

    return 1;
}
