/*
 * TX Package Manager v1.0 - List Command
 * Lists all installed packages.
 */

#include "cmd_list.h"
#include "../config.h"
#include "../database.h"
#include "../error.h"
#include <stdio.h>

int cmd_list(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");
    printf("Installed packages\n");
    printf("------------------\n\n");

    tx_db_t db;
    char db_path[TX_MAX_PATH];
    snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

    if (tx_db_open(&db, db_path) != TX_OK) {
        printf("(none)\n");
        return 0;
    }

    tx_installed_pkg_t *pkgs = NULL;
    size_t count = 0;

    tx_status_t s = tx_db_list_packages(&db, &pkgs, &count);

    if (s == TX_OK && count > 0) {
        printf("%-20s %-15s %-10s\n",
               "Package", "Version", "Status");
        printf("%-20s %-15s %-10s\n",
               "-------", "-------", "------");

        for (size_t i = 0; i < count; i++) {
            const char *status = "installed";
            if (pkgs[i].is_held) status = "held";

            printf("%-20s %-15s %-10s\n",
                   pkgs[i].name,
                   pkgs[i].version.upstream,
                   status);
        }

        printf("\nTotal: %zu package(s)\n", count);
    } else {
        printf("(none)\n");
    }

    free(pkgs);
    tx_db_close(&db);
    return 0;
}
