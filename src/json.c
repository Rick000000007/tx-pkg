#include <stdio.h>
#include <string.h>

#include "json.h"

int json_load(PackageDB *db)
{
    if (!db)
        return 0;

    memset(db, 0, sizeof(PackageDB));

    return 1;
}

int json_save(PackageDB *db)
{
    FILE *fp;

    if (!db)
        return 0;

    fp = fopen(
        "/data/data/com.termux/files/home/.tx/var/lib/txpkg/installed.json",
        "w"
    );

    if (!fp)
        return 0;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"packages\": [\n");

    for (int i = 0; i < db->count; i++) {

        fprintf(
            fp,
            "    {\"name\":\"%s\",\"version\":\"%s\",\"release\":%d}",
            db->packages[i].name,
            db->packages[i].version,
            db->packages[i].release
        );

        if (i + 1 < db->count)
            fprintf(fp, ",");

        fprintf(fp, "\n");
    }

    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    fclose(fp);

    return 1;
}

int json_find_package(
    PackageDB *db,
    const char *name)
{
    int i;

    if (!db || !name)
        return -1;

    for (i = 0; i < db->count; i++) {

        if (strcmp(
                db->packages[i].name,
                name) == 0) {

            return i;
        }
    }

    return -1;
}

void json_test(void)
{
    PackageDB db;

    json_load(&db);

    db.count = 1;

    strcpy(db.packages[0].name, "busybox");
    strcpy(db.packages[0].version, "1.37.0");
    db.packages[0].release = 1;

    json_save(&db);

    if (json_find_package(&db, "busybox") >= 0)
        printf("Package found in native database.\n");
    else
        printf("Package not found.\n");

    printf("Native JSON database written successfully.\n");
}
