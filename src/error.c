/*
 * TX Package Manager v1.0 - Error Handling Implementation
 */

#include "error.h"
#include <stdarg.h>

__thread tx_error_t tx_last_error = {0};

static const char *error_code_names[] = {
    [TX_OK] = "OK",
    [TX_ERROR_GENERAL] = "General Error",
    [TX_ERROR_NOT_FOUND] = "Not Found",
    [TX_ERROR_ALREADY_EXISTS] = "Already Exists",
    [TX_ERROR_PERMISSION] = "Permission Denied",
    [TX_ERROR_IO] = "I/O Error",
    [TX_ERROR_NETWORK] = "Network Error",
    [TX_ERROR_PARSE] = "Parse Error",
    [TX_ERROR_DEPENDENCY] = "Dependency Error",
    [TX_ERROR_CONFLICT] = "Conflict Error",
    [TX_ERROR_VERIFY] = "Verification Error",
    [TX_ERROR_DATABASE] = "Database Error",
    [TX_ERROR_TRANSACTION] = "Transaction Error",
    [TX_ERROR_ROLLBACK] = "Rollback Error",
    [TX_ERROR_LOCK] = "Lock Error",
    [TX_ERROR_TIMEOUT] = "Timeout Error",
    [TX_ERROR_INVALID_ARG] = "Invalid Argument",
    [TX_ERROR_NO_SPACE] = "No Space",
    [TX_ERROR_CORRUPTED] = "Corrupted Data",
    [TX_ERROR_CANCELLED] = "Cancelled",
    [TX_ERROR_UNSUPPORTED] = "Unsupported",
};

static const char *error_resolutions[] = {
    [TX_OK] = "No action needed.",
    [TX_ERROR_GENERAL] = "Check the error message and retry. If the problem persists, run 'pkg doctor' to diagnose.",
    [TX_ERROR_NOT_FOUND] = "Verify the spelling or check if the package/repository exists. Run 'pkg search <name>' to find packages.",
    [TX_ERROR_ALREADY_EXISTS] = "Use 'pkg upgrade' to update or 'pkg reinstall' to reinstall the package.",
    [TX_ERROR_PERMISSION] = "Ensure you have write permissions. Try running with appropriate privileges or check directory ownership.",
    [TX_ERROR_IO] = "Check disk health, file system permissions, and available disk space. Run 'pkg doctor' for diagnosis.",
    [TX_ERROR_NETWORK] = "Check your network connection. Verify the repository URL is correct with 'pkg repo list'.",
    [TX_ERROR_PARSE] = "The package or repository metadata may be malformed. Try 'pkg update' to refresh.",
    [TX_ERROR_DEPENDENCY] = "Run 'pkg install' with the missing dependencies, or use 'pkg doctor' to check system health.",
    [TX_ERROR_CONFLICT] = "Remove conflicting packages first, or use 'pkg --force' to override (not recommended).",
    [TX_ERROR_VERIFY] = "The package may be corrupted or tampered. Run 'pkg clean' and retry, or contact the repository maintainer.",
    [TX_ERROR_DATABASE] = "The package database may be corrupted. Run 'pkg doctor' to diagnose and repair.",
    [TX_ERROR_TRANSACTION] = "The previous transaction may have failed. Run 'pkg doctor' to check and recover.",
    [TX_ERROR_ROLLBACK] = "Manual intervention may be required. Run 'pkg doctor' and check backup files.",
    [TX_ERROR_LOCK] = "Another package operation is running. Wait for it to complete, or remove stale lock with 'pkg doctor --fix'.",
    [TX_ERROR_TIMEOUT] = "Check your network connection and retry. Increase timeout with environment variable if needed.",
    [TX_ERROR_INVALID_ARG] = "Check command usage with 'pkg help <command>'.",
    [TX_ERROR_NO_SPACE] = "Free up disk space and retry. Use 'pkg clean' to remove cached packages.",
    [TX_ERROR_CORRUPTED] = "The package or database may be corrupted. Run 'pkg verify' and 'pkg doctor' to diagnose.",
    [TX_ERROR_CANCELLED] = "Operation was cancelled. No changes were made.",
    [TX_ERROR_UNSUPPORTED] = "This operation is not supported in the current version. Check documentation for alternatives.",
};

void tx_error_set(tx_status_t code, const char *message,
                  const char *context, const char *cause,
                  const char *resolution)
{
    tx_last_error.code = code;
    strncpy(tx_last_error.message, message ? message : "",
            sizeof(tx_last_error.message) - 1);
    strncpy(tx_last_error.context, context ? context : "",
            sizeof(tx_last_error.context) - 1);
    strncpy(tx_last_error.cause, cause ? cause : "",
            sizeof(tx_last_error.cause) - 1);
    strncpy(tx_last_error.resolution, resolution ? resolution : "",
            sizeof(tx_last_error.resolution) - 1);
    tx_last_error.errno_value = errno;
}

void tx_error_set_simple(tx_status_t code, const char *message)
{
    tx_error_set(code, message, NULL, NULL,
                 tx_error_default_resolution(code));
}

const char *tx_error_string(tx_status_t code)
{
    if (code >= 0 && code < (int)TX_ARRAY_SIZE(error_code_names))
        return error_code_names[code];
    return "Unknown Error";
}

void tx_error_print(void)
{
    if (tx_last_error.code == TX_OK) {
        fprintf(stderr, "No error set.\n");
        return;
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "Error [%d]: %s\n",
            tx_last_error.code,
            tx_error_string(tx_last_error.code));
    fprintf(stderr, "====================================\n\n");

    if (tx_last_error.message[0]) {
        fprintf(stderr, "What happened:\n  %s\n\n", tx_last_error.message);
    }

    if (tx_last_error.context[0]) {
        fprintf(stderr, "Where:\n  %s\n\n", tx_last_error.context);
    }

    if (tx_last_error.cause[0]) {
        fprintf(stderr, "Why:\n  %s\n\n", tx_last_error.cause);
    }

    if (tx_last_error.resolution[0]) {
        fprintf(stderr, "How to resolve:\n  %s\n\n", tx_last_error.resolution);
    }

    if (tx_last_error.errno_value != 0) {
        fprintf(stderr, "System error: %s\n",
                strerror(tx_last_error.errno_value));
    }
}

void tx_error_clear(void)
{
    memset(&tx_last_error, 0, sizeof(tx_last_error));
}

bool tx_error_is_set(void)
{
    return tx_last_error.code != TX_OK;
}

const char *tx_error_default_resolution(tx_status_t code)
{
    if (code >= 0 && code < (int)TX_ARRAY_SIZE(error_resolutions))
        return error_resolutions[code];
    return "Check the error message and retry. Run 'pkg doctor' for diagnosis.";
}
