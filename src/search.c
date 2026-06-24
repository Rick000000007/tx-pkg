#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"

int pkg_search(int argc, char *argv[])
{
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
        "grep -A 5 \"%s\" ~/.cache/tx-pkg/Packages.json",
        argv[2]
    );

    system(command);

    return 0;
}
