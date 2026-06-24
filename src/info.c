#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "info.h"

int pkg_info(int argc, char *argv[])
{
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
        "grep -A 10 \"%s\" ~/.cache/tx-pkg/Packages.json",
        argv[2]
    );

    system(command);

    return 0;
}
