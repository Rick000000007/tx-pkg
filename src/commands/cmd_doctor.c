/*
 * TX Package Manager v1.0 - Doctor Command
 * Diagnoses and fixes common issues.
 */

#include "cmd_doctor.h"
#include "../config.h"
#include "../database.h"
#include "../recovery.h"
#include "../lock.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int cmd_doctor(int argc, char *argv[])
{
    printf("TX Package Manager v%s - Doctor\n", TX_PKG_VERSION);
    printf("================================\n\n");

    bool fix_mode = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--fix") == 0) {
            fix_mode = true;
            break;
        }
    }

    /* Check database */
    printf("[1/5] Checking package database...\n");
    tx_db_t db;
    char db_path[TX_MAX_PATH];
    snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

    if (tx_db_open(&db, db_path) == TX_OK) {
        char *report = NULL;
        tx_status_t s = tx_db_integrity_check(&db, &report);

        if (s == TX_OK)
            printf("      [OK] Database integrity verified\n");
        else
            printf("      [FAIL] Database integrity: %s\n",
                   report ? report : "unknown");

        free(report);

        /* Stats */
        size_t pkg_count, file_count, db_size;
        tx_db_stats(&db, &pkg_count, &file_count, &db_size);
        printf("      %zu packages, %zu files tracked\n",
               pkg_count, file_count);

        tx_db_close(&db);
    } else {
        printf("      [INFO] No database found (will be created on "
               "first use)\n");
    }

    /* Check cache */
    printf("\n[2/5] Checking cache directory...\n");
    if (access(TX_CACHE_DIR, F_OK) == 0)
        printf("      [OK] Cache directory exists\n");
    else
        printf("      [WARN] Cache directory missing\n");

    /* Check lock */
    printf("\n[3/5] Checking for stale locks...\n");
    if (tx_lock_is_held(TX_LOCK_FILE)) {
        printf("      [WARN] Lock file exists\n");

        if (fix_mode) {
            printf("      Removing stale lock...\n");
            tx_recovery_remove_stale_locks();
        }
    } else {
        printf("      [OK] No active locks\n");
    }

    /* Check for recovery needs */
    printf("\n[4/5] Checking for incomplete transactions...\n");

    tx_db_open(&db, db_path);
    tx_recovery_status_t rec_status;
    tx_recovery_check(&db, &rec_status);

    if (rec_status.needs_recovery) {
        printf("      [WARN] Recovery needed:\n");
        tx_recovery_print_status(&rec_status);

        if (fix_mode) {
            printf("\n      Running recovery...\n");
            tx_recovery_run(&db, true);
        }
    } else {
        printf("      [OK] No recovery needed\n");
    }

    tx_db_close(&db);

    /* Check directories */
    printf("\n[5/5] Checking directory structure...\n");
    const char *dirs[] = {
        TX_ROOT, TX_VAR_DIR, TX_LIB_DIR, TX_CACHE_DIR,
        TX_LOCK_DIR, TX_BACKUP_DIR, TX_JOURNAL_DIR, TX_ETC_DIR
    };

    for (size_t i = 0; i < TX_ARRAY_SIZE(dirs); i++) {
        if (access(dirs[i], F_OK) == 0)
            printf("      [OK] %s\n", dirs[i]);
        else
            printf("      [WARN] Missing: %s\n", dirs[i]);
    }

    printf("\n");
    if (fix_mode)
        printf("Doctor check complete. Issues were fixed where possible.\n");
    else
        printf("Doctor check complete. Run 'pkg doctor --fix' to "
               "automatically fix issues.\n");

    return 0;
}
