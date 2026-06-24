#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "remove.h"
#include "db.h"

int pkg_remove(int argc, char *argv[])
{
    printf("TX Package Manager\n");
    printf("==================\n\n");

    if (argc < 3) {
        printf("Usage: pkg remove <package>\n");
        return 1;
    }

    printf("Removing package: %s\n\n", argv[2]);

    db_remove_package(argv[2]);

    printf("Package removed successfully.\n");

    return 0;
}
