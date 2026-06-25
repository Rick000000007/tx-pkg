/*
 * TX Package Manager v1.0 - SQLite Database Implementation
 */

#include "database.h"
#include "error.h"
#include <sys/stat.h>
#include <unistd.h>

#define TX_DB_SCHEMA_VERSION 1

/* ==================================================================
 * SQL Schema
 * ================================================================== */

static const char *schema_sql =
    "CREATE TABLE IF NOT EXISTS schema_version ("
    "    version INTEGER PRIMARY KEY"
    ");"
    "INSERT OR IGNORE INTO schema_version (version) VALUES (0);"

    "CREATE TABLE IF NOT EXISTS packages ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    name TEXT UNIQUE NOT NULL,"
    "    version TEXT NOT NULL,"
    "    release TEXT,"
    "    architecture TEXT,"
    "    description TEXT,"
    "    repository TEXT,"
    "    channel TEXT,"
    "    sha256 TEXT,"
    "    installed_size INTEGER DEFAULT 0,"
    "    is_essential INTEGER DEFAULT 0,"
    "    is_held INTEGER DEFAULT 0,"
    "    is_auto INTEGER DEFAULT 0,"
    "    status INTEGER DEFAULT 0,"
    "    install_date INTEGER,"
    "    update_date INTEGER"
    ");"

    "CREATE INDEX IF NOT EXISTS idx_pkg_name ON packages(name);"
    "CREATE INDEX IF NOT EXISTS idx_pkg_status ON packages(status);"

    "CREATE TABLE IF NOT EXISTS files ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    package_id INTEGER NOT NULL,"
    "    path TEXT UNIQUE NOT NULL,"
    "    sha256 TEXT,"
    "    size INTEGER DEFAULT 0,"
    "    mode INTEGER,"
    "    is_config INTEGER DEFAULT 0,"
    "    FOREIGN KEY (package_id) REFERENCES packages(id) "
    "        ON DELETE CASCADE"
    ");"

    "CREATE INDEX IF NOT EXISTS idx_file_path ON files(path);"
    "CREATE INDEX IF NOT EXISTS idx_file_pkg ON files(package_id);"

    "CREATE TABLE IF NOT EXISTS reverse_deps ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    package_name TEXT NOT NULL,"
    "    depends_on TEXT NOT NULL,"
    "    UNIQUE(package_name, depends_on)"
    ");"

    "CREATE INDEX IF NOT EXISTS idx_revdep_depends ON "
    "    reverse_deps(depends_on);"

    "CREATE TABLE IF NOT EXISTS transactions ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    tx_id TEXT NOT NULL,"
    "    operation TEXT NOT NULL,"
    "    package_name TEXT NOT NULL,"
    "    old_state TEXT,"
    "    new_state TEXT,"
    "    status TEXT DEFAULT 'pending',"
    "    timestamp INTEGER DEFAULT (strftime('%s','now'))"
    ");"

    "CREATE INDEX IF NOT EXISTS idx_tx_txid ON transactions(tx_id);"

    "PRAGMA foreign_keys = ON;"
    "PRAGMA journal_mode = WAL;";

static tx_status_t exec_sql(sqlite3 *db, const char *sql)
{
    char *errmsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg) {
            TX_ERROR_SET(TX_ERROR_DATABASE, "SQL execution failed",
                         sql, errmsg, "Database may be corrupted. "
                         "Run 'pkg doctor' to diagnose.");
            sqlite3_free(errmsg);
        }
        return TX_ERROR_DATABASE;
    }
    return TX_OK;
}

/* ==================================================================
 * Lifecycle
 * ================================================================== */

tx_status_t tx_db_open(tx_db_t *db, const char *path)
{
    if (!db || !path)
        return TX_ERROR_INVALID_ARG;

    memset(db, 0, sizeof(*db));
    strncpy(db->path, path, TX_MAX_PATH - 1);

    int rc = sqlite3_open(path, &db->sqlite);
    if (rc != SQLITE_OK) {
        TX_ERROR_SET(TX_ERROR_DATABASE, "Cannot open database",
                     path,
                     sqlite3_errmsg(db->sqlite),
                     "Check permissions and disk space.");
        sqlite3_close(db->sqlite);
        db->sqlite = NULL;
        return TX_ERROR_DATABASE;
    }

    db->is_open = true;

    /* Enable WAL mode for better concurrency */
    exec_sql(db->sqlite, "PRAGMA journal_mode = WAL;");
    exec_sql(db->sqlite, "PRAGMA foreign_keys = ON;");

    return tx_db_init_schema(db);
}

