/*
 * TX Package Manager v1.0 - Rollback Engine
 *
 * Provides complete rollback capability for failed transactions:
 *   - File restoration from backups
 *   - Database state restoration
 *   - Package cache restoration
 *   - Configuration preservation
 */

#ifndef TX_ROLLBACK_H
#define TX_ROLLBACK_H

#include "common.h"
#include "database.h"
#include "package.h"

/* ==================================================================
 * Rollback Handle
 * ================================================================== */

typedef struct tx_rollback {
    char backup_dir[TX_MAX_PATH];
    tx_db_t *db;
} tx_rollback_t;

/* ==================================================================
 * Rollback API
 * ================================================================== */

/**
 * Initialize rollback engine.
 */
tx_status_t tx_rollback_init(tx_rollback_t *rb,
    const char *backup_dir, tx_db_t *db);

/**
 * Free rollback resources.
 */
void tx_rollback_free(tx_rollback_t *rb);

/**
 * Create a backup of a package's files before modification.
 */
tx_status_t tx_rollback_backup_package(tx_rollback_t *rb,
    const char *package_name);

/**
 * Restore a package's files from backup.
 */
tx_status_t tx_rollback_restore_package(tx_rollback_t *rb,
    const char *package_name);

/**
 * Backup a single file.
 */
tx_status_t tx_rollback_backup_file(tx_rollback_t *rb,
    const char *file_path);

/**
 * Restore a single file from backup.
 */
tx_status_t tx_rollback_restore_file(tx_rollback_t *rb,
    const char *file_path);

/**
 * Backup database state.
 */
tx_status_t tx_rollback_backup_database(tx_rollback_t *rb);

/**
 * Restore database from backup.
 */
tx_status_t tx_rollback_restore_database(tx_rollback_t *rb);

/**
 * Clean up all backups for a package.
 */
tx_status_t tx_rollback_cleanup(tx_rollback_t *rb,
    const char *package_name);

#endif /* TX_ROLLBACK_H */
