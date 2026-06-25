/*
 * TX Package Manager v1.0 - Verification Engine Implementation
 */

#include "verify.h"
#include "error.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

bool tx_verify_sha256(const char *file_path,
    const char *expected_sha256, char *actual_sha256, size_t sha_size)
{
    if (!file_path || !expected_sha256)
        return false;

    char computed[65] = "";
    if (tx_compute_sha256(file_path, computed, sizeof(computed)) != TX_OK)
        return false;

    if (actual_sha256 && sha_size > 0) {
        strncpy(actual_sha256, computed, sha_size - 1);
        actual_sha256[sha_size - 1] = '\0';
    }

    return strcmp(computed, expected_sha256) == 0;
}

tx_status_t tx_verify_archive(const char *package_path,
    tx_verify_result_t *result)
{
    if (!package_path || !result)
        return TX_ERROR_INVALID_ARG;

    memset(result, 0, sizeof(*result));
    result->total_checks = 3;

    /* Check file exists */
    struct stat st;
    if (stat(package_path, &st) != 0) {
        snprintf(result->message, sizeof(result->message),
                 "Package file not found: %s", package_path);
        result->failed_checks++;
        result->passed = false;
        return TX_ERROR_NOT_FOUND;
    }

    /* Check file is not empty */
    if (st.st_size == 0) {
        snprintf(result->message, sizeof(result->message),
                 "Package file is empty: %s", package_path);
        result->failed_checks++;
        result->passed = false;
        return TX_ERROR_CORRUPTED;
    }

    /* Check it's a valid tar archive */
    char cmd[TX_MAX_PATH + 64];
    snprintf(cmd, sizeof(cmd),
             "tar -tf '%s' >/dev/null 2>&1", package_path);

    if (system(cmd) != 0) {
        snprintf(result->message, sizeof(result->message),
                 "Package is not a valid archive: %s", package_path);
        result->failed_checks++;
        result->passed = false;
        return TX_ERROR_CORRUPTED;
    }

    result->passed = (result->failed_checks == 0);
    if (result->passed)
        snprintf(result->message, sizeof(result->message),
                 "Package archive is valid");

    return TX_OK;
}

tx_status_t tx_verify_manifest(const tx_manifest_t *manifest,
    tx_verify_result_t *result)
{
    if (!manifest || !result)
        return TX_ERROR_INVALID_ARG;

    memset(result, 0, sizeof(*result));
    result->total_checks = 4;

    /* Check name */
    if (manifest->name[0] == '\0') {
        strncat(result->message, "Missing package name. ",
                sizeof(result->message) - strlen(result->message) - 1);
        result->failed_checks++;
    }

    /* Check version */
    if (manifest->version.upstream[0] == '\0') {
        strncat(result->message, "Missing package version. ",
                sizeof(result->message) - strlen(result->message) - 1);
        result->failed_checks++;
    }

    /* Check architecture */
    if (manifest->architecture[0] == '\0') {
        strncat(result->message, "Missing architecture. ",
                sizeof(result->message) - strlen(result->message) - 1);
        result->failed_checks++;
    }

    /* Check description */
    if (manifest->description[0] == '\0') {
        strncat(result->message, "Missing description. ",
                sizeof(result->message) - strlen(result->message) - 1);
        result->failed_checks++;
    }

    result->passed = (result->failed_checks == 0);
    if (result->passed)
        snprintf(result->message, sizeof(result->message),
                 "Manifest is valid");

    return result->passed ? TX_OK : TX_ERROR_VERIFY;
}

tx_status_t tx_verify_installed(tx_db_t *db, const char *package_name,
    tx_verify_result_t *result)
{
    if (!db || !package_name || !result)
        return TX_ERROR_INVALID_ARG;

    memset(result, 0, sizeof(*result));

    /* Check package is installed */
    if (!tx_db_is_installed(db, package_name)) {
        snprintf(result->message, sizeof(result->message),
                 "Package '%s' is not installed", package_name);
        result->passed = false;
        return TX_ERROR_NOT_FOUND;
    }

    /* Get file list */
    tx_file_entry_t *files = NULL;
    size_t count = 0;

    tx_status_t s = tx_db_package_files(db, package_name, &files, &count);
    if (s != TX_OK) {
        snprintf(result->message, sizeof(result->message),
                 "Cannot read file list for '%s'", package_name);
        result->passed = false;
        return s;
    }

    result->total_checks = (int)count;

    for (size_t i = 0; i < count; i++) {
        if (access(files[i].path, F_OK) != 0) {
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "Missing file: %s\n", files[i].path);
            strncat(result->message, msg,
                    sizeof(result->message) - strlen(result->message) - 1);
            result->failed_checks++;
            continue;
        }

        if (files[i].sha256[0] &&
            !tx_verify_file(files[i].path, files[i].sha256)) {
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "Checksum mismatch: %s\n", files[i].path);
            strncat(result->message, msg,
                    sizeof(result->message) - strlen(result->message) - 1);
            result->failed_checks++;
        }
    }

    free(files);

    result->passed = (result->failed_checks == 0);
    if (result->passed)
        snprintf(result->message, sizeof(result->message),
                 "Package '%s' verification passed (%zu files checked)",
                 package_name, count);
    else
        snprintf(result->message + strlen(result->message),
                 sizeof(result->message) - strlen(result->message),
                 "\n%d/%d checks failed",
                 result->failed_checks, result->total_checks);

    return result->passed ? TX_OK : TX_ERROR_VERIFY;
}

