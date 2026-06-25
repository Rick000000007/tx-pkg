/*
 * TX Package Manager v1.0 - Rollback Engine Implementation
 */

#include "rollback.h"
#include "error.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

tx_status_t tx_rollback_init(tx_rollback_t *rb,
    const char *backup_dir, tx_db_t *db)
{
    if (!rb || !backup_dir)
        return TX_ERROR_INVALID_ARG;

    memset(rb, 0, sizeof(*rb));
    strncpy(rb->backup_dir, backup_dir, TX_MAX_PATH - 1);
    rb->db = db;

    char cmd[TX_MAX_PATH + 32];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", backup_dir);
    system(cmd);

    return TX_OK;
}

void tx_rollback_free(tx_rollback_t *rb)
{
    if (!rb) return;
    memset(rb, 0, sizeof(*rb));
}

tx_status_t tx_rollback_backup_package(tx_rollback_t *rb,
    const char *package_name)
{
    if (!rb || !package_name)
        return TX_ERROR_INVALID_ARG;

    /* Create package-specific backup directory */
    char pkg_backup[TX_MAX_PATH];
    snprintf(pkg_backup, sizeof(pkg_backup), "%s/%s",
             rb->backup_dir, package_name);

    char cmd[TX_MAX_PATH * 2 + 64];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", pkg_backup);
    system(cmd);

    /* Get installed files */
    if (!rb->db)
        return TX_ERROR_DATABASE;

    tx_file_entry_t *files = NULL;
    size_t count = 0;

    tx_status_t s = tx_db_package_files(rb->db, package_name,
                                        &files, &count);
    if (s != TX_OK) return s;

    /* Backup each file */
    for (size_t i = 0; i < count; i++) {
        if (access(files[i].path, F_OK) != 0)
            continue;

        /* Create relative path in backup */
        char *rel = files[i].path;
        if (rel[0] == '/') rel++;

        char backup_file[TX_MAX_PATH];
        snprintf(backup_file, sizeof(backup_file), "%s/%s",
                 pkg_backup, rel);

        /* Create directory structure */
        char backup_parent[TX_MAX_PATH];
        strncpy(backup_parent, backup_file, TX_MAX_PATH - 1);
        char *slash = strrchr(backup_parent, '/');
        if (slash) {
            *slash = '\0';
            char mkdir_cmd[TX_MAX_PATH + 32];
            snprintf(mkdir_cmd, sizeof(mkdir_cmd),
                     "mkdir -p '%s'", backup_parent);
            system(mkdir_cmd);
        }

        /* Copy file to backup */
        snprintf(cmd, sizeof(cmd), "cp -a '%s' '%s' 2>/dev/null",
                 files[i].path, backup_file);
        system(cmd);
    }

    free(files);
    return TX_OK;
}

tx_status_t tx_rollback_restore_package(tx_rollback_t *rb,
    const char *package_name)
{
    if (!rb || !package_name)
        return TX_ERROR_INVALID_ARG;

    char pkg_backup[TX_MAX_PATH];
    snprintf(pkg_backup, sizeof(pkg_backup), "%s/%s",
             rb->backup_dir, package_name);

    if (access(pkg_backup, F_OK) != 0)
        return TX_ERROR_NOT_FOUND;

    /* Restore all files from backup */
    char cmd[TX_MAX_PATH * 4 + 128];
    snprintf(cmd, sizeof(cmd),
             "cd '%s' && find . -type f -exec cp -a --parents '{}' '%s' \\;",
             pkg_backup, TX_PREFIX);

    system(cmd);

    return TX_OK;
}

