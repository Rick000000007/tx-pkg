/*
 * TX Package Manager v1.0 - Repository Manager
 *
 * Manages multiple package repositories with support for:
 *   - Multiple repository sources
 *   - Channel management (stable, testing, nightly, etc.)
 *   - Repository priorities
 *   - Metadata download and caching
 *   - Incremental updates
 */

#ifndef TX_REPOSITORY_H
#define TX_REPOSITORY_H

#include "common.h"
#include "package.h"

/* ==================================================================
 * Repository Channel
 * ================================================================== */

typedef enum {
    TX_CHANNEL_STABLE = 0,
    TX_CHANNEL_TESTING = 1,
    TX_CHANNEL_NIGHTLY = 2,
    TX_CHANNEL_COMMUNITY = 3,
    TX_CHANNEL_LOCAL = 4,
    TX_CHANNEL_CUSTOM = 5,
} tx_channel_type_t;

/* ==================================================================
 * Repository Definition
 * ================================================================== */

typedef struct tx_repository {
    char name[TX_MAX_PACKAGE_NAME];
    char url[TX_MAX_URL];
    char channel[32];
    char architecture[32];
    int priority;               /* Higher = preferred */
    bool enabled;
    bool trusted;

    /* Cached metadata */
    char local_cache_path[TX_MAX_PATH];
    time_t last_updated;
    char fingerprint[65];       /* For future signature support */
} tx_repository_t;

/* ==================================================================
 * Repository List
 * ================================================================== */

typedef struct tx_repo_list {
    tx_repository_t *repos;
    size_t count;
    size_t capacity;
} tx_repo_list_t;

/* ==================================================================
 * Repository Lifecycle
 * ================================================================== */

/**
 * Initialize repository list (loads from config).
 */
tx_status_t tx_repo_list_init(tx_repo_list_t *list);

/**
 * Free repository list.
 */
void tx_repo_list_free(tx_repo_list_t *list);

/**
 * Load repository configuration.
 */
tx_status_t tx_repo_list_load(tx_repo_list_t *list, const char *config_path);

/**
 * Save repository configuration.
 */
tx_status_t tx_repo_list_save(const tx_repo_list_t *list, const char *config_path);

/* ==================================================================
 * Repository Management
 * ================================================================== */

/**
 * Add a new repository.
 */
tx_status_t tx_repo_add(tx_repo_list_t *list, const char *name,
    const char *url, const char *channel, int priority);

/**
 * Remove a repository.
 */
tx_status_t tx_repo_remove(tx_repo_list_t *list, const char *name);

/**
 * Enable/disable a repository.
 */
tx_status_t tx_repo_set_enabled(tx_repo_list_t *list,
    const char *name, bool enabled);

/**
 * Set repository priority.
 */
tx_status_t tx_repo_set_priority(tx_repo_list_t *list,
    const char *name, int priority);

/**
 * Set repository channel.
 */
tx_status_t tx_repo_set_channel(tx_repo_list_t *list,
    const char *name, const char *channel);

/**
 * Get a repository by name.
 */
tx_repository_t *tx_repo_get(tx_repo_list_t *list, const char *name);

/* ==================================================================
 * Metadata Operations
 * ================================================================== */

/**
 * Download repository metadata (Packages.json) for all enabled repos.
 */
tx_status_t tx_repo_update(tx_repo_list_t *list);

/**
 * Download metadata for a single repository.
 */
tx_status_t tx_repo_update_one(tx_repository_t *repo);

/**
 * Parse downloaded Packages.json into package entries.
 */
tx_status_t tx_repo_parse_packages(tx_repository_t *repo,
    tx_pkg_entry_t **entries, size_t *count);

/**
 * Get all available packages from all enabled repositories.
 */
tx_status_t tx_repo_get_all_packages(tx_repo_list_t *list,
    tx_pkg_entry_t **entries, size_t *count);

/**
 * Find a package across all enabled repositories.
 */
tx_pkg_entry_t *tx_repo_find_package(tx_repo_list_t *list,
    const char *name, const char *channel);

/* ==================================================================
 * Channel Management
 * ================================================================== */

/**
 * List available channels.
 */
void tx_repo_list_channels(void);

/**
 * Get default channel.
 */
const char *tx_repo_default_channel(void);

/**
 * Validate channel name.
 */
bool tx_repo_channel_is_valid(const char *channel);

/* ==================================================================
 * Utility
 * ================================================================== */

/**
 * Print repository list.
 */
void tx_repo_list_print(const tx_repo_list_t *list);

/**
 * Initialize default repositories.
 */
tx_status_t tx_repo_setup_defaults(tx_repo_list_t *list);

#endif /* TX_REPOSITORY_H */
