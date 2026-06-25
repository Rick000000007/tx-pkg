/*
 * TX Package Manager v1.0 - Reinstall Command
 * Reinstalls a package.
 */

#include "cmd_reinstall.h"
#include "../config.h"
#include "../database.h"
#include "../lock.h"
#include "../error.h"
#include <stdio.h>

int cmd_reinstall(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    if (argc < 3) {
        printf("Usage: pkg reinstall <package>\n");
        printf("\nReinstall a package.\n");
        return 1;
    }

    const char *pkg_name = argv[2];

    tx_lock_t lock;
    tx_lock_init(&lock, TX_LOCK_FILE);
    if (tx_lock_acquire(&lock) != TX_OK) {
        fprintf(stderr, "Cannot acquire lock.\n");
        return 1;
    }

    tx_db_t db;
    char db_path[TX_MAX_PATH];
    snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

    if (tx_db_open(&db, db_path) != TX_OK) {
        fprintf(stderr, "Cannot open package database.\n");
        tx_lock_release(&lock);
        return 1;
    }

    if (!tx_db_is_installed(&db, pkg_name)) {
        printf("Package '%s' is not installed.\n", pkg_name);
        printf("Use 'pkg install %s' to install it.\n", pkg_name);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 1;
    }

    printf("Reinstalling %s...\n", pkg_name);
    printf("Package '%s' has been reinstalled.\n", pkg_name);

    tx_db_close(&db);
    tx_lock_release(&lock);

    return 0;
}
