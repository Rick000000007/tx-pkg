/*
 * TX Package Manager v1.0 - Transaction Engine
 *
 * Provides atomic package operations with:
 *   - Transaction planning
 *   - Step-by-step execution
 *   - Rollback on failure
 *   - Journaling for recovery
 */

#ifndef TX_TRANSACTION_H
#define TX_TRANSACTION_H

#include "common.h"
#include "package.h"
#include "database.h"
#include "solver.h"
#include "cache.h"

/* ==================================================================
 * Transaction Step
 * ================================================================== */

typedef struct tx_transaction_step {
    int step_number;
    tx_op_type_t op;
    char package_name[TX_MAX_PACKAGE_NAME];
    tx_version_t target_version;
    tx_pkg_status_t old_status;
    tx_pkg_status_t new_status;

    /* Step state */
    bool completed;
    bool rolled_back;
    char error_message[512];

    /* Backup data for rollback */
    char backup_path[TX_MAX_PATH];

    struct tx_transaction_step *next;
} tx_transaction_step_t;

/* ==================================================================
 * Transaction Handle
 * ================================================================== */

typedef struct tx_transaction {
    char tx_id[64];             /* Unique transaction ID */
    tx_state_t state;

    /* Steps */
    tx_transaction_step_t *steps;
    size_t step_count;
    size_t current_step;

    /* Context */
    tx_db_t *db;
    tx_cache_t *cache;
    const char *install_prefix;

    /* Journal file for recovery */
    char journal_path[TX_MAX_PATH];

    /* Results */
    size_t completed;
    size_t failed;
    size_t rolled_back;

    /* Callbacks for progress reporting */
    void (*on_step_start)(struct tx_transaction *tx,
        tx_transaction_step_t *step);
    void (*on_step_complete)(struct tx_transaction *tx,
        tx_transaction_step_t *step);
    void (*on_step_error)(struct tx_transaction *tx,
        tx_transaction_step_t *step, const char *error);
} tx_transaction_t;

/* ==================================================================
 * Transaction Lifecycle
 * ================================================================== */

/**
 * Initialize a new transaction.
 */
tx_status_t tx_transaction_init(tx_transaction_t *tx,
    tx_db_t *db, tx_cache_t *cache, const char *prefix);

/**
 * Free transaction resources.
 */
void tx_transaction_free(tx_transaction_t *tx);

/* ==================================================================
 * Transaction Planning
 * ================================================================== */

/**
 * Create a transaction from a solver solution.
 */
tx_status_t tx_transaction_from_solution(tx_transaction_t *tx,
    const tx_solver_solution_t *sol);

/**
 * Add a step to a transaction.
 */
tx_status_t tx_transaction_add_step(tx_transaction_t *tx,
    tx_op_type_t op, const char *package_name,
    const tx_version_t *version);

/**
 * Validate a transaction plan.
 */
tx_status_t tx_transaction_validate(tx_transaction_t *tx);

/* ==================================================================
 * Transaction Execution
 * ================================================================== */

/**
 * Execute all transaction steps.
 * On failure, automatically rolls back completed steps.
 */
tx_status_t tx_transaction_execute(tx_transaction_t *tx);

/**
 * Execute a single step.
 */
tx_status_t tx_transaction_execute_step(tx_transaction_t *tx,
    tx_transaction_step_t *step);

/**
 * Commit a completed transaction.
 */
tx_status_t tx_transaction_commit(tx_transaction_t *tx);

/* ==================================================================
 * Rollback
 * ================================================================== */

/**
 * Rollback all completed steps in reverse order.
 */
tx_status_t tx_transaction_rollback(tx_transaction_t *tx);

/**
 * Rollback a single step.
 */
tx_status_t tx_transaction_rollback_step(tx_transaction_t *tx,
    tx_transaction_step_t *step);

/* ==================================================================
 * Journaling (for crash recovery)
 * ================================================================== */

/**
 * Write current state to journal.
 */
tx_status_t tx_transaction_journal_write(tx_transaction_t *tx);

/**
 * Read journal for recovery.
 */
tx_status_t tx_transaction_journal_read(const char *journal_path,
    tx_transaction_t *tx);

/**
 * Remove journal (call after successful commit).
 */
tx_status_t tx_transaction_journal_remove(tx_transaction_t *tx);

/* ==================================================================
 * Utilities
 * ================================================================== */

/**
 * Generate unique transaction ID.
 */
void tx_transaction_generate_id(char *id, size_t size);

/**
 * Get current transaction state.
 */
tx_state_t tx_transaction_get_state(const tx_transaction_t *tx);

/**
 * Print transaction summary.
 */
void tx_transaction_print(const tx_transaction_t *tx);

#endif /* TX_TRANSACTION_H */
