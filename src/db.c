#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "json.h"

int db_init(void)
{
    system("mkdir -p ~/.tx/var/lib/txpkg");

    system(
        "if [ ! -f ~/.tx/var/lib/txpkg/installed.json ]; then "
        "printf '{\n"
        "  \"packages\": []\n"
        "}\n' > ~/.tx/var/lib/txpkg/installed.json; "
        "fi"
    );

    return 1;
}

int db_add_package(const char *name)
{
    PackageDB db;

    if (!json_load(&db))
        return 0;

    if (json_find_package(&db, name) >= 0)
        return 1;

    if (db.count >= MAX_PACKAGES)
        return 0;

    strcpy(db.packages[db.count].name, name);
    strcpy(db.packages[db.count].version, "1.37.0");
    db.packages[db.count].release = 1;

    db.count++;

    return json_save(&db);
}

int db_remove_package(const char *name)
{
    PackageDB db;

    if (!json_load(&db))
        return 0;

    if (!json_remove_package(&db, name))
        return 0;

    return json_save(&db);
}

int db_is_installed(const char *name)
{
    PackageDB db;

    if (!json_load(&db))
        return 0;

    if (json_find_package(&db, name) >= 0)
        return 1;

    return 0;
}
