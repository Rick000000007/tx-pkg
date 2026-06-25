/*
 * TX Package Manager v1.0 - Install Command
 * Installs a package with full dependency resolution.
 */

#include "cmd_install.h"
#include "../config.h"
#include "../database.h"
#include "../repository.h"
#include "../solver.h"
#include "../cache.h"
#include "../lock.h"
#include "../error.h"
#include <stdio.h>

int cmd_install(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    if (argc < 3) {
        printf("Usage: pkg install <package>\n");
        printf("\nInstall a package and its dependencies.\n");
        return 1;
    }

    const char *pkg_name = argv[2];

    /* Acquire lock */
    tx_lock_t lock;
    tx_lock_init(&lock, TX_LOCK_FILE);
    if (tx_lock_acquire(&lock) != TX_OK) {
        fprintf(stderr, "Cannot acquire lock. Another package "
                "operation is in progress.\n");
        return 1;
    }

    /* Open database */
    tx_db_t db;
    char db_path[TX_MAX_PATH];
    snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

    if (tx_db_open(&db, db_path) != TX_OK) {
        fprintf(stderr, "Cannot open package database.\n");
        tx_lock_release(&lock);
        return 1;
    }

    /* Check if already installed */
    if (tx_db_is_installed(&db, pkg_name)) {
        printf("Package '%s' is already installed.\n", pkg_name);
        printf("Use 'pkg upgrade %s' to update, or "
               "'pkg reinstall %s' to reinstall.\n",
               pkg_name, pkg_name);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 0;
    }

    /* Load repositories */
    tx_repo_list_t list;
    tx_repo_list_init(&list);

    /* Load installed packages for solver context */
    tx_installed_pkg_t *installed = NULL;
    size_t installed_count = 0;
    tx_db_list_packages(&db, &installed, &installed_count);

    /* Get available packages */
    tx_pkg_entry_t *available = NULL;
    size_t available_count = 0;
    tx_repo_get_all_packages(&list, &available, &available_count);

    /* Setup solver context */
    tx_solver_context_t ctx = {
        .available = available,
        .available_count = available_count,
        .installed = installed,
        .installed_count = installed_count,
        .db_handle = &db,
    };

    /* Solve dependencies */
    printf("Resolving dependencies for '%s'...\n", pkg_name);

    tx_solver_solution_t solution;
    tx_status_t s = tx_solve_install(&ctx, pkg_name, &solution);

    if (s != TX_OK || !solution.is_valid) {
        printf("\nCannot install '%s': %s\n",
               pkg_name, solution.error_message);

        free(available);
        free(installed);
        tx_solution_free(&solution);
        tx_repo_list_free(&list);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 1;
    }

    /* Show plan */
    tx_solution_print(&solution);

    /* Confirm */
    printf("Proceed with installation? [Y/n] ");
    fflush(stdout);

    char response[16];
    if (fgets(response, sizeof(response), stdin) != NULL) {
        if (response[0] == 'n' || response[0] == 'N') {
            printf("Installation cancelled.\n");

            free(available);
            free(installed);
            tx_solution_free(&solution);
            tx_repo_list_free(&list);
            tx_db_close(&db);
            tx_lock_release(&lock);
            return 0;
        }
    }

    printf("\nInstalling...\n");

    /* For each action, download and install */
    tx_solver_action_t *action = solution.actions;
    while (action) {
        if (action->op == TX_OP_INSTALL) {
            printf("  Installing %s...\n", action->package_name);

            /* Find the package in available */
            for (size_t i = 0; i < available_count; i++) {
                if (strcmp(available[i].name, action->package_name) == 0) {
                    /* Download */
                    char cache_file[TX_MAX_PATH];
                    tx_cache_t cache;
                    tx_cache_init(&cache, TX_PKG_CACHE);

                    if (!tx_cache_has_package(&cache, available[i].filename)) {
                        printf("    Downloading %s...\n",
                               available[i].filename);

                        char url[TX_MAX_URL * 2];
                        snprintf(url, sizeof(url), "%s/repo/%s/%s",
                                 available[i].repository[0] ?
                                     available[i].repository :
                                     TX_DEFAULT_REPO_URL,
                                 available[i].channel,
                                 available[i].filename);

                        tx_cache_get_path(&cache, available[i].filename,
                                          cache_file, sizeof(cache_file));

                        char cmd[TX_MAX_URL + TX_MAX_PATH + 128];
                        snprintf(cmd, sizeof(cmd),
                                 "curl -L --connect-timeout 30 "
                                 "--max-time 300 -o '%s' '%s' "
                                 "2>/dev/null",
                                 cache_file, url);

                        if (system(cmd) != 0) {
                            printf("    FAILED to download %s\n",
                                   action->package_name);
                            tx_cache_free(&cache);
                            break;
                        }
                    } else {
                        tx_cache_get_path(&cache, available[i].filename,
                                          cache_file, sizeof(cache_file));
                        printf("    Using cached %s\n",
                               available[i].filename);
                    }

                    /* Register in database */
                    tx_installed_pkg_t pkg;
                    tx_installed_pkg_init(&pkg);
                    strncpy(pkg.name, available[i].name,
                            TX_MAX_PACKAGE_NAME - 1);
                    tx_version_parse(&pkg.version,
                                     available[i].version.upstream);
                    strncpy(pkg.architecture, available[i].architecture, 31);
                    strncpy(pkg.description, available[i].description,
                            TX_MAX_DESCRIPTION - 1);
                    strncpy(pkg.repository, available[i].repository,
                            TX_MAX_PACKAGE_NAME - 1);
                    strncpy(pkg.channel, available[i].channel, 31);
                    strncpy(pkg.package_sha256, available[i].sha256, 64);
                    pkg.installed_size = available[i].installed_size;
                    pkg.is_auto = (strcmp(action->package_name, pkg_name) != 0);
                    pkg.status = TX_PKG_STATUS_INSTALLED;
                    pkg.install_date = time(NULL);

                    tx_db_register_package(&db, &pkg);
                    tx_installed_pkg_free(&pkg);

                    printf("    OK\n");
                    tx_cache_free(&cache);
                    break;
                }
            }
        }
        action = action->next;
    }

    printf("\nInstallation complete.\n");

    free(available);
    free(installed);
    tx_solution_free(&solution);
    tx_repo_list_free(&list);
    tx_db_close(&db);
    tx_lock_release(&lock);

    return 0;
}
