/*
 * TX Package Manager v1.0 - Remove Command
 * Removes an installed package, respecting reverse dependencies.
 */

#include "cmd_remove.h"
#include "../config.h"
#include "../database.h"
#include "../solver.h"
#include "../lock.h"
#include "../error.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int cmd_remove(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    if (argc < 3) {
        printf("Usage: pkg remove <package>\n");
        printf("\nRemove an installed package.\n");
        return 1;
    }

    const char *pkg_name = argv[2];
    bool force = false;

    /* Check for --force */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            force = true;
            break;
        }
    }

    /* Acquire lock */
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

    /* Check if installed */
    if (!tx_db_is_installed(&db, pkg_name)) {
        printf("Package '%s' is not installed.\n", pkg_name);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 1;
    }

    /* Check reverse dependencies */
    char **revdeps = NULL;
    size_t rev_count = 0;
    tx_db_get_reverse_deps(&db, pkg_name, &revdeps, &rev_count);

    if (rev_count > 0 && !force) {
        printf("Cannot remove '%s': required by:\n", pkg_name);
        for (size_t i = 0; i < rev_count; i++)
            printf("  - %s\n", revdeps[i]);
        printf("\nUse --force to remove anyway (may break "
               "dependent packages).\n");

        for (size_t i = 0; i < rev_count; i++)
            free(revdeps[i]);
        free(revdeps);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 1;
    }

    for (size_t i = 0; i < rev_count; i++)
        free(revdeps[i]);
    free(revdeps);

    /* Get package info */
    tx_installed_pkg_t pkg;
    tx_db_get_package(&db, pkg_name, &pkg);

    if (pkg.is_essential && !force) {
        printf("Cannot remove essential package '%s'.\n", pkg_name);
        printf("Use --force to override (not recommended).\n");
        tx_installed_pkg_free(&pkg);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 1;
    }

    printf("Removing package: %s\n\n", pkg_name);

    /* Remove files */
    tx_file_entry_t *files = NULL;
    size_t file_count = 0;
    tx_db_package_files(&db, pkg_name, &files, &file_count);

    printf("Removing %zu file(s)...\n", file_count);

    for (size_t i = 0; i < file_count; i++) {
        if (unlink(files[i].path) == 0) {
            TX_DEBUG_LOG("Removed: %s\n", files[i].path);
        }
    }

    free(files);

    /* Unregister from database */
    tx_db_unregister_package(&db, pkg_name);

    printf("\nPackage '%s' removed successfully.\n", pkg_name);

    tx_installed_pkg_free(&pkg);
    tx_db_close(&db);
    tx_lock_release(&lock);

    return 0;
}
