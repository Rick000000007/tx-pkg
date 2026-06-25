/*
 * TX Package Manager v1.0 - Recovery Engine
 *
 * Handles recovery from:
 *   - Interrupted installations
 *   - Power failures during transactions
 *   - Process kills
 *   - Storage failures
 *   - Partial extractions
 */

#ifndef TX_RECOVERY_H
#define TX_RECOVERY_H

#include "common.h"
#include "database.h"

/* ==================================================================
 * Recovery Check Result
 * ================================================================== */

typedef struct tx_recovery_status {
    bool needs_recovery;
    char description[1024];
    int incomplete_transactions;
    int broken_packages;
    int stale_locks;
} tx_recovery_status_t;

/* ==================================================================
 * Recovery API
 * ================================================================== */

/**
 * Check if recovery is needed.
 * Scans for:
 *   - Uncommitted journals
 *   - Half-installed packages
 *   - Stale locks
 *   - Orphaned files
 */
tx_status_t tx_recovery_check(tx_db_t *db,
    tx_recovery_status_t *status);

/**
 * Perform automatic recovery.
 *
 * For each incomplete transaction found:
 *   - If post-install state, complete it
 *   - If mid-install, rollback
 */
tx_status_t tx_recovery_run(tx_db_t *db, bool auto_fix);

/**
 * Recover from a specific journal file.
 */
tx_status_t tx_recovery_from_journal(tx_db_t *db,
    const char *journal_path);

/**
 * Fix half-installed packages by completing or rolling back.
 */
tx_status_t tx_recovery_fix_packages(tx_db_t *db, bool rollback);

/**
 * Remove stale lock files.
 */
tx_status_t tx_recovery_remove_stale_locks(void);

/**
 * Clean up orphaned temporary files.
 */
tx_status_t tx_recovery_cleanup_temp(void);

/**
 * Print recovery status.
 */
void tx_recovery_print_status(const tx_recovery_status_t *status);

#endif /* TX_RECOVERY_H */
