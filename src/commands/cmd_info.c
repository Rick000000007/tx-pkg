/*
 * TX Package Manager v1.0 - Info Command
 * Displays detailed information about a package.
 */

#include "cmd_info.h"
#include "../config.h"
#include "../repository.h"
#include "../database.h"
#include "../error.h"
#include <stdio.h>

int cmd_info(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    if (argc < 3) {
        printf("Usage: pkg info <package>\n");
        printf("\nShow detailed information about a package.\n");
        return 1;
    }

    const char *pkg_name = argv[2];

    /* Try installed database first */
    tx_db_t db;
    char db_path[TX_MAX_PATH];
    snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

    if (tx_db_open(&db, db_path) == TX_OK) {
        tx_installed_pkg_t pkg;
        if (tx_db_get_package(&db, pkg_name, &pkg) == TX_OK) {
            printf("Package:     %s\n", pkg.name);
            printf("Version:     %s\n", pkg.version.upstream);
            if (pkg.version.release[0])
                printf("Release:     %s\n", pkg.version.release);
            printf("Status:      installed\n");
            printf("Architecture: %s\n", pkg.architecture);
            printf("Repository:  %s\n", pkg.repository);
            printf("Channel:     %s\n", pkg.channel);
            printf("Essential:   %s\n", pkg.is_essential ? "yes" : "no");
            printf("Held:        %s\n", pkg.is_held ? "yes" : "no");
            printf("Auto:        %s\n", pkg.is_auto ? "yes" : "no");

            if (pkg.description[0])
                printf("\nDescription:\n  %s\n", pkg.description);

            tx_db_close(&db);
            return 0;
        }
        tx_db_close(&db);
    }

    /* Try repositories */
    tx_repo_list_t list;
    if (tx_repo_list_init(&list) == TX_OK) {
        tx_pkg_entry_t *pkg = tx_repo_find_package(&list, pkg_name, NULL);
        if (pkg) {
            printf("Package:      %s\n", pkg->name);
            printf("Version:      %s\n", pkg->version.upstream);
            printf("Architecture: %s\n", pkg->architecture);
            printf("Category:     %s\n", pkg->category);
            printf("Repository:   %s\n", pkg->repository);
            printf("Channel:      %s\n", pkg->channel);
            printf("Size:         %zu bytes\n", pkg->size);
            printf("Installed:    %zu bytes\n", pkg->installed_size);

            if (pkg->depends[0])
                printf("\nDepends:      %s\n", pkg->depends);
            if (pkg->provides[0])
                printf("Provides:     %s\n", pkg->provides);
            if (pkg->conflicts[0])
                printf("Conflicts:    %s\n", pkg->conflicts);

            if (pkg->description[0])
                printf("\nDescription:\n  %s\n", pkg->description);

            free(pkg);
            tx_repo_list_free(&list);
            return 0;
        }
        tx_repo_list_free(&list);
    }

    printf("Package '%s' not found.\n", pkg_name);
    printf("Run 'pkg search %s' to find similar packages.\n", pkg_name);
    return 1;
}
