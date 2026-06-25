/*
 * TX Package Manager v1.0 - Version Comparison Engine
 *
 * Implements Debian-style version comparison with epoch support.
 * Supports: epoch:upstream-release format.
 *
 * Comparison rules follow dpkg --compare-version semantics:
 *   - Epoch compared first (numeric, higher wins)
 *   - Upstream version compared lexically with tilde-sorting
 *   - Release number compared last
 */

#ifndef TX_VERSION_H
#define TX_VERSION_H

#include "common.h"

/* ==================================================================
 * Version Structure
 *
 * Format: [epoch:]upstream-version[-release]
 * Examples:
 *   1.2.3
 *   2:1.2.3-1
 *   1.2.3~rc1-2
 * ================================================================== */

typedef struct tx_version {
    uint32_t epoch;
    char upstream[TX_MAX_VERSION];
    char release[TX_MAX_VERSION];
} tx_version_t;

/* ==================================================================
 * Version Parsing
 * ================================================================== */

/**
 * Parse a version string into a tx_version structure.
 * Returns TX_OK on success, TX_ERROR_PARSE on failure.
 */
tx_status_t tx_version_parse(tx_version_t *ver, const char *str);

/**
 * Convert a version structure back to a string.
 * Returns TX_OK on success.
 */
tx_status_t tx_version_to_string(const tx_version_t *ver,
                                  char *buf, size_t buf_size);

/* ==================================================================
 * Version Comparison
 * ================================================================== */

/**
 * Compare two versions.
 * Returns: -1 if a < b, 0 if a == b, 1 if a > b
 */
int tx_version_compare(const tx_version_t *a, const tx_version_t *b);

/**
 * Check if a version satisfies a relation against another version.
 * Example: tx_version_satisfies(installed, TX_REL_GE, required)
 *          checks if installed >= required
 */
bool tx_version_satisfies(const tx_version_t *installed,
                          tx_relation_t rel,
                          const tx_version_t *required);

/**
 * Convenience: compare two version strings directly.
 */
int tx_version_compare_strings(const char *a, const char *b);

/**
 * Convenience: check if version string satisfies relation.
 */
bool tx_version_string_satisfies(const char *installed,
                                  tx_relation_t rel,
                                  const char *required);

/* ==================================================================
 * Version Utilities
 * ================================================================== */

/**
 * Check if a string is a valid version format.
 */
bool tx_version_is_valid(const char *str);

/**
 * Normalize a version string (trim whitespace, validate).
 */
tx_status_t tx_version_normalize(char *str, size_t len);

/**
 * Get the epoch from a version string. Returns 0 if no epoch.
 */
uint32_t tx_version_get_epoch(const char *str);

/**
 * Check if a version represents a downgrade from another.
 */
bool tx_version_is_downgrade(const tx_version_t *current,
                              const tx_version_t *candidate);

#endif /* TX_VERSION_H */
