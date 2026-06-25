/*
 * TX Package Manager v1.0 - Error Handling System
 *
 * Provides structured error reporting with context,
 * diagnostic messages, and resolution guidance.
 */

#ifndef TX_ERROR_H
#define TX_ERROR_H

#include "common.h"

/* ==================================================================
 * Error Context
 * ================================================================== */

typedef struct {
    tx_status_t code;
    char message[1024];
    char context[512];
    char cause[512];
    char resolution[1024];
    const char *file;
    int line;
    int errno_value;
} tx_error_t;

/* ==================================================================
 * Global Error State
 * ================================================================== */

extern __thread tx_error_t tx_last_error;

/* ==================================================================
 * Error Reporting API
 * ================================================================== */

/**
 * Set the last error with full diagnostic information.
 */
void tx_error_set(tx_status_t code,
                  const char *message,
                  const char *context,
                  const char *cause,
                  const char *resolution);

/**
 * Set a simple error with automatic resolution suggestion.
 */
void tx_error_set_simple(tx_status_t code, const char *message);

/**
 * Get a human-readable string for a status code.
 */
const char *tx_error_string(tx_status_t code);

/**
 * Print the current error to stderr with full diagnostics.
 */
void tx_error_print(void);

/**
 * Clear the last error.
 */
void tx_error_clear(void);

/**
 * Check if an error is set.
 */
bool tx_error_is_set(void);

/**
 * Get a default resolution message for a status code.
 */
const char *tx_error_default_resolution(tx_status_t code);

/* ==================================================================
 * Convenience Macros
 * ================================================================== */

#define TX_ERROR_SET(code, msg, ctx, cause, res) \
    tx_error_set((code), (msg), (ctx), (cause), (res))

#define TX_ERROR_SET_SIMPLE(code, msg) \
    tx_error_set_simple((code), (msg))

#define TX_ERROR_CONTEXT(file, line) \
    "at " file ":" #line

#endif /* TX_ERROR_H */