void tx_db_close(tx_db_t *db)
{
    if (!db) return;
    if (db->sqlite) {
        sqlite3_close(db->sqlite);
        db->sqlite = NULL;
    }
    db->is_open = false;
}

bool tx_db_is_open(const tx_db_t *db)
{
    return db && db->is_open && db->sqlite;
}

/* ==================================================================
 * Schema
 * ================================================================== */

tx_status_t tx_db_init_schema(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    tx_status_t s = exec_sql(db->sqlite, schema_sql);
    if (s != TX_OK)
        return s;

    /* Update schema version */
    char sql[128];
    snprintf(sql, sizeof(sql),
             "UPDATE schema_version SET version = %d;",
             TX_DB_SCHEMA_VERSION);
    return exec_sql(db->sqlite, sql);
}

int tx_db_schema_version(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return -1;

    sqlite3_stmt *stmt;
    int version = 0;

    if (sqlite3_prepare_v2(db->sqlite,
            "SELECT version FROM schema_version LIMIT 1",
            -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            version = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    return version;
}

/* ==================================================================
 * Package Registration
 * ================================================================== */

tx_status_t tx_db_register_package(tx_db_t *db,
    const tx_installed_pkg_t *pkg)
{
    if (!tx_db_is_open(db) || !pkg)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT OR REPLACE INTO packages "
        "(name, version, release, architecture, description, "
        " repository, channel, sha256, installed_size, "
        " is_essential, is_held, is_auto, status, "
        " install_date, update_date) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, pkg->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pkg->version.upstream, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pkg->version.release, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, pkg->architecture, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, pkg->description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, pkg->repository, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, pkg->channel, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, pkg->package_sha256, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 9, (sqlite3_int64)pkg->installed_size);
    sqlite3_bind_int(stmt, 10, pkg->is_essential ? 1 : 0);
    sqlite3_bind_int(stmt, 11, pkg->is_held ? 1 : 0);
    sqlite3_bind_int(stmt, 12, pkg->is_auto ? 1 : 0);
    sqlite3_bind_int(stmt, 13, pkg->status);
    sqlite3_bind_int64(stmt, 14, (sqlite3_int64)pkg->install_date);
    sqlite3_bind_int64(stmt, 15, (sqlite3_int64)pkg->update_date);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
        return TX_ERROR_DATABASE;

    /* Register files */
    for (size_t i = 0; i < pkg->file_count; i++) {
        tx_db_register_file(db, pkg->name, &pkg->files[i]);
    }

    return TX_OK;
}

tx_status_t tx_db_unregister_package(tx_db_t *db,
    const char *name)
{
    if (!tx_db_is_open(db) || !name)
        return TX_ERROR_INVALID_ARG;

    /* Remove files first (CASCADE will handle it, but be explicit) */
    tx_db_unregister_files(db, name);
    tx_db_remove_reverse_deps(db, name);

    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM packages WHERE name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_set_status(tx_db_t *db, const char *name,
    tx_pkg_status_t status)
{
    if (!tx_db_is_open(db) || !name)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE packages SET status = ? WHERE name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_set_version(tx_db_t *db, const char *name,
    const tx_version_t *version)
{
    if (!tx_db_is_open(db) || !name || !version)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE packages SET version = ?, release = ?, "
        "update_date = ? WHERE name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, version->upstream, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, version->release, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)time(NULL));
    sqlite3_bind_text(stmt, 4, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_set_held(tx_db_t *db, const char *name,
    bool held)
{
    if (!tx_db_is_open(db) || !name)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE packages SET is_held = ? WHERE name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_int(stmt, 1, held ? 1 : 0);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_set_auto(tx_db_t *db, const char *name,
    bool auto_flag)
{
    if (!tx_db_is_open(db) || !name)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE packages SET is_auto = ? WHERE name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_int(stmt, 1, auto_flag ? 1 : 0);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

/* ==================================================================
 * Package Queries
 * ================================================================== */

bool tx_db_is_installed(tx_db_t *db, const char *name)
{
    if (!tx_db_is_open(db) || !name)
        return false;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT 1 FROM packages WHERE name = ? "
        "AND status = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, TX_PKG_STATUS_INSTALLED);

    bool result = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    return result;
}

tx_status_t tx_db_get_package(tx_db_t *db, const char *name,
    tx_installed_pkg_t *pkg)
{
    if (!tx_db_is_open(db) || !name || !pkg)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT name, version, release, architecture, description, "
        "repository, channel, sha256, installed_size, "
        "is_essential, is_held, is_auto, status, "
        "install_date, update_date "
        "FROM packages WHERE name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    tx_status_t result = TX_ERROR_NOT_FOUND;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        tx_installed_pkg_init(pkg);

        strncpy(pkg->name, (const char *)sqlite3_column_text(stmt, 0),
                TX_MAX_PACKAGE_NAME - 1);
        tx_version_parse(&pkg->version,
            (const char *)sqlite3_column_text(stmt, 1));
        strncpy(pkg->version.release,
                (const char *)sqlite3_column_text(stmt, 2), 31);
        strncpy(pkg->architecture,
                (const char *)sqlite3_column_text(stmt, 3), 31);
        strncpy(pkg->description,
                (const char *)sqlite3_column_text(stmt, 4),
                TX_MAX_DESCRIPTION - 1);
        strncpy(pkg->repository,
                (const char *)sqlite3_column_text(stmt, 5),
                TX_MAX_PACKAGE_NAME - 1);
        strncpy(pkg->channel,
                (const char *)sqlite3_column_text(stmt, 6), 31);
        strncpy(pkg->package_sha256,
                (const char *)sqlite3_column_text(stmt, 7), 64);
        pkg->installed_size = (size_t)sqlite3_column_int64(stmt, 8);
        pkg->is_essential = sqlite3_column_int(stmt, 9) != 0;
        pkg->is_held = sqlite3_column_int(stmt, 10) != 0;
        pkg->is_auto = sqlite3_column_int(stmt, 11) != 0;
        pkg->status = sqlite3_column_int(stmt, 12);
        pkg->install_date = (time_t)sqlite3_column_int64(stmt, 13);
        pkg->update_date = (time_t)sqlite3_column_int64(stmt, 14);

        result = TX_OK;
    }

    sqlite3_finalize(stmt);
    return result;
}

tx_status_t tx_db_list_packages(tx_db_t *db,
    tx_installed_pkg_t **pkgs, size_t *count)
{
    if (!tx_db_is_open(db) || !pkgs || !count)
        return TX_ERROR_INVALID_ARG;

    *pkgs = NULL;
    *count = 0;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT COUNT(*) FROM packages WHERE status = ?;";

    size_t n = 0;
    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, TX_PKG_STATUS_INSTALLED);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            n = (size_t)sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    }

    if (n == 0)
        return TX_OK;

    *pkgs = calloc(n, sizeof(tx_installed_pkg_t));
    if (!*pkgs)
        return TX_ERROR_GENERAL;

    sql = "SELECT name FROM packages WHERE status = ? ORDER BY name;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(*pkgs);
        *pkgs = NULL;
        return TX_ERROR_DATABASE;
    }

    sqlite3_bind_int(stmt, 1, TX_PKG_STATUS_INSTALLED);

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        tx_db_get_package(db, name, &(*pkgs)[i]);
        i++;
    }

    sqlite3_finalize(stmt);
    *count = i;

    return TX_OK;
}

tx_pkg_status_t tx_db_get_status(tx_db_t *db, const char *name)
{
    if (!tx_db_is_open(db) || !name)
        return TX_PKG_STATUS_UNKNOWN;

    sqlite3_stmt *stmt;
    const char *sql = "SELECT status FROM packages WHERE name = ?;";

    tx_pkg_status_t status = TX_PKG_STATUS_UNKNOWN;

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            status = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    return status;
}

size_t tx_db_count_packages(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return 0;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT COUNT(*) FROM packages WHERE status = ?;";

    size_t count = 0;
    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, TX_PKG_STATUS_INSTALLED);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            count = (size_t)sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    }

    return count;
}

/* ==================================================================
 * File Ownership
 * ================================================================== */

tx_status_t tx_db_register_file(tx_db_t *db, const char *pkg_name,
    const tx_file_entry_t *file)
{
    if (!tx_db_is_open(db) || !pkg_name || !file)
        return TX_ERROR_INVALID_ARG;

    /* Get package ID */
    sqlite3_stmt *stmt;
    const char *id_sql = "SELECT id FROM packages WHERE name = ?;";

    int pkg_id = -1;
    if (sqlite3_prepare_v2(db->sqlite, id_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            pkg_id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    if (pkg_id < 0)
        return TX_ERROR_NOT_FOUND;

    const char *sql =
        "INSERT OR REPLACE INTO files "
        "(package_id, path, sha256, size, mode, is_config) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_int(stmt, 1, pkg_id);
    sqlite3_bind_text(stmt, 2, file->path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, file->sha256, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)file->size);
    sqlite3_bind_int(stmt, 5, (int)file->mode);
    sqlite3_bind_int(stmt, 6, file->is_config ? 1 : 0);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_unregister_files(tx_db_t *db,
    const char *pkg_name)
{
    if (!tx_db_is_open(db) || !pkg_name)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "DELETE FROM files WHERE package_id = "
        "(SELECT id FROM packages WHERE name = ?);";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_file_owner(tx_db_t *db, const char *path,
    char *pkg_name, size_t pkg_name_size)
{
    if (!tx_db_is_open(db) || !path || !pkg_name || pkg_name_size == 0)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT p.name FROM packages p "
        "JOIN files f ON p.id = f.package_id "
        "WHERE f.path = ?;";

    tx_status_t result = TX_ERROR_NOT_FOUND;

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            strncpy(pkg_name,
                    (const char *)sqlite3_column_text(stmt, 0),
                    pkg_name_size - 1);
            pkg_name[pkg_name_size - 1] = '\0';
            result = TX_OK;
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

tx_status_t tx_db_package_files(tx_db_t *db, const char *name,
    tx_file_entry_t **files, size_t *count)
{
    if (!tx_db_is_open(db) || !name || !files || !count)
        return TX_ERROR_INVALID_ARG;

    *files = NULL;
    *count = 0;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT f.path, f.sha256, f.size, f.mode, f.is_config "
        "FROM files f "
        "JOIN packages p ON f.package_id = p.id "
        "WHERE p.name = ? ORDER BY f.path;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        n++;
    sqlite3_finalize(stmt);

    if (n == 0)
        return TX_OK;

    *files = calloc(n, sizeof(tx_file_entry_t));
    if (!*files)
        return TX_ERROR_GENERAL;

    /* Re-query to get data */
    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(*files);
        *files = NULL;
        return TX_ERROR_DATABASE;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        strncpy((*files)[i].path,
                (const char *)sqlite3_column_text(stmt, 0),
                TX_MAX_PATH - 1);
        strncpy((*files)[i].sha256,
                (const char *)sqlite3_column_text(stmt, 1), 64);
        (*files)[i].size = (size_t)sqlite3_column_int64(stmt, 2);
        (*files)[i].mode = (mode_t)sqlite3_column_int(stmt, 3);
        (*files)[i].is_config = sqlite3_column_int(stmt, 4) != 0;
        i++;
    }

    sqlite3_finalize(stmt);
    *count = i;

    return TX_OK;
}

bool tx_db_path_is_safe(const char *path, const char *prefix)
{
    if (!path || !prefix)
        return false;

    /* Check for null bytes */
    if (strlen(path) != strnlen(path, TX_MAX_PATH))
        return false;

    /* Check for path traversal */
    if (strstr(path, "..") != NULL)
        return false;

    /* Must start with prefix */
    if (strncmp(path, prefix, strlen(prefix)) != 0)
        return false;

    /* No absolute paths outside prefix */
    if (path[0] == '/' && strncmp(path, prefix, strlen(prefix)) != 0)
        return false;

    return true;
}

/* ==================================================================
 * Reverse Dependencies
 * ================================================================== */

tx_status_t tx_db_add_reverse_dep(tx_db_t *db,
    const char *pkg_name, const char *depends_on)
{
    if (!tx_db_is_open(db) || !pkg_name || !depends_on)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT OR IGNORE INTO reverse_deps "
        "(package_name, depends_on) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, depends_on, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_remove_reverse_deps(tx_db_t *db,
    const char *pkg_name)
{
    if (!tx_db_is_open(db) || !pkg_name)
        return TX_ERROR_INVALID_ARG;

    sqlite3_stmt *stmt;
    const char *sql =
        "DELETE FROM reverse_deps WHERE package_name = ?;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? TX_OK : TX_ERROR_DATABASE;
}

tx_status_t tx_db_get_reverse_deps(tx_db_t *db,
    const char *pkg_name, char ***deps, size_t *count)
{
    if (!tx_db_is_open(db) || !pkg_name || !deps || !count)
        return TX_ERROR_INVALID_ARG;

    *deps = NULL;
    *count = 0;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT package_name FROM reverse_deps "
        "WHERE depends_on = ? ORDER BY package_name;";

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK)
        return TX_ERROR_DATABASE;

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);

    /* Count */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        n++;
    sqlite3_finalize(stmt);

    if (n == 0)
        return TX_OK;

    *deps = calloc(n, sizeof(char *));
    if (!*deps)
        return TX_ERROR_GENERAL;

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(*deps);
        *deps = NULL;
        return TX_ERROR_DATABASE;
    }

    sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        (*deps)[i] = strdup(name);
        i++;
    }

    sqlite3_finalize(stmt);
    *count = i;

    return TX_OK;
}

