/*
 * TX Package Manager v1.0 - Package Database
 *
 * SQLite-based installed package database providing:
 *   - Package registration and tracking
 *   - File ownership records
 *   - Reverse dependency tracking
 *   - Status management
 *   - Query and search capabilities
 */

#ifndef TX_DATABASE_H
#define TX_DATABASE_H

#include "common.h"
#include "package.h"
#include <sqlite3.h>

/* ==================================================================
 * Database Handle
 * ================================================================== */

typedef struct tx_db {
    sqlite3 *sqlite;
    char path[TX_MAX_PATH];
    bool is_open;
    int transaction_depth;
} tx_db_t;

/* ==================================================================
 * Database Lifecycle
 * ================================================================== */

/**
 * Open or create the package database.
 * Initializes schema if database is new.
 */
tx_status_t tx_db_open(tx_db_t *db, const char *path);

/**
 * Close the database.
 */
void tx_db_close(tx_db_t *db);

/**
 * Check if database is open and valid.
 */
bool tx_db_is_open(const tx_db_t *db);

/* ==================================================================
 * Schema Management
 * ================================================================== */

/**
 * Initialize/upgrade database schema.
 * Creates tables and indexes if they don't exist.
 */
tx_status_t tx_db_init_schema(tx_db_t *db);

/**
 * Get current schema version.
 */
int tx_db_schema_version(tx_db_t *db);

/* ==================================================================
 * Package Registration
 * ================================================================== */

/**
 * Register an installed package in the database.
 */
tx_status_t tx_db_register_package(tx_db_t *db,
    const tx_installed_pkg_t *pkg);

/**
 * Remove a package from the database.
 */
tx_status_t tx_db_unregister_package(tx_db_t *db,
    const char *name);

/**
 * Update package status.
 */
tx_status_t tx_db_set_status(tx_db_t *db, const char *name,
    tx_pkg_status_t status);

/**
 * Update package version (after upgrade).
 */
tx_status_t tx_db_set_version(tx_db_t *db, const char *name,
    const tx_version_t *version);

/**
 * Set/unset the 'held' flag on a package.
 */
tx_status_t tx_db_set_held(tx_db_t *db, const char *name,
    bool held);

/**
 * Set/unset the 'auto' flag on a package.
 */
tx_status_t tx_db_set_auto(tx_db_t *db, const char *name,
    bool auto_flag);

/* ==================================================================
 * Package Queries
 * ================================================================== */

/**
 * Check if a package is installed.
 */
bool tx_db_is_installed(tx_db_t *db, const char *name);

/**
 * Get installed package info.
 */
tx_status_t tx_db_get_package(tx_db_t *db, const char *name,
    tx_installed_pkg_t *pkg);

/**
 * List all installed packages.
 */
tx_status_t tx_db_list_packages(tx_db_t *db,
    tx_installed_pkg_t **pkgs, size_t *count);

/**
 * Get package status.
 */
tx_pkg_status_t tx_db_get_status(tx_db_t *db, const char *name);

/**
 * Count installed packages.
 */
size_t tx_db_count_packages(tx_db_t *db);

/* ==================================================================
 * File Ownership
 * ================================================================== */

/**
 * Register a file as owned by a package.
 */
tx_status_t tx_db_register_file(tx_db_t *db, const char *pkg_name,
    const tx_file_entry_t *file);

/**
 * Unregister all files for a package.
 */
tx_status_t tx_db_unregister_files(tx_db_t *db,
    const char *pkg_name);

/**
 * Find which package owns a file.
 * Returns TX_OK and fills pkg_name, or TX_ERROR_NOT_FOUND.
 */
tx_status_t tx_db_file_owner(tx_db_t *db, const char *path,
    char *pkg_name, size_t pkg_name_size);

/**
 * Get list of files owned by a package.
 */
tx_status_t tx_db_package_files(tx_db_t *db, const char *name,
    tx_file_entry_t **files, size_t *count);

/**
 * Check if a file path is safe (doesn't escape prefix).
 */
bool tx_db_path_is_safe(const char *path, const char *prefix);

/* ==================================================================
 * Dependency Tracking
 * ================================================================== */

/**
 * Register a reverse dependency.
 */
tx_status_t tx_db_add_reverse_dep(tx_db_t *db,
    const char *pkg_name, const char *depends_on);

/**
 * Remove reverse dependencies for a package.
 */
tx_status_t tx_db_remove_reverse_deps(tx_db_t *db,
    const char *pkg_name);

/**
 * Get packages that depend on a given package.
 */
tx_status_t tx_db_get_reverse_deps(tx_db_t *db,
    const char *pkg_name, char ***deps, size_t *count);

/**
 * Check if any installed package depends on the given package.
 */
bool tx_db_is_required(tx_db_t *db, const char *pkg_name);

/* ==================================================================
 * Transactions (Database-level)
 * ================================================================== */

/**
 * Begin a database transaction.
 */
tx_status_t tx_db_begin(tx_db_t *db);

/**
 * Commit a database transaction.
 */
tx_status_t tx_db_commit(tx_db_t *db);

/**
 * Rollback a database transaction.
 */
tx_status_t tx_db_rollback(tx_db_t *db);

/* ==================================================================
 * Database Integrity
 * ================================================================== */

/**
 * Run integrity checks on the database.
 */
tx_status_t tx_db_integrity_check(tx_db_t *db, char **report);

/**
 * Vacuum the database.
 */
tx_status_t tx_db_vacuum(tx_db_t *db);

/**
 * Get database statistics.
 */
tx_status_t tx_db_stats(tx_db_t *db, size_t *pkg_count,
    size_t *file_count, size_t *db_size);

#endif /* TX_DATABASE_H */
