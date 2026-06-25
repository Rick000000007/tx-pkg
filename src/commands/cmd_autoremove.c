/*
 * TX Package Manager v1.0 - Autoremove Command
 * Removes packages that were auto-installed but are no longer needed.
 */

#include "cmd_autoremove.h"
#include "../config.h"
#include "../database.h"
#include "../solver.h"
#include "../lock.h"
#include <stdio.h>
#include <string.h>

int cmd_autoremove(int argc, char *argv[])
{
    (void)argc;

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

    tx_installed_pkg_t *installed = NULL;
    size_t installed_count = 0;
    tx_db_list_packages(&db, &installed, &installed_count);

    tx_solver_context_t ctx = {
        .installed = installed,
        .installed_count = installed_count,
    };

    tx_solver_solution_t solution;
    tx_status_t s = tx_solve_autoremove(&ctx, &solution);

    if (s != TX_OK || solution.action_count == 0) {
        printf("No orphaned packages found.\n");
        free(installed);
        tx_solution_free(&solution);
        tx_db_close(&db);
        tx_lock_release(&lock);
        return 0;
    }

    printf("The following packages will be removed:\n");
    tx_solution_print(&solution);

    /* Confirm */
    printf("Proceed? [Y/n] ");
    fflush(stdout);

    char response[16];
    if (fgets(response, sizeof(response), stdin) != NULL) {
        if (response[0] == 'n' || response[0] == 'N') {
            printf("Cancelled.\n");
            free(installed);
            tx_solution_free(&solution);
            tx_db_close(&db);
            tx_lock_release(&lock);
            return 0;
        }
    }

    /* Remove orphaned packages */
    tx_solver_action_t *action = solution.actions;
    while (action) {
        if (action->op == TX_OP_REMOVE) {
            printf("  Removing %s...\n", action->package_name);
            tx_db_unregister_package(&db, action->package_name);
        }
        action = action->next;
    }

    printf("\nAutoremove complete.\n");

    free(installed);
    tx_solution_free(&solution);
    tx_db_close(&db);
    tx_lock_release(&lock);

    return 0;
}