bool tx_db_is_required(tx_db_t *db, const char *pkg_name)
{
    if (!tx_db_is_open(db) || !pkg_name)
        return false;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT 1 FROM reverse_deps WHERE depends_on = ? LIMIT 1;";

    bool required = false;
    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, pkg_name, -1, SQLITE_STATIC);
        required = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
    }

    return required;
}

/* ==================================================================
 * Database Transactions
 * ================================================================== */

tx_status_t tx_db_begin(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    if (db->transaction_depth == 0) {
        tx_status_t s = exec_sql(db->sqlite, "BEGIN IMMEDIATE;");
        if (s != TX_OK) return s;
    }

    db->transaction_depth++;
    return TX_OK;
}

tx_status_t tx_db_commit(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    if (db->transaction_depth <= 0)
        return TX_ERROR_TRANSACTION;

    db->transaction_depth--;

    if (db->transaction_depth == 0)
        return exec_sql(db->sqlite, "COMMIT;");

    return TX_OK;
}

tx_status_t tx_db_rollback(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    db->transaction_depth = 0;
    return exec_sql(db->sqlite, "ROLLBACK;");
}

/* ==================================================================
 * Integrity and Maintenance
 * ================================================================== */

tx_status_t tx_db_integrity_check(tx_db_t *db, char **report)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    sqlite3_stmt *stmt;
    const char *sql = "PRAGMA integrity_check;";

    tx_status_t result = TX_OK;

    if (sqlite3_prepare_v2(db->sqlite, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *status = (const char *)sqlite3_column_text(stmt, 0);
            if (strcmp(status, "ok") != 0) {
                if (report)
                    *report = strdup(status);
                result = TX_ERROR_CORRUPTED;
            } else {
                if (report)
                    *report = strdup("ok");
            }
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

tx_status_t tx_db_vacuum(tx_db_t *db)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    return exec_sql(db->sqlite, "VACUUM;");
}

tx_status_t tx_db_stats(tx_db_t *db, size_t *pkg_count,
    size_t *file_count, size_t *db_size)
{
    if (!tx_db_is_open(db))
        return TX_ERROR_DATABASE;

    if (pkg_count) {
        sqlite3_stmt *stmt;
        *pkg_count = 0;
        if (sqlite3_prepare_v2(db->sqlite,
                "SELECT COUNT(*) FROM packages;",
                -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW)
                *pkg_count = (size_t)sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
        }
    }

    if (file_count) {
        sqlite3_stmt *stmt;
        *file_count = 0;
        if (sqlite3_prepare_v2(db->sqlite,
                "SELECT COUNT(*) FROM files;",
                -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW)
                *file_count = (size_t)sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
        }
    }

    if (db_size) {
        struct stat st;
        if (stat(db->path, &st) == 0)
            *db_size = (size_t)st.st_size;
        else
            *db_size = 0;
    }

    return TX_OK;
}
