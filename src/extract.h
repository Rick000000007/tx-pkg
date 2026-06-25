/*
 * TX Package Manager v1.0 - Package Extraction
 *
 * Safely extracts package archives with:
 *   - Path traversal protection
 *   - Script validation
 *   - Atomic extraction to temp then move
 */

#ifndef TX_EXTRACT_H
#define TX_EXTRACT_H

#include "common.h"
#include "package.h"

/* ==================================================================
 * Extraction Handle
 * ================================================================== */

typedef struct tx_extract {
    char package_path[TX_MAX_PATH];
    char extract_dir[TX_MAX_PATH];
    char install_prefix[TX_MAX_PATH];
    tx_manifest_t manifest;
    bool is_extracted;
} tx_extract_t;

/* ==================================================================
 * Extraction API
 * ================================================================== */

/**
 * Initialize extraction handle.
 */
tx_status_t tx_extract_init(tx_extract_t *ex,
    const char *package_path, const char *extract_dir,
    const char *install_prefix);

/**
 * Free extraction handle.
 */
void tx_extract_free(tx_extract_t *ex);

/**
 * Extract package archive to temporary directory.
 */
tx_status_t tx_extract_archive(tx_extract_t *ex);

/**
 * Read and parse manifest from extracted package.
 */
tx_status_t tx_extract_read_manifest(tx_extract_t *ex);

/**
 * Read and parse CONTROL file.
 */
tx_status_t tx_extract_read_control(tx_extract_t *ex);

/**
 * Validate extracted package structure.
 */
tx_status_t tx_extract_validate(tx_extract_t *ex);

/**
 * Check if all paths in the package are safe.
 */
tx_status_t tx_extract_check_paths(tx_extract_t *ex);

/**
 * Install extracted files to the install prefix.
 */
tx_status_t tx_extract_install_files(tx_extract_t *ex);

/**
 * Run an installation script if present.
 */
tx_status_t tx_extract_run_script(tx_extract_t *ex,
    const char *script_name);

/**
 * Clean up extraction directory.
 */
tx_status_t tx_extract_cleanup(tx_extract_t *ex);

#endif /* TX_EXTRACT_H */
