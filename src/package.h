/*
 * TX Package Manager v1.0 - Package Data Structures
 *
 * Defines the core package representation including metadata,
 * dependencies, file lists, and status tracking.
 */

#ifndef TX_PACKAGE_H
#define TX_PACKAGE_H

#include "common.h"
#include "version.h"

/* ==================================================================
 * Dependency Declaration
 *
 * A single dependency relation from a package to another package.
 * Examples: "depends >= 1.0", "conflicts: old-name"
 * ================================================================== */

typedef struct tx_dependency {
    char name[TX_MAX_PACKAGE_NAME];
    tx_version_t version;
    tx_relation_t relation;
    tx_dep_kind_t kind;
    char description[512];
    struct tx_dependency *next;
} tx_dependency_t;

/* ==================================================================
 * File Entry
 *
 * Tracks a single installed file from a package.
 * ================================================================== */

typedef struct tx_file_entry {
    char path[TX_MAX_PATH];
    char sha256[65];
    size_t size;
    mode_t mode;
    uid_t owner;
    gid_t group;
    bool is_config;
    bool is_directory;
    time_t mtime;
} tx_file_entry_t;

/* ==================================================================
 * Package Manifest
 *
 * Complete metadata for a package as stored in manifest.json.
 * ================================================================== */

typedef struct tx_manifest {
    char name[TX_MAX_PACKAGE_NAME];
    tx_version_t version;
    char architecture[32];
    char license[64];
    char description[TX_MAX_DESCRIPTION];
    char homepage[TX_MAX_URL];
    char maintainer[256];
    char category[64];
    char section[64];
    char source_url[TX_MAX_URL];

    /* Dependencies - linked lists by kind */
    tx_dependency_t *depends;
    tx_dependency_t *pre_depends;
    tx_dependency_t *provides;
    tx_dependency_t *conflicts;
    tx_dependency_t *replaces;
    tx_dependency_t *breaks;
    tx_dependency_t *suggests;
    tx_dependency_t *recommends;
    tx_dependency_t *optional;
    tx_dependency_t *enhances;

    /* Script paths (within package archive) */
    char preinst[TX_MAX_PATH];
    char postinst[TX_MAX_PATH];
    char prerm[TX_MAX_PATH];
    char postrm[TX_MAX_PATH];

    /* File list */
    tx_file_entry_t *files;
    size_t file_count;
    size_t file_capacity;

    /* Checksums */
    char package_sha256[65];
    size_t package_size;
    size_t installed_size;

    /* Repository origin */
    char repository[TX_MAX_PACKAGE_NAME];
    char channel[32];
    char filename[TX_MAX_FILENAME];

    /* Flags */
    bool is_essential;
    bool is_virtual;
    bool is_installed;
    bool is_held;
    bool is_auto;
} tx_manifest_t;

/* ==================================================================
 * Installed Package Record
 *
 * Represents a package as installed in the local database.
 * ================================================================== */

typedef struct tx_installed_pkg {
    char name[TX_MAX_PACKAGE_NAME];
    tx_version_t version;
    char architecture[32];
    char description[TX_MAX_DESCRIPTION];
    tx_pkg_status_t status;
    time_t install_date;
    time_t update_date;
    char repository[TX_MAX_PACKAGE_NAME];
    char channel[32];
    char package_sha256[65];
    size_t installed_size;
    bool is_essential;
    bool is_held;
    bool is_auto;       /* auto-installed as dependency */

    /* File ownership */
    tx_file_entry_t *files;
    size_t file_count;

    /* Reverse dependencies (names of packages that depend on this) */
    char **reverse_deps;
    size_t reverse_dep_count;
} tx_installed_pkg_t;

/* ==================================================================
 * Package Database Entry (for repository queries)
 * ================================================================== */

typedef struct tx_pkg_entry {
    char name[TX_MAX_PACKAGE_NAME];
    tx_version_t version;
    char architecture[32];
    char category[64];
    char description[TX_MAX_DESCRIPTION];
    char filename[TX_MAX_FILENAME];
    char sha256[65];
    size_t size;
    size_t installed_size;
    char depends[TX_MAX_DEPENDENCY];
    char provides[TX_MAX_DEPENDENCY];
    char conflicts[TX_MAX_DEPENDENCY];
    char repository[TX_MAX_PACKAGE_NAME];
    char channel[32];
    time_t added_date;
} tx_pkg_entry_t;

/* ==================================================================
 * Package Operations
 * ================================================================== */

/**
 * Initialize a manifest structure.
 */
void tx_manifest_init(tx_manifest_t *m);

/**
 * Free all memory associated with a manifest.
 */
void tx_manifest_free(tx_manifest_t *m);

/**
 * Add a dependency to a manifest.
 */
tx_status_t tx_manifest_add_dependency(tx_manifest_t *m,
    tx_dep_kind_t kind, const char *name,
    tx_relation_t rel, const char *version_str);

/**
 * Get the dependency list of a specific kind from a manifest.
 */
tx_dependency_t *tx_manifest_get_deps(const tx_manifest_t *m,
    tx_dep_kind_t kind);

/**
 * Parse a dependency string like "pkg (>= 1.0) | otherpkg".
 * For complex disjunctions, returns the first alternative.
 */
tx_status_t tx_dep_parse(const char *str, tx_dependency_t *dep);

/**
 * Parse multiple comma-separated dependencies.
 */
tx_status_t tx_dep_parse_list(const char *str,
    tx_dep_kind_t kind, tx_manifest_t *m);

/**
 * Free a dependency list.
 */
void tx_dep_free_list(tx_dependency_t *deps);

/**
 * Copy a dependency.
 */
tx_status_t tx_dep_copy(tx_dependency_t *dst,
    const tx_dependency_t *src);

/**
 * Parse a relation operator string (>=, <=, =, >>, <<, !=).
 */
tx_relation_t tx_relation_parse(const char *str);

/**
 * Convert a relation to a string.
 */
const char *tx_relation_to_string(tx_relation_t rel);

/**
 * Initialize an installed package record.
 */
void tx_installed_pkg_init(tx_installed_pkg_t *pkg);

/**
 * Free an installed package record.
 */
void tx_installed_pkg_free(tx_installed_pkg_t *pkg);

/**
 * Convert a manifest to an installed package record.
 */
tx_status_t tx_manifest_to_installed(const tx_manifest_t *m,
    tx_installed_pkg_t *pkg);

/**
 * Parse CONTROL file (Debian-style key-value pairs).
 */
tx_status_t tx_parse_control_file(const char *path,
    tx_manifest_t *m);

/**
 * Parse manifest.json file.
 */
tx_status_t tx_parse_manifest_json(const char *path,
    tx_manifest_t *m);

/**
 * Write manifest.json file.
 */
tx_status_t tx_write_manifest_json(const char *path,
    const tx_manifest_t *m);

#endif /* TX_PACKAGE_H */
