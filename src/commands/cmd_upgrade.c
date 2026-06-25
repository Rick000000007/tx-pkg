/*
 * TX Package Manager v1.0 - Upgrade Command
 * Upgrades all installed packages or a specific package.
 */

#include "cmd_upgrade.h"
#include "../config.h"
#include "../database.h"
#include "../repository.h"
#include "../solver.h"
#include "../lock.h"
#include "../error.h"
#include <stdio.h>
#include <string.h>

int cmd_upgrade(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

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

    /* Load repositories */
    tx_repo_list_t list;
    tx_repo_list_init(&list);

    tx_installed_pkg_t *installed = NULL;
    size_t installed_count = 0;
    tx_db_list_packages(&db, &installed, &installed_count);

    tx_pkg_entry_t *available = NULL;
    size_t available_count = 0;
    tx_repo_get_all_packages(&list, &available, &available_count);

    tx_solver_context_t ctx = {
        .available = available,
        .available_count = available_count,
        .installed = installed,
        .installed_count = installed_count,
        .db_handle = &db,
    };

    tx_solver_solution_t solution;
    tx_status_t s;

    if (argc >= 3) {
        /* Upgrade specific package */
        printf("Checking for upgrade to '%s'...\n", argv[2]);
        s = tx_solve_upgrade_one(&ctx, argv[2], &solution);
    } else {
        /* Upgrade all */
        printf("Checking for upgrades...\n");
        s = tx_solve_upgrade(&ctx, &solution);
    }

    if (s != TX_OK || !solution.is_valid || solution.action_count == 0) {
        if (solution.error_message[0])
            printf("%s\n", solution.error_message);
        else
            printf("All packages are up to date.\n");

        free(available);
        free(installed);
        tx_solution_free(&solution);
        tx_repo_list_free(&list);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 0;
    }

    tx_solution_print(&solution);

    /* Confirm */
    printf("Proceed with upgrade? [Y/n] ");
    fflush(stdout);

    char response[16];
    if (fgets(response, sizeof(response), stdin) != NULL) {
        if (response[0] == 'n' || response[0] == 'N') {
            printf("Upgrade cancelled.\n");

            free(available);
            free(installed);
            tx_solution_free(&solution);
            tx_repo_list_free(&list);
            tx_db_close(&db);
            tx_lock_release(&lock);
            return 0;
        }
    }

    printf("\nUpgrading...\n");

    /* Execute upgrades */
    tx_solver_action_t *action = solution.actions;
    while (action) {
        if (action->op == TX_OP_UPGRADE) {
            printf("  Upgrading %s to %s...\n",
                   action->package_name,
                   action->version.upstream);

            tx_db_set_version(&db, action->package_name,
                              &action->version);
            printf("    OK\n");
        }
        action = action->next;
    }

    printf("\nUpgrade complete.\n");

    free(available);
    free(installed);
    tx_solution_free(&solution);
    tx_repo_list_free(&list);
    tx_db_close(&db);
    tx_lock_release(&lock);

    return 0;
}
