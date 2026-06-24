#include <stdio.h>

#include "json.h"
#include "stats.h"

int pkg_stats(void)
{
    PackageDB db;

    printf("TX Package Manager Statistics\n");
    printf("=============================\n\n");

    if (!json_load(&db)) {
        printf("Unable to load package database.\n");
        return 1;
    }

    printf("Installed packages : %d\n", db.count);
    printf("Database capacity  : %d\n", MAX_PACKAGES);

    if (db.count == 0)
        printf("Status             : Empty\n");
    else
        printf("Status             : Active\n");

    return 0;
}
