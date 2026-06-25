/*
 * TX Package Manager v1.0 - Common Types and Macros
 *
 * Copyright (c) 2026 TX Terminal Project
 * Licensed under the MIT License
 */

#ifndef TX_COMMON_H
#define TX_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include "config.h"

/* ==================================================================
 * Return Codes
 * ================================================================== */

typedef enum {
    TX_OK = 0,
    TX_ERROR_GENERAL = 1,
    TX_ERROR_NOT_FOUND = 2,
    TX_ERROR_ALREADY_EXISTS = 3,
    TX_ERROR_PERMISSION = 4,
    TX_ERROR_IO = 5,
    TX_ERROR_NETWORK = 6,
    TX_ERROR_PARSE = 7,
    TX_ERROR_DEPENDENCY = 8,
    TX_ERROR_CONFLICT = 9,
    TX_ERROR_VERIFY = 10,
    TX_ERROR_DATABASE = 11,
    TX_ERROR_TRANSACTION = 12,
    TX_ERROR_ROLLBACK = 13,
    TX_ERROR_LOCK = 14,
    TX_ERROR_TIMEOUT = 15,
    TX_ERROR_INVALID_ARG = 16,
    TX_ERROR_NO_SPACE = 17,
    TX_ERROR_CORRUPTED = 18,
    TX_ERROR_CANCELLED = 19,
    TX_ERROR_UNSUPPORTED = 20,
} tx_status_t;

/* ==================================================================
 * Package Status
 * ================================================================== */

typedef enum {
    TX_PKG_STATUS_UNKNOWN = 0,
    TX_PKG_STATUS_NOT_INSTALLED = 1,
    TX_PKG_STATUS_INSTALLED = 2,
    TX_PKG_STATUS_HALF_CONFIGURED = 3,
    TX_PKG_STATUS_HALF_INSTALLED = 4,
    TX_PKG_STATUS_CONFIG_FILES = 5,
    TX_PKG_STATUS_UNPACKED = 6,
    TX_PKG_STATUS_FAILED_CONFIG = 7,
    TX_PKG_STATUS_FAILED_INSTALL = 8,
    TX_PKG_STATUS_HELD = 9,
    TX_PKG_STATUS_PURGED = 10,
} tx_pkg_status_t;

/* ==================================================================
 * Dependency Relation Types
 * ================================================================== */

typedef enum {
    TX_REL_ANY = 0,
    TX_REL_EQ = 1,
    TX_REL_LT = 2,
    TX_REL_LE = 3,
    TX_REL_GT = 4,
    TX_REL_GE = 5,
    TX_REL_NE = 6,
} tx_relation_t;

/* ==================================================================
 * Dependency Kinds
 * ================================================================== */

typedef enum {
    TX_DEP_DEPENDS = 0,
    TX_DEP_PRE_DEPENDS = 1,
    TX_DEP_PROVIDES = 2,
    TX_DEP_CONFLICTS = 3,
    TX_DEP_REPLACES = 4,
    TX_DEP_BREAKS = 5,
    TX_DEP_SUGGESTS = 6,
    TX_DEP_RECOMMENDS = 7,
    TX_DEP_OPTIONAL = 8,
    TX_DEP_ENHANCES = 9,
    TX_DEP_VIRTUAL = 10,
} tx_dep_kind_t;

/* ==================================================================
 * Transaction Operations
 * ================================================================== */

typedef enum {
    TX_OP_INSTALL = 0,
    TX_OP_UPGRADE = 1,
    TX_OP_REMOVE = 2,
    TX_OP_REINSTALL = 3,
    TX_OP_CONFIGURE = 4,
    TX_OP_DECONFIGURE = 5,
    TX_OP_PURGE = 6,
} tx_op_type_t;

/* ==================================================================
 * Transaction States
 * ================================================================== */

typedef enum {
    TX_STATE_IDLE = 0,
    TX_STATE_PLANNING = 1,
    TX_STATE_DOWNLOADING = 2,
    TX_STATE_VERIFYING = 3,
    TX_STATE_EXTRACTING = 4,
    TX_STATE_RUNNING_SCRIPTS = 5,
    TX_STATE_CONFIGURING = 6,
    TX_STATE_REGISTERING = 7,
    TX_STATE_COMPLETED = 8,
    TX_STATE_FAILED = 9,
    TX_STATE_ROLLING_BACK = 10,
    TX_STATE_ROLLED_BACK = 11,
} tx_state_t;

/* ==================================================================
 * Forward Declarations
 * ================================================================== */

typedef struct tx_version tx_version_t;
typedef struct tx_dependency tx_dependency_t;
typedef struct tx_package tx_package_t;
typedef struct tx_file_entry tx_file_entry_t;
typedef struct tx_repository tx_repository_t;
typedef struct tx_transaction tx_transaction_t;
typedef struct tx_transaction_step tx_transaction_step_t;
typedef struct tx_solver_solution tx_solver_solution_t;

/* ==================================================================
 * Utility Macros
 * ================================================================== */

#define TX_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define TX_MIN(a, b) ((a) < (b) ? (a) : (b))
#define TX_MAX(a, b) ((a) > (b) ? (a) : (b))

#define TX_SAFE_FREE(ptr) do { \
    free(ptr); \
    (ptr) = NULL; \
} while (0)

#define TX_RETURN_IF_ERROR(status) do { \
    tx_status_t _s = (status); \
    if (_s != TX_OK) return _s; \
} while (0)

#define TX_GOTO_IF_ERROR(label, status) do { \
    if ((status) != TX_OK) goto label; \
} while (0)

#define TX_STRING_OR_EMPTY(str) ((str) ? (str) : "")
#define TX_STRING_OR_DEFAULT(str, def) ((str) ? (str) : (def))

#endif /* TX_COMMON_H */
