/*
 * TX Package Manager v1.0 - Self Update Implementation
 */

#include "selfupdate.h"
#include "config.h"
#include "error.h"
#include <sys/stat.h>
#include <unistd.h>

#define TX_PKG_UPDATE_URL  TX_DEFAULT_REPO_URL "/tx-pkg-latest"
#define TX_PKG_BACKUP      TX_VAR_DIR "/lib/txpkg/pkg.backup"

tx_status_t tx_selfupdate_check(bool *update_available,
    char *new_version, size_t ver_size)
{
    if (!update_available)
        return TX_ERROR_INVALID_ARG;

    *update_available = false;

    /* Download latest version info */
    char cmd[TX_MAX_URL + TX_MAX_PATH + 128];
    char tmpfile[TX_MAX_PATH];
    snprintf(tmpfile, sizeof(tmpfile), "%s/version-check",
             TX_CACHE_DIR);

    snprintf(cmd, sizeof(cmd),
             "curl -sL --connect-timeout 10 --max-time 30 "
             "-o '%s' '%s' 2>/dev/null",
             tmpfile, TX_PKG_UPDATE_URL "/version");

    if (system(cmd) != 0)
        return TX_ERROR_NETWORK;

    /* Read version */
    FILE *fp = fopen(tmpfile, "r");
    if (!fp) return TX_ERROR_IO;

    char remote_ver[64] = "";
    if (fgets(remote_ver, sizeof(remote_ver), fp))
        remote_ver[strcspn(remote_ver, "\n")] = '\0';
    fclose(fp);
    unlink(tmpfile);

    if (remote_ver[0] == '\0')
        return TX_ERROR_PARSE;

    if (new_version && ver_size > 0)
        strncpy(new_version, remote_ver, ver_size - 1);

    /* Compare versions */
    int cmp = strcmp(remote_ver, TX_PKG_VERSION);
    *update_available = (cmp > 0);

    return TX_OK;
}

tx_status_t tx_selfupdate_perform(void)
{
    printf("TX Package Manager Self-Update\n");
    printf("==============================\n\n");
    printf("Current version: %s\n", TX_PKG_VERSION);

    /* Check for update */
    bool available = false;
    char new_ver[64] = "";

    tx_status_t s = tx_selfupdate_check(&available, new_ver,
                                        sizeof(new_ver));
    if (s != TX_OK) {
        printf("Cannot check for updates.\n");
        return s;
    }

    if (!available) {
        printf("You are running the latest version.\n");
        return TX_OK;
    }

    printf("New version available: %s\n\n", new_ver);

    /* Download new binary */
    char tmp_binary[TX_MAX_PATH];
    snprintf(tmp_binary, sizeof(tmp_binary), "%s/pkg.new",
             TX_CACHE_DIR);

    printf("Downloading update...\n");
    char cmd[TX_MAX_URL + TX_MAX_PATH + 128];

#ifdef __aarch64__
    const char *arch = "aarch64";
#else
    const char *arch = "x86_64";
#endif

    snprintf(cmd, sizeof(cmd),
             "curl -L --connect-timeout 30 --max-time 120 "
             "-o '%s' '%s/%s/pkg' 2>/dev/null",
             tmp_binary, TX_PKG_UPDATE_URL, arch);

    if (system(cmd) != 0) {
        TX_ERROR_SET(TX_ERROR_NETWORK,
            "Failed to download update", NULL,
            "curl command failed",
            "Check your network connection.");
        return TX_ERROR_NETWORK;
    }

    /* Make executable */
    chmod(tmp_binary, 0755);

    /* Verify it's a valid binary */
    if (access(tmp_binary, X_OK) != 0) {
        unlink(tmp_binary);
        TX_ERROR_SET(TX_ERROR_VERIFY,
            "Downloaded file is not a valid executable", NULL,
            "The download may be corrupted",
            "Run 'pkg self-update' to retry.");
        return TX_ERROR_VERIFY;
    }

    /* Backup current binary */
    char current_binary[TX_MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", current_binary,
                           sizeof(current_binary) - 1);
    if (len < 0) {
        snprintf(current_binary, sizeof(current_binary),
                 "%s/pkg", TX_PREFIX_BIN);
    } else {
        current_binary[len] = '\0';
    }

    printf("Backing up current version...\n");
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s' 2>/dev/null",
             current_binary, TX_PKG_BACKUP);
    system(cmd);

    /* Atomic swap */
    printf("Installing update...\n");

    char tmp_path[TX_MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s.new", current_binary);

    if (rename(tmp_binary, tmp_path) != 0) {
        unlink(tmp_binary);
        TX_ERROR_SET(TX_ERROR_IO, "Cannot stage new binary",
                     tmp_path, strerror(errno), NULL);
        return TX_ERROR_IO;
    }

    if (rename(tmp_path, current_binary) != 0) {
        /* Try to restore */
        rename(tmp_path, tmp_binary);
        TX_ERROR_SET(TX_ERROR_IO, "Cannot install new binary",
                     current_binary, strerror(errno),
                     "Run 'pkg self-update --rollback' to restore.");
        return TX_ERROR_IO;
    }

    printf("\nUpdate installed successfully!\n");
    printf("New version: %s\n", new_ver);
    printf("Run 'pkg --version' to verify.\n");

    return TX_OK;
}

tx_status_t tx_selfupdate_rollback(void)
{
    printf("TX Package Manager Rollback\n");
    printf("===========================\n\n");

    if (access(TX_PKG_BACKUP, F_OK) != 0) {
        TX_ERROR_SET(TX_ERROR_NOT_FOUND,
            "No backup found to rollback to",
            TX_PKG_BACKUP, NULL,
            "No previous version backup exists.");
        return TX_ERROR_NOT_FOUND;
    }

    char current_binary[TX_MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", current_binary,
                           sizeof(current_binary) - 1);
    if (len < 0) {
        snprintf(current_binary, sizeof(current_binary),
                 "%s/pkg", TX_PREFIX_BIN);
    } else {
        current_binary[len] = '\0';
    }

    if (rename(TX_PKG_BACKUP, current_binary) != 0) {
        TX_ERROR_SET(TX_ERROR_IO, "Cannot restore backup",
                     current_binary, strerror(errno), NULL);
        return TX_ERROR_IO;
    }

    printf("Rollback complete.\n");
    return TX_OK;
}

const char *tx_selfupdate_current_version(void)
{
    return TX_PKG_VERSION;
}

void tx_selfupdate_print_status(void)
{
    printf("TX Package Manager\n");
    printf("==================\n\n");
    printf("Version:    %s\n", TX_PKG_VERSION);
    printf("Release:    %d\n", TX_PKG_RELEASE);
    printf("Channel:    %s\n", TX_DEFAULT_CHANNEL);
    printf("Location:   ");

    char path[TX_MAX_PATH];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len >= 0) {
        path[len] = '\0';
        printf("%s\n", path);
    } else {
        printf("%s/pkg\n", TX_PREFIX_BIN);
    }

    bool available = false;
    char new_ver[64] = "";

    if (tx_selfupdate_check(&available, new_ver, sizeof(new_ver)) == TX_OK
        && available) {
        printf("\nUpdate available: %s\n", new_ver);
        printf("Run 'pkg self-update' to upgrade.\n");
    }
}
