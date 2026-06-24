#include <stdio.h>

#include "list.h"
#include "json.h"

int pkg_list(void)
{
    PackageDB db;
    int i;

    printf("TX Package Manager\n");
    printf("==================\n\n");

    if (!json_load(&db)) {
        printf("Unable to load package database.\n");
        return 1;
    }

    printf("Installed packages\n");
    printf("------------------\n\n");

    if (db.count == 0) {
        printf("(none)\n");
        return 0;
    }

    for (i = 0; i < db.count; i++) {

        printf(
            "%-20s %s-%d\n",
            db.packages[i].name,
            db.packages[i].version,
            db.packages[i].release
        );
    }

    return 0;
}
