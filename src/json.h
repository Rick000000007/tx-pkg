#ifndef TX_JSON_H
#define TX_JSON_H

#define MAX_PACKAGES 256

typedef struct {
    char name[64];
    char version[32];
    int release;
} Package;

typedef struct {
    int count;
    Package packages[MAX_PACKAGES];
} PackageDB;

int json_load(PackageDB *db);
int json_save(PackageDB *db);
int json_find_package(
    PackageDB *db,
    const char *name
);

void json_test(void);

#endif
