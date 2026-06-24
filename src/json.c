#include <stdio.h>
#include <string.h>

#include "json.h"

int json_load(PackageDB *db)
{
    FILE *fp;
    char line[256];

    if (!db)
        return 0;

    memset(db, 0, sizeof(PackageDB));

    fp = fopen(
        "/data/data/com.termux/files/home/.tx/var/lib/txpkg/installed.json",
        "r"
    );

    if (!fp)
        return 1;

    while (fgets(line, sizeof(line), fp)) {

        if (strstr(line, "\"name\"")) {

            char name[64] = "";
            char version[32] = "";
            int release = 0;

            sscanf(
                line,
                " {\"name\":\"%63[^\"]\",\"version\":\"%31[^\"]\",\"release\":%d}",
                name,
                version,
                &release
            );

            strcpy(db->packages[db->count].name, name);
            strcpy(db->packages[db->count].version, version);
            db->packages[db->count].release = release;

            db->count++;
        }
    }

    fclose(fp);

    return 1;
}

int json_save(PackageDB *db)
{
    FILE *fp;
    int i;

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

    for (i = 0; i < db->count; i++) {

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

    if (json_find_package(&db, "busybox") < 0) {

        if (db.count < MAX_PACKAGES) {

            strcpy(db.packages[db.count].name, "busybox");
            strcpy(db.packages[db.count].version, "1.37.0");
            db.packages[db.count].release = 1;

            db.count++;
        }
    }

    json_save(&db);

    if (json_find_package(&db, "busybox") >= 0)
        printf("Package found in native database.\n");
    else
        printf("Package not found.\n");

    printf("Packages loaded: %d\n", db.count);
    printf("Native JSON database written successfully.\n");
}

int json_remove_package(
    PackageDB *db,
    const char *name)
{
    int index;
    int i;

    if (!db || !name)
        return 0;

    index = json_find_package(db, name);

    if (index < 0)
        return 0;

    for (i = index; i < db->count - 1; i++) {

        db->packages[i] = db->packages[i + 1];
    }

    db->count--;

    return 1;
}