bool tx_verify_file(const char *path, const char *expected_sha256)
{
    if (!path || !expected_sha256)
        return false;

    char actual[65] = "";
    return (tx_compute_sha256(path, actual, sizeof(actual)) == TX_OK &&
            strcmp(actual, expected_sha256) == 0);
}

tx_status_t tx_compute_sha256(const char *file_path,
    char *sha256_out, size_t out_size)
{
    if (!file_path || !sha256_out || out_size < 65)
        return TX_ERROR_INVALID_ARG;

    char cmd[TX_MAX_PATH + 64];
    snprintf(cmd, sizeof(cmd), "sha256sum '%s' 2>/dev/null", file_path);

    FILE *fp = popen(cmd, "r");
    if (!fp) return TX_ERROR_IO;

    char result[65] = "";
    if (fscanf(fp, "%64s", result) != 1) {
        pclose(fp);
        return TX_ERROR_IO;
    }
    pclose(fp);

    strncpy(sha256_out, result, out_size - 1);
    sha256_out[out_size - 1] = '\0';

    return TX_OK;
}

tx_status_t tx_verify_repository(const char *repo_cache_dir,
    tx_verify_result_t *result)
{
    if (!repo_cache_dir || !result)
        return TX_ERROR_INVALID_ARG;

    memset(result, 0, sizeof(*result));

    /* Check Packages.json exists */
    char path[TX_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s",
             repo_cache_dir, TX_REPO_PACKAGES_FILE);

    if (access(path, F_OK) != 0) {
        snprintf(result->message, sizeof(result->message),
                 "Repository metadata not found: %s", path);
        result->passed = false;
        return TX_ERROR_NOT_FOUND;
    }

    /* Try to parse it */
    FILE *fp = fopen(path, "r");
    if (!fp) {
        snprintf(result->message, sizeof(result->message),
                 "Cannot read repository metadata");
        result->passed = false;
        return TX_ERROR_IO;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);

    if (size <= 0) {
        snprintf(result->message, sizeof(result->message),
                 "Repository metadata is empty");
        result->passed = false;
        return TX_ERROR_CORRUPTED;
    }

    result->passed = true;
    snprintf(result->message, sizeof(result->message),
             "Repository metadata is valid (%ld bytes)", size);

    return TX_OK;
}

tx_status_t tx_verify_system(tx_db_t *db, tx_verify_result_t *result)
{
    if (!db || !result)
        return TX_ERROR_INVALID_ARG;

    memset(result, 0, sizeof(*result));
    result->passed = true;

    /* Check database integrity */
    char *integrity = NULL;
    tx_status_t s = tx_db_integrity_check(db, &integrity);
    if (s != TX_OK) {
        snprintf(result->message, sizeof(result->message),
                 "Database integrity check failed: %s",
                 integrity ? integrity : "unknown");
        result->passed = false;
    }
    free(integrity);

    /* List all packages and count */
    size_t count = tx_db_count_packages(db);
    if (result->passed) {
        snprintf(result->message, sizeof(result->message),
                 "System verification passed (%zu packages)", count);
    }

    return result->passed ? TX_OK : TX_ERROR_VERIFY;
}

void tx_verify_print_result(const tx_verify_result_t *result)
{
    if (!result) return;

    if (result->passed)
        printf("[PASS] %s\n", result->message);
    else
        printf("[FAIL] %s\n", result->message);

    if (result->total_checks > 0)
        printf("       Checks: %d/%d passed\n",
               result->total_checks - result->failed_checks,
               result->total_checks);
}
