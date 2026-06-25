/*
 * TX Package Manager v1.0 - Verify Command
 * Verifies package integrity.
 */

#include "cmd_verify.h"
#include "../config.h"
#include "../database.h"
#include "../verify.h"
#include <stdio.h>
#include <string.h>

int cmd_verify(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    tx_db_t db;
    char db_path[TX_MAX_PATH];
    snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

    if (tx_db_open(&db, db_path) != TX_OK) {
        fprintf(stderr, "Cannot open package database.\n");
        return 1;
    }

    if (argc >= 3) {
        /* Verify specific package */
        const char *pkg_name = argv[2];

        printf("Verifying package '%s'...\n\n", pkg_name);

        tx_verify_result_t result;
        tx_status_t s = tx_verify_installed(&db, pkg_name, &result);
        tx_verify_print_result(&result);
    } else {
        /* Verify system */
        printf("Verifying system integrity...\n\n");

        tx_verify_result_t result;
        tx_status_t s = tx_verify_system(&db, &result);
        tx_verify_print_result(&result);
    }

    tx_db_close(&db);
    return 0;
}
