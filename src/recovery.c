/*
 * TX Package Manager v1.0 - Recovery Engine Implementation
 */

#include "recovery.h"
#include "error.h"
#include "lock.h"
#include "transaction.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

tx_status_t tx_recovery_check(tx_db_t *db, tx_recovery_status_t *status)
{
    if (!status)
        return TX_ERROR_INVALID_ARG;

    memset(status, 0, sizeof(*status));

    /* Check for stale locks */
    if (tx_lock_is_held(TX_LOCK_FILE))
        status->stale_locks++;

    /* Check for journal files */
    DIR *dir = opendir(TX_JOURNAL_DIR);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".journal"))
                status->incomplete_transactions++;
        }
        closedir(dir);
    }

    /* Check for half-installed packages */
    if (db && tx_db_is_open(db)) {
        sqlite3_stmt *stmt;
        const char *sql =
            "SELECT COUNT(*) FROM packages WHERE "
            "status IN (?, ?, ?, ?);";

        if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, TX_PKG_STATUS_HALF_INSTALLED);
            sqlite3_bind_int(stmt, 2, TX_PKG_STATUS_HALF_CONFIGURED);
            sqlite3_bind_int(stmt, 3, TX_PKG_STATUS_FAILED_CONFIG);
            sqlite3_bind_int(stmt, 4, TX_PKG_STATUS_FAILED_INSTALL);

            if (sqlite3_step(stmt) == SQLITE_ROW)
                status->broken_packages = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
    }

    status->needs_recovery = (status->stale_locks > 0 ||
                              status->incomplete_transactions > 0 ||
                              status->broken_packages > 0);

    if (status->needs_recovery) {
        snprintf(status->description, sizeof(status->description),
                 "Found: %d stale locks, %d incomplete transactions, "
                 "%d broken packages",
                 status->stale_locks,
                 status->incomplete_transactions,
                 status->broken_packages);
    } else {
        strncpy(status->description, "System is healthy",
                sizeof(status->description) - 1);
    }

    return TX_OK;
}

tx_status_t tx_recovery_run(tx_db_t *db, bool auto_fix)
{
    tx_recovery_status_t status;
    tx_status_t result = TX_OK;

    tx_recovery_check(db, &status);

    if (!status.needs_recovery) {
        printf("No recovery needed. System is healthy.\n");
        return TX_OK;
    }

    printf("Recovery needed:\n");
    tx_recovery_print_status(&status);

    if (!auto_fix) {
        printf("\nRun 'pkg doctor --fix' to automatically fix issues.\n");
        return TX_OK;
    }

    /* Fix stale locks */
    if (status.stale_locks > 0) {
        printf("Removing stale locks...\n");
        tx_recovery_remove_stale_locks();
    }

    /* Recover from journals */
    if (status.incomplete_transactions > 0) {
        printf("Recovering incomplete transactions...\n");

        DIR *dir = opendir(TX_JOURNAL_DIR);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (!strstr(entry->d_name, ".journal"))
                    continue;

                char journal_path[TX_MAX_PATH];
                snprintf(journal_path, sizeof(journal_path),
                         "%s/%s", TX_JOURNAL_DIR, entry->d_name);

                tx_recovery_from_journal(db, journal_path);
            }
            closedir(dir);
        }
    }

    /* Fix broken packages */
    if (status.broken_packages > 0) {
        printf("Fixing broken packages...\n");
        tx_recovery_fix_packages(db, true);
    }

    /* Clean up temp files */
    tx_recovery_cleanup_temp();

    printf("Recovery complete.\n");
    return result;
}

tx_status_t tx_recovery_from_journal(tx_db_t *db,
    const char *journal_path)
{
    if (!db || !journal_path)
        return TX_ERROR_INVALID_ARG;

    tx_transaction_t tx;
    tx_transaction_init(&tx, db, NULL, NULL);

    if (tx_transaction_journal_read(journal_path, &tx) != TX_OK) {
        /* Invalid journal - remove it */
        unlink(journal_path);
        return TX_ERROR_PARSE;
    }

    printf("  Recovering transaction %s (state %d)\n",
           tx.tx_id, tx.state);

    if (tx.state == TX_STATE_COMPLETED) {
        /* Transaction was committed - just clean up */
        tx_transaction_journal_remove(&tx);
    } else if (tx.state == TX_STATE_FAILED ||
               tx.state == TX_STATE_ROLLING_BACK ||
               tx.state == TX_STATE_EXTRACTING) {
        /* Need to rollback */
        tx_transaction_rollback(&tx);
        tx_transaction_journal_remove(&tx);
    } else {
        /* Unknown state - rollback to be safe */
        tx_transaction_rollback(&tx);
        tx_transaction_journal_remove(&tx);
    }

    tx_transaction_free(&tx);
    return TX_OK;
}

tx_status_t tx_recovery_fix_packages(tx_db_t *db, bool rollback)
{
    if (!db || !tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT name FROM packages WHERE "
        "status IN (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_int(stmt, 1, TX_PKG_STATUS_HALF_INSTALLED);
    sqlite3_bind_int(stmt, 2, TX_PKG_STATUS_HALF_CONFIGURED);
    sqlite3_bind_int(stmt, 3, TX_PKG_STATUS_FAILED_CONFIG);
    sqlite3_bind_int(stmt, 4, TX_PKG_STATUS_FAILED_INSTALL);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);

        if (rollback) {
            printf("  Rolling back broken package: %s\n", name);
            tx_db_unregister_package(db, name);
        } else {
            printf("  Marking package as installed: %s\n", name);
            tx_db_set_status(db, name, TX_PKG_STATUS_INSTALLED);
        }
    }

    sqlite3_finalize(stmt);
    return TX_OK;
}

tx_status_t tx_recovery_remove_stale_locks(void)
{
    if (!tx_lock_is_held(TX_LOCK_FILE))
        return TX_OK;

    tx_lock_t lock;
    tx_lock_init(&lock, TX_LOCK_FILE);

    /* Try to break the lock */
    tx_status_t s = tx_lock_break(&lock);
    tx_lock_free(&lock);

    if (s == TX_OK)
        printf("  Removed stale lock file\n");

    return s;
}

tx_status_t tx_recovery_cleanup_temp(void)
{
    /* Clean extraction directory */
    char cmd[TX_MAX_PATH + 64];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'/* 2>/dev/null",
             TX_EXTRACT_DIR);
    system(cmd);

    /* Clean old journal files */
    DIR *dir = opendir(TX_JOURNAL_DIR);
    if (!dir) return TX_OK;

    time_t now = time(NULL);
    time_t max_age = 7 * 24 * 3600;  /* 7 days */

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".journal")) {
            char path[TX_MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s",
                     TX_JOURNAL_DIR, entry->d_name);

            struct stat st;
            if (stat(path, &st) == 0) {
                if ((now - st.st_mtime) > max_age) {
                    printf("  Removing old journal: %s\n", entry->d_name);
                    unlink(path);
                }
            }
        }
    }

    closedir(dir);
    return TX_OK;
}

void tx_recovery_print_status(const tx_recovery_status_t *status)
{
    if (!status) return;

    printf("  %s\n", status->description);

    if (status->stale_locks > 0)
        printf("  - %d stale lock(s)\n", status->stale_locks);

    if (status->incomplete_transactions > 0)
        printf("  - %d incomplete transaction(s)\n",
               status->incomplete_transactions);

    if (status->broken_packages > 0)
        printf("  - %d broken package(s)\n",
               status->broken_packages);
}
