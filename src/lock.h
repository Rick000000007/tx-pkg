/*
 * TX Package Manager v1.0 - File Locking
 *
 * Prevents concurrent package operations using:
 *   - PID-based file locking
 *   - Stale lock detection
 *   - Timeout handling
 */

#ifndef TX_LOCK_H
#define TX_LOCK_H

#include "common.h"

/* ==================================================================
 * Lock Handle
 * ================================================================== */

typedef struct tx_lock {
    char lock_file[TX_MAX_PATH];
    int fd;
    bool is_locked;
    pid_t owner_pid;
} tx_lock_t;

/* ==================================================================
 * Lock API
 * ================================================================== */

/**
 * Initialize lock handle.
 */
tx_status_t tx_lock_init(tx_lock_t *lock, const char *lock_file);

/**
 * Acquire lock. Blocks until lock is available or timeout.
 */
tx_status_t tx_lock_acquire(tx_lock_t *lock);

/**
 * Try to acquire lock without blocking.
 * Returns TX_OK if acquired, TX_ERROR_LOCK if not.
 */
tx_status_t tx_lock_try_acquire(tx_lock_t *lock);

/**
 * Release lock.
 */
tx_status_t tx_lock_release(tx_lock_t *lock);

/**
 * Check if a lock is held (without acquiring).
 */
bool tx_lock_is_held(const char *lock_file);

/**
 * Force release a stale lock (dangerous - use with care).
 */
tx_status_t tx_lock_break(tx_lock_t *lock);

/**
 * Free lock handle.
 */
void tx_lock_free(tx_lock_t *lock);

#endif /* TX_LOCK_H */
