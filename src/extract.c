/*
 * TX Package Manager v1.0 - Package Extraction Implementation
 */

#include "extract.h"
#include "error.h"
#include "database.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

tx_status_t tx_extract_init(tx_extract_t *ex,
    const char *package_path, const char *extract_dir,
    const char *install_prefix)
{
    if (!ex || !package_path)
        return TX_ERROR_INVALID_ARG;

    memset(ex, 0, sizeof(*ex));
    strncpy(ex->package_path, package_path, TX_MAX_PATH - 1);
    strncpy(ex->extract_dir, extract_dir ? extract_dir : TX_EXTRACT_DIR,
            TX_MAX_PATH - 1);
    strncpy(ex->install_prefix, install_prefix ? install_prefix : TX_PREFIX,
            TX_MAX_PATH - 1);

    tx_manifest_init(&ex->manifest);

    return TX_OK;
}

void tx_extract_free(tx_extract_t *ex)
{
    if (!ex) return;
    tx_manifest_free(&ex->manifest);
    memset(ex, 0, sizeof(*ex));
}

tx_status_t tx_extract_archive(tx_extract_t *ex)
{
    if (!ex) return TX_ERROR_INVALID_ARG;

    /* Verify package exists */
    if (access(ex->package_path, F_OK) != 0) {
        TX_ERROR_SET(TX_ERROR_IO, "Package file not found",
                     ex->package_path, strerror(errno),
                     "The package may have been deleted or "
                     "the cache corrupted. Run 'pkg clean' and retry.");
        return TX_ERROR_IO;
    }

    /* Clean and create extraction directory */
    char cmd[TX_MAX_PATH * 2 + 64];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' && mkdir -p '%s'",
             ex->extract_dir, ex->extract_dir);

    if (system(cmd) != 0) {
        TX_ERROR_SET(TX_ERROR_IO, "Cannot create extraction directory",
                     ex->extract_dir, strerror(errno),
                     "Check permissions and disk space.");
        return TX_ERROR_IO;
    }

    /* Extract archive */
    snprintf(cmd, sizeof(cmd),
             "tar -xf '%s' -C '%s' 2>/dev/null",
             ex->package_path, ex->extract_dir);

    if (system(cmd) != 0) {
        TX_ERROR_SET(TX_ERROR_IO, "Failed to extract package",
                     ex->package_path,
                     "tar command failed",
                     "The package may be corrupted. "
                     "Run 'pkg clean' and retry, or re-download.");
        return TX_ERROR_IO;
    }

    ex->is_extracted = true;

    /* Read metadata */
    tx_extract_read_manifest(ex);
    tx_extract_read_control(ex);

    return TX_OK;
}

tx_status_t tx_extract_read_manifest(tx_extract_t *ex)
{
    if (!ex || !ex->is_extracted)
        return TX_ERROR_INVALID_ARG;

    char path[TX_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s",
             ex->extract_dir, TX_MANIFEST_FILE);

    return tx_parse_manifest_json(path, &ex->manifest);
}

tx_status_t tx_extract_read_control(tx_extract_t *ex)
{
    if (!ex || !ex->is_extracted)
        return TX_ERROR_INVALID_ARG;

    char path[TX_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s",
             ex->extract_dir, TX_CONTROL_FILE);

    if (access(path, F_OK) != 0)
        return TX_ERROR_NOT_FOUND;

    return tx_parse_control_file(path, &ex->manifest);
}

tx_status_t tx_extract_validate(tx_extract_t *ex)
{
    if (!ex || !ex->is_extracted)
        return TX_ERROR_INVALID_ARG;

    /* Check for manifest or CONTROL */
    char manifest_path[TX_MAX_PATH];
    char control_path[TX_MAX_PATH];

    snprintf(manifest_path, sizeof(manifest_path), "%s/%s",
             ex->extract_dir, TX_MANIFEST_FILE);
    snprintf(control_path, sizeof(control_path), "%s/%s",
             ex->extract_dir, TX_CONTROL_FILE);

    if (access(manifest_path, F_OK) != 0 &&
        access(control_path, F_OK) != 0) {
        TX_ERROR_SET(TX_ERROR_VERIFY,
            "Package has no manifest or CONTROL file",
            ex->extract_dir, NULL,
            "The package archive is malformed. Contact the "
            "repository maintainer.");
        return TX_ERROR_VERIFY;
    }

    /* Check for payload */
    char payload_path[TX_MAX_PATH];
    snprintf(payload_path, sizeof(payload_path), "%s/files",
             ex->extract_dir);

    if (access(payload_path, F_OK) != 0) {
        /* Files might be at root level */
        snprintf(payload_path, sizeof(payload_path), "%s/usr",
                 ex->extract_dir);
        if (access(payload_path, F_OK) != 0) {
            TX_ERROR_SET(TX_ERROR_VERIFY,
                "Package has no files/ payload directory",
                ex->extract_dir, NULL,
                "The package archive may be empty or malformed.");
            return TX_ERROR_VERIFY;
        }
    }

    /* Check paths for traversal attacks */
    return tx_extract_check_paths(ex);
}