tx_status_t tx_rollback_backup_file(tx_rollback_t *rb,
    const char *file_path)
{
    if (!rb || !file_path)
        return TX_ERROR_INVALID_ARG;

    if (access(file_path, F_OK) != 0)
        return TX_OK;  /* Nothing to backup */

    char *rel = (char *)file_path;
    if (rel[0] == '/') rel++;

    char backup_file[TX_MAX_PATH];
    snprintf(backup_file, sizeof(backup_file), "%s/files/%s",
             rb->backup_dir, rel);

    char backup_parent[TX_MAX_PATH];
    strncpy(backup_parent, backup_file, TX_MAX_PATH - 1);
    char *slash = strrchr(backup_parent, '/');
    if (slash) {
        *slash = '\0';
        char mkdir_cmd[TX_MAX_PATH + 32];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd),
                 "mkdir -p '%s'", backup_parent);
        system(mkdir_cmd);
    }

    char cmd[TX_MAX_PATH * 2 + 32];
    snprintf(cmd, sizeof(cmd), "cp -a '%s' '%s' 2>/dev/null",
             file_path, backup_file);
    system(cmd);

    return TX_OK;
}

tx_status_t tx_rollback_restore_file(tx_rollback_t *rb,
    const char *file_path)
{
    if (!rb || !file_path)
        return TX_ERROR_INVALID_ARG;

    char *rel = (char *)file_path;
    if (rel[0] == '/') rel++;

    char backup_file[TX_MAX_PATH];
    snprintf(backup_file, sizeof(backup_file), "%s/files/%s",
             rb->backup_dir, rel);

    if (access(backup_file, F_OK) != 0)
        return TX_ERROR_NOT_FOUND;

    /* Ensure parent directory exists */
    char parent[TX_MAX_PATH];
    strncpy(parent, file_path, TX_MAX_PATH - 1);
    char *slash = strrchr(parent, '/');
    if (slash) {
        *slash = '\0';
        char mkdir_cmd[TX_MAX_PATH + 32];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd),
                 "mkdir -p '%s'", parent);
        system(mkdir_cmd);
    }

    char cmd[TX_MAX_PATH * 2 + 32];
    snprintf(cmd, sizeof(cmd), "cp -a '%s' '%s'",
             backup_file, file_path);

    if (system(cmd) != 0)
        return TX_ERROR_IO;

    return TX_OK;
}

tx_status_t tx_rollback_backup_database(tx_rollback_t *rb)
{
    if (!rb || !rb->db)
        return TX_ERROR_INVALID_ARG;

    char backup_path[TX_MAX_PATH];
    snprintf(backup_path, sizeof(backup_path), "%s/txpkg.db.bak",
             rb->backup_dir);

    char cmd[TX_MAX_PATH * 2 + 32];
    snprintf(cmd, sizeof(cmd), "cp -a '%s' '%s' 2>/dev/null",
             rb->db->path, backup_path);
    system(cmd);

    return TX_OK;
}

tx_status_t tx_rollback_restore_database(tx_rollback_t *rb)
{
    if (!rb || !rb->db)
        return TX_ERROR_INVALID_ARG;

    char backup_path[TX_MAX_PATH];
    snprintf(backup_path, sizeof(backup_path), "%s/txpkg.db.bak",
             rb->backup_dir);

    if (access(backup_path, F_OK) != 0)
        return TX_ERROR_NOT_FOUND;

    /* Close current database */
    tx_db_close(rb->db);

    /* Restore from backup */
    char cmd[TX_MAX_PATH * 2 + 32];
    snprintf(cmd, sizeof(cmd), "cp -a '%s' '%s'",
             backup_path, rb->db->path);

    if (system(cmd) != 0)
        return TX_ERROR_IO;

    /* Reopen database */
    tx_db_open(rb->db, rb->db->path);

    return TX_OK;
}

tx_status_t tx_rollback_cleanup(tx_rollback_t *rb,
    const char *package_name)
{
    if (!rb || !package_name)
        return TX_ERROR_INVALID_ARG;

    char pkg_backup[TX_MAX_PATH];
    snprintf(pkg_backup, sizeof(pkg_backup), "%s/%s",
             rb->backup_dir, package_name);

    char cmd[TX_MAX_PATH + 32];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", pkg_backup);
    system(cmd);

    return TX_OK;
}
