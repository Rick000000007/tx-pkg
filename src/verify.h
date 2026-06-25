/*
 * TX Package Manager v1.0 - Verification Engine
 *
 * Provides comprehensive package verification:
 *   - SHA-256 checksum verification
 *   - Manifest validation
 *   - Installed file integrity checking
 *   - Corruption detection
 *   - Repository signature verification (future-ready)
 */

#ifndef TX_VERIFY_H
#define TX_VERIFY_H

#include "common.h"
#include "package.h"
#include "database.h"

/* ==================================================================
 * Verification Result
 * ================================================================== */

typedef struct tx_verify_result {
    bool passed;
    char message[1024];
    int failed_checks;
    int total_checks;
} tx_verify_result_t;

/* ==================================================================
 * Verification API
 * ================================================================== */

/**
 * Verify a package file against expected SHA256.
 */
bool tx_verify_sha256(const char *file_path,
    const char *expected_sha256, char *actual_sha256, size_t sha_size);

/**
 * Verify a package archive is valid and can be extracted.
 */
tx_status_t tx_verify_archive(const char *package_path,
    tx_verify_result_t *result);

/**
 * Verify package manifest is complete and valid.
 */
tx_status_t tx_verify_manifest(const tx_manifest_t *manifest,
    tx_verify_result_t *result);

/**
 * Verify all files of an installed package.
 */
tx_status_t tx_verify_installed(tx_db_t *db, const char *package_name,
    tx_verify_result_t *result);

/**
 * Verify a single file against stored checksum.
 */
bool tx_verify_file(const char *path, const char *expected_sha256);

/**
 * Calculate SHA256 of a file.
 */
tx_status_t tx_compute_sha256(const char *file_path,
    char *sha256_out, size_t out_size);

/**
 * Verify repository metadata integrity.
 */
tx_status_t tx_verify_repository(const char *repo_cache_dir,
    tx_verify_result_t *result);

/**
 * Check for system-wide package integrity issues.
 */
tx_status_t tx_verify_system(tx_db_t *db, tx_verify_result_t *result);

/**
 * Print verification result.
 */
void tx_verify_print_result(const tx_verify_result_t *result);

#endif /* TX_VERIFY_H */
