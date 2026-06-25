/*
 * TX Package Manager v1.0 - Self Update
 *
 * Atomic self-update with:
 *   - Download new version
 *   - Verify integrity
 *   - Atomic swap
 *   - Rollback capability
 *   - Channel awareness
 *   - Runtime compatibility checking
 */

#ifndef TX_SELFUPDATE_H
#define TX_SELFUPDATE_H

#include "common.h"

/* ==================================================================
 * Self Update API
 * ================================================================== */

/**
 * Check if a new version of tx-pkg is available.
 */
tx_status_t tx_selfupdate_check(bool *update_available,
    char *new_version, size_t ver_size);

/**
 * Perform self-update.
 * Returns TX_OK on success, leaving the new binary in place.
 */
tx_status_t tx_selfupdate_perform(void);

/**
 * Rollback to previous version.
 */
tx_status_t tx_selfupdate_rollback(void);

/**
 * Get current tx-pkg version string.
 */
const char *tx_selfupdate_current_version(void);

/**
 * Print self-update status.
 */
void tx_selfupdate_print_status(void);

#endif /* TX_SELFUPDATE_H */
