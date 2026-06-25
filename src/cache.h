/*
 * TX Package Manager v1.0 - Cache Manager
 *
 * Manages download cache with:
 *   - Download caching and reuse
 *   - Cache verification
 *   - Automatic cleanup
 *   - Cache size limiting
 */

#ifndef TX_CACHE_H
#define TX_CACHE_H

#include "common.h"

/* ==================================================================
 * Cache Manager
 * ================================================================== */

typedef struct tx_cache {
    char cache_dir[TX_MAX_PATH];
    size_t max_size_mb;
    int max_age_days;
} tx_cache_t;

/* ==================================================================
 * Cache Lifecycle
 * ================================================================== */

/**
 * Initialize cache manager.
 */
tx_status_t tx_cache_init(tx_cache_t *cache, const char *cache_dir);

/**
 * Free cache manager.
 */
void tx_cache_free(tx_cache_t *cache);

/* ==================================================================
 * Cache Operations
 * ================================================================== */

/**
 * Check if a package is in the cache.
 */
bool tx_cache_has_package(tx_cache_t *cache, const char *filename);

/**
 * Get path to cached package.
 */
tx_status_t tx_cache_get_path(tx_cache_t *cache, const char *filename,
    char *path, size_t path_size);

/**
 * Add a file to the cache.
 */
tx_status_t tx_cache_add(tx_cache_t *cache, const char *source,
    const char *filename);

/**
 * Remove a file from the cache.
 */
tx_status_t tx_cache_remove(tx_cache_t *cache, const char *filename);

/**
 * Verify a cached file against expected SHA256.
 */
bool tx_cache_verify(tx_cache_t *cache, const char *filename,
    const char *expected_sha256);

/* ==================================================================
 * Cache Maintenance
 * ================================================================== */

/**
 * Clean expired cache entries.
 */
tx_status_t tx_cache_clean_expired(tx_cache_t *cache);

/**
 * Clean all cache entries.
 */
tx_status_t tx_cache_clean_all(tx_cache_t *cache);

/**
 * Get cache size in bytes.
 */
size_t tx_cache_get_size(tx_cache_t *cache);

/**
 * Limit cache size by removing oldest files.
 */
tx_status_t tx_cache_limit_size(tx_cache_t *cache, size_t max_bytes);

/**
 * Print cache statistics.
 */
void tx_cache_stats(tx_cache_t *cache);

#endif /* TX_CACHE_H */