tx_status_t tx_extract_check_paths(tx_extract_t *ex)
{
    if (!ex || !ex->is_extracted)
        return TX_ERROR_INVALID_ARG;

    /* Walk all files in extraction directory */
    char cmd[TX_MAX_PATH * 2 + 128];
    snprintf(cmd, sizeof(cmd),
             "find '%s' -print 2>/dev/null", ex->extract_dir);

    FILE *fp = popen(cmd, "r");
    if (!fp) return TX_ERROR_IO;

    char line[TX_MAX_PATH];
    while (fgets(line, sizeof(line), fp)) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        /* Skip the extraction directory itself */
        if (strcmp(line, ex->extract_dir) == 0)
            continue;

        /* Check for '..' in path */
        if (strstr(line, "..")) {
            pclose(fp);
            TX_ERROR_SET(TX_ERROR_VERIFY,
                "Package contains unsafe path with '..'",
                line, NULL,
                "The package may be malicious. Rejecting installation.");
            return TX_ERROR_VERIFY;
        }

        /* Check for absolute symlinks that escape */
        struct stat st;
        if (lstat(line, &st) == 0 && S_ISLNK(st.st_mode)) {
            char target[TX_MAX_PATH];
            ssize_t r = readlink(line, target, sizeof(target) - 1);
            if (r >= 0) {
                target[r] = '\0';
                if (target[0] == '/' &&
                    strncmp(target, ex->install_prefix,
                            strlen(ex->install_prefix)) != 0) {
                    pclose(fp);
                    TX_ERROR_SET(TX_ERROR_VERIFY,
                        "Package contains symlink escaping install prefix",
                        line, NULL,
                        "The package may be malicious. "
                        "Rejecting installation.");
                    return TX_ERROR_VERIFY;
                }
            }
        }
    }

    pclose(fp);
    return TX_OK;
}

tx_status_t tx_extract_install_files(tx_extract_t *ex)
{
    if (!ex || !ex->is_extracted)
        return TX_ERROR_INVALID_ARG;

    /* Determine source directory (files/ or root) */
    char src_dir[TX_MAX_PATH];
    snprintf(src_dir, sizeof(src_dir), "%s/files",
             ex->extract_dir);

    if (access(src_dir, F_OK) != 0) {
        /* Use root of extracted archive */
        strncpy(src_dir, ex->extract_dir, TX_MAX_PATH - 1);
    }

    /* Ensure install prefix exists */
    char mkdir_cmd[TX_MAX_PATH + 32];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd),
             "mkdir -p '%s'", ex->install_prefix);
    system(mkdir_cmd);

    /* Copy files to install prefix */
    char cmd[TX_MAX_PATH * 4 + 256];
    snprintf(cmd, sizeof(cmd),
             "cp -a '%s'/. '%s'/ 2>/dev/null",
             src_dir, ex->install_prefix);

    if (system(cmd) != 0) {
        TX_ERROR_SET(TX_ERROR_IO, "Failed to install package files",
                     ex->install_prefix, strerror(errno),
                     "Check disk space and permissions.");
        return TX_ERROR_IO;
    }

    return TX_OK;
}

tx_status_t tx_extract_run_script(tx_extract_t *ex,
    const char *script_name)
{
    if (!ex || !script_name)
        return TX_ERROR_INVALID_ARG;

    char script_path[TX_MAX_PATH];
    snprintf(script_path, sizeof(script_path), "%s/%s",
             ex->extract_dir, script_name);

    if (access(script_path, F_OK) != 0)
        return TX_OK;  /* No script - not an error */

    /* Validate script */
    if (access(script_path, X_OK) != 0) {
        if (chmod(script_path, 0755) != 0) {
            TX_ERROR_SET(TX_ERROR_PERMISSION,
                "Cannot make script executable",
                script_path, strerror(errno), NULL);
            return TX_ERROR_PERMISSION;
        }
    }

    /* Run script with install prefix as environment */
    char cmd[TX_MAX_PATH * 2 + 256];
    snprintf(cmd, sizeof(cmd),
             "cd '%s' && PREFIX='%s' '%s' 2>&1",
             ex->extract_dir, ex->install_prefix, script_path);

    int rc = system(cmd);
    if (rc != 0) {
        TX_ERROR_SET(TX_ERROR_GENERAL,
            "Script execution failed",
            script_name,
            "The script returned a non-zero exit code",
            "Check the package's install script for errors.");
        return TX_ERROR_GENERAL;
    }

    return TX_OK;
}

tx_status_t tx_extract_cleanup(tx_extract_t *ex)
{
    if (!ex) return TX_ERROR_INVALID_ARG;

    char cmd[TX_MAX_PATH + 32];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", ex->extract_dir);
    system(cmd);

    ex->is_extracted = false;
    return TX_OK;
}
