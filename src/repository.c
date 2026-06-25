/*
 * TX Package Manager v1.0 - Repository Manager Implementation
 */

#include "repository.h"
#include "error.h"
#include "vendor/cjson.h"
#include <sys/stat.h>
#include <unistd.h>

/* ==================================================================
 * Default Channels
 * ================================================================== */

static const char *channel_names[] = {
    "stable", "testing", "nightly", "community", "local", "custom"
};

void tx_repo_list_channels(void)
{
    printf("Available channels:\n");
    for (size_t i = 0; i < TX_ARRAY_SIZE(channel_names); i++)
        printf("  %s\n", channel_names[i]);
}

const char *tx_repo_default_channel(void)
{
    return TX_DEFAULT_CHANNEL;
}

bool tx_repo_channel_is_valid(const char *channel)
{
    if (!channel) return false;
    for (size_t i = 0; i < TX_ARRAY_SIZE(channel_names); i++) {
        if (strcmp(channel, channel_names[i]) == 0)
            return true;
    }
    return false;
}

/* ==================================================================
 * Repository List
 * ================================================================== */

tx_status_t tx_repo_list_init(tx_repo_list_t *list)
{
    if (!list) return TX_ERROR_INVALID_ARG;

    memset(list, 0, sizeof(*list));
    list->capacity = TX_MAX_REPOSITORIES;
    list->repos = calloc(list->capacity, sizeof(tx_repository_t));
    if (!list->repos)
        return TX_ERROR_GENERAL;

    /* Try to load existing config */
    char config_path[TX_MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s", TX_REPO_CONFIG);

    if (access(config_path, F_OK) == 0) {
        tx_repo_list_load(list, config_path);
    } else {
        /* Setup defaults */
        tx_repo_setup_defaults(list);
    }

    return TX_OK;
}

void tx_repo_list_free(tx_repo_list_t *list)
{
    if (!list) return;
    free(list->repos);
    memset(list, 0, sizeof(*list));
}

/* ==================================================================
 * Configuration Load/Save
 * ================================================================== */

tx_status_t tx_repo_list_load(tx_repo_list_t *list, const char *config_path)
{
    if (!list || !config_path) return TX_ERROR_INVALID_ARG;

    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        /* Not an error if file doesn't exist */
        return TX_OK;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        fclose(fp);
        return TX_ERROR_GENERAL;
    }

    fread(json_str, 1, fsize, fp);
    json_str[fsize] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) return TX_ERROR_PARSE;

    cJSON *repos = cJSON_GetObjectItemCaseSensitive(root, "repositories");
    if (repos) {
        cJSON *repo;
        cJSON_ArrayForEach(repo, repos) {
            if (list->count >= list->capacity) break;

            cJSON *name = cJSON_GetObjectItemCaseSensitive(repo, "name");
            cJSON *url = cJSON_GetObjectItemCaseSensitive(repo, "url");
            cJSON *channel = cJSON_GetObjectItemCaseSensitive(repo, "channel");
            cJSON *priority = cJSON_GetObjectItemCaseSensitive(repo, "priority");
            cJSON *enabled = cJSON_GetObjectItemCaseSensitive(repo, "enabled");

            tx_repository_t *r = &list->repos[list->count++];
            if (name) strncpy(r->name, name->valuestring, TX_MAX_PACKAGE_NAME - 1);
            if (url) strncpy(r->url, url->valuestring, TX_MAX_URL - 1);
            if (channel) strncpy(r->channel, channel->valuestring, 31);
            if (priority) r->priority = priority->valueint;
            if (enabled) r->enabled = cJSON_IsTrue(enabled);
            strncpy(r->architecture, TX_DEFAULT_ARCH, 31);
        }
    }

    cJSON_Delete(root);
    return TX_OK;
}

tx_status_t tx_repo_list_save(const tx_repo_list_t *list, const char *config_path)
{
    if (!list || !config_path) return TX_ERROR_INVALID_ARG;

    /* Ensure directory exists */
    char dir[TX_MAX_PATH];
    strncpy(dir, config_path, TX_MAX_PATH - 1);
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        char cmd[TX_MAX_PATH + 32];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir);
        system(cmd);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *repos = cJSON_CreateArray();

    for (size_t i = 0; i < list->count; i++) {
        tx_repository_t *r = &list->repos[i];
        cJSON *repo = cJSON_CreateObject();
        cJSON_AddStringToObject(repo, "name", r->name);
        cJSON_AddStringToObject(repo, "url", r->url);
        cJSON_AddStringToObject(repo, "channel", r->channel);
        cJSON_AddNumberToObject(repo, "priority", r->priority);
        cJSON_AddBoolToObject(repo, "enabled", r->enabled);
        cJSON_AddItemToArray(repos, repo);
    }

    cJSON_AddItemToObject(root, "repositories", repos);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) return TX_ERROR_GENERAL;

    FILE *fp = fopen(config_path, "w");
    if (!fp) {
        free(json_str);
        return TX_ERROR_IO;
    }

    fprintf(fp, "%s\n", json_str);
    fclose(fp);
    free(json_str);

    return TX_OK;
}

/* ==================================================================
 * Repository CRUD
 * ================================================================== */

tx_status_t tx_repo_add(tx_repo_list_t *list, const char *name,
    const char *url, const char *channel, int priority)
{
    if (!list || !name || !url) return TX_ERROR_INVALID_ARG;

    /* Check for duplicates */
    if (tx_repo_get(list, name)) {
        TX_ERROR_SET_SIMPLE(TX_ERROR_ALREADY_EXISTS,
            "Repository already exists");
        return TX_ERROR_ALREADY_EXISTS;
    }

    if (list->count >= list->capacity)
        return TX_ERROR_GENERAL;

    tx_repository_t *r = &list->repos[list->count++];
    strncpy(r->name, name, TX_MAX_PACKAGE_NAME - 1);
    strncpy(r->url, url, TX_MAX_URL - 1);
    strncpy(r->channel, channel ? channel : TX_DEFAULT_CHANNEL, 31);
    strncpy(r->architecture, TX_DEFAULT_ARCH, 31);
    r->priority = priority;
    r->enabled = true;
    r->trusted = true;

    /* Set local cache path */
    snprintf(r->local_cache_path, sizeof(r->local_cache_path),
             "%s/repos/%s", TX_CACHE_DIR, name);

    return TX_OK;
}

tx_status_t tx_repo_remove(tx_repo_list_t *list, const char *name)
{
    if (!list || !name) return TX_ERROR_INVALID_ARG;

    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->repos[i].name, name) == 0) {
            /* Shift remaining */
            for (size_t j = i; j < list->count - 1; j++)
                list->repos[j] = list->repos[j + 1];
            list->count--;
            memset(&list->repos[list->count], 0,
                   sizeof(tx_repository_t));
            return TX_OK;
        }
    }

    return TX_ERROR_NOT_FOUND;
}

tx_status_t tx_repo_set_enabled(tx_repo_list_t *list,
    const char *name, bool enabled)
{
    tx_repository_t *r = tx_repo_get(list, name);
    if (!r) return TX_ERROR_NOT_FOUND;
    r->enabled = enabled;
    return TX_OK;
}

tx_status_t tx_repo_set_priority(tx_repo_list_t *list,
    const char *name, int priority)
{
    tx_repository_t *r = tx_repo_get(list, name);
    if (!r) return TX_ERROR_NOT_FOUND;
    r->priority = priority;
    return TX_OK;
}

tx_status_t tx_repo_set_channel(tx_repo_list_t *list,
    const char *name, const char *channel)
{
    if (!channel || !tx_repo_channel_is_valid(channel))
        return TX_ERROR_INVALID_ARG;

    tx_repository_t *r = tx_repo_get(list, name);
    if (!r) return TX_ERROR_NOT_FOUND;
    strncpy(r->channel, channel, 31);
    return TX_OK;
}

tx_repository_t *tx_repo_get(tx_repo_list_t *list, const char *name)
{
    if (!list || !name) return NULL;

    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->repos[i].name, name) == 0)
            return &list->repos[i];
    }
    return NULL;
}

/* ==================================================================
 * Metadata Operations
 * ================================================================== */

tx_status_t tx_repo_update(tx_repo_list_t *list)
{
    if (!list) return TX_ERROR_INVALID_ARG;

    printf("Updating repositories...\n");

    size_t updated = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (!list->repos[i].enabled)
            continue;

        printf("  [%s] %s (%s)... ",
               list->repos[i].channel,
               list->repos[i].name,
               list->repos[i].url);
        fflush(stdout);

        tx_status_t s = tx_repo_update_one(&list->repos[i]);
        if (s == TX_OK) {
            printf("OK\n");
            updated++;
        } else {
            printf("FAILED\n");
        }
    }

    printf("Updated %zu/%zu repositories.\n", updated, list->count);
    return TX_OK;
}

tx_status_t tx_repo_update_one(tx_repository_t *repo)
{
    if (!repo || !repo->enabled) return TX_ERROR_INVALID_ARG;

    /* Ensure cache directory exists */
    char mkdir_cmd[TX_MAX_PATH + 32];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd),
             "mkdir -p %s", repo->local_cache_path);
    system(mkdir_cmd);

    /* Build download URL */
    char url[TX_MAX_URL * 2];
    snprintf(url, sizeof(url), "%s/repo/%s/%s",
             repo->url, repo->channel, TX_REPO_PACKAGES_FILE);

    /* Build local cache path */
    char cache_file[TX_MAX_PATH];
    snprintf(cache_file, sizeof(cache_file), "%s/%s",
             repo->local_cache_path, TX_REPO_PACKAGES_FILE);

    /* Download with curl */
    char cmd[TX_MAX_URL + TX_MAX_PATH + 256];
    snprintf(cmd, sizeof(cmd),
             "curl -sL -f --connect-timeout 30 "
             "--max-time 120 -o '%s' '%s' 2>/dev/null",
             cache_file, url);

    int rc = system(cmd);
    if (rc != 0) {
        TX_ERROR_SET(TX_ERROR_NETWORK, "Failed to download repository",
                     url, "curl returned non-zero exit code",
                     "Check network connection and repository URL.");
        return TX_ERROR_NETWORK;
    }

    /* Also download SHA256SUMS if available */
    snprintf(url, sizeof(url), "%s/repo/%s/%s",
             repo->url, repo->channel, TX_REPO_CHECKSUMS_FILE);
    snprintf(cache_file, sizeof(cache_file), "%s/%s",
             repo->local_cache_path, TX_REPO_CHECKSUMS_FILE);
    snprintf(cmd, sizeof(cmd),
             "curl -sL -f --connect-timeout 30 "
             "--max-time 60 -o '%s' '%s' 2>/dev/null",
             cache_file, url);
    system(cmd);  /* Best effort, don't fail if not present */

    /* Also download index.json */
    snprintf(url, sizeof(url), "%s/repo/%s/%s",
             repo->url, repo->channel, TX_REPO_INDEX_FILE);
    snprintf(cache_file, sizeof(cache_file), "%s/%s",
             repo->local_cache_path, TX_REPO_INDEX_FILE);
    snprintf(cmd, sizeof(cmd),
             "curl -sL -f --connect-timeout 30 "
             "--max-time 60 -o '%s' '%s' 2>/dev/null",
             cache_file, url);
    system(cmd);

    repo->last_updated = time(NULL);

    return TX_OK;
}

tx_status_t tx_repo_parse_packages(tx_repository_t *repo,
    tx_pkg_entry_t **entries, size_t *count)
{
    if (!repo || !entries || !count)
        return TX_ERROR_INVALID_ARG;

    *entries = NULL;
    *count = 0;

    char cache_file[TX_MAX_PATH];
    snprintf(cache_file, sizeof(cache_file), "%s/%s",
             repo->local_cache_path, TX_REPO_PACKAGES_FILE);

    FILE *fp = fopen(cache_file, "r");
    if (!fp) return TX_ERROR_IO;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(fp);
        return TX_ERROR_IO;
    }

    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        fclose(fp);
        return TX_ERROR_GENERAL;
    }

    fread(json_str, 1, fsize, fp);
    json_str[fsize] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) return TX_ERROR_PARSE;

    cJSON *packages = cJSON_GetObjectItemCaseSensitive(root, "packages");
    if (!packages || !cJSON_IsArray(packages)) {
        cJSON_Delete(root);
        return TX_ERROR_PARSE;
    }

    size_t n = cJSON_GetArraySize(packages);
    if (n == 0) {
        cJSON_Delete(root);
        return TX_OK;
    }

    *entries = calloc(n, sizeof(tx_pkg_entry_t));
    if (!*entries) {
        cJSON_Delete(root);
        return TX_ERROR_GENERAL;
    }

    size_t i = 0;
    cJSON *pkg;
    cJSON_ArrayForEach(pkg, packages) {
        if (i >= n) break;

        tx_pkg_entry_t *e = &(*entries)[i];
        cJSON *item;

        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "name")))
            strncpy(e->name, item->valuestring, TX_MAX_PACKAGE_NAME - 1);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "version")))
            tx_version_parse(&e->version, item->valuestring);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "release")) &&
            cJSON_IsNumber(item)) {
            char ver[TX_MAX_VERSION * 2];
            snprintf(ver, sizeof(ver), "%s-%d",
                     e->version.upstream, item->valueint);
            tx_version_parse(&e->version, ver);
        }
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "architecture")))
            strncpy(e->architecture, item->valuestring, 31);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "category")))
            strncpy(e->category, item->valuestring, 63);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "description")))
            strncpy(e->description, item->valuestring, TX_MAX_DESCRIPTION - 1);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "filename")))
            strncpy(e->filename, item->valuestring, TX_MAX_FILENAME - 1);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "sha256")))
            strncpy(e->sha256, item->valuestring, 64);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "size")))
            e->size = (size_t)item->valueint;
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "installed_size")))
            e->installed_size = (size_t)item->valueint;
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "depends")))
            strncpy(e->depends, item->valuestring, TX_MAX_DEPENDENCY - 1);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "provides")))
            strncpy(e->provides, item->valuestring, TX_MAX_DEPENDENCY - 1);
        if ((item = cJSON_GetObjectItemCaseSensitive(pkg, "conflicts")))
            strncpy(e->conflicts, item->valuestring, TX_MAX_DEPENDENCY - 1);

        strncpy(e->repository, repo->name, TX_MAX_PACKAGE_NAME - 1);
        strncpy(e->channel, repo->channel, 31);
        strncpy(e->architecture, repo->architecture, 31);

        i++;
    }

    cJSON_Delete(root);
    *count = i;

    return TX_OK;
}

tx_status_t tx_repo_get_all_packages(tx_repo_list_t *list,
    tx_pkg_entry_t **entries, size_t *count)
{
    if (!list || !entries || !count)
        return TX_ERROR_INVALID_ARG;

    *entries = NULL;
    *count = 0;

    /* First pass: count total packages */
    size_t total = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (!list->repos[i].enabled) continue;

        tx_pkg_entry_t *e = NULL;
        size_t n = 0;
        tx_repo_parse_packages(&list->repos[i], &e, &n);
        total += n;
        free(e);
    }

    if (total == 0)
        return TX_OK;

    *entries = calloc(total, sizeof(tx_pkg_entry_t));
    if (!*entries)
        return TX_ERROR_GENERAL;

    /* Second pass: populate */
    size_t offset = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (!list->repos[i].enabled) continue;

        tx_pkg_entry_t *e = NULL;
        size_t n = 0;
        if (tx_repo_parse_packages(&list->repos[i], &e, &n) == TX_OK) {
            for (size_t j = 0; j < n && offset < total; j++)
                (*entries)[offset++] = e[j];
            free(e);
        }
    }

    *count = offset;
    return TX_OK;
}

tx_pkg_entry_t *tx_repo_find_package(tx_repo_list_t *list,
    const char *name, const char *channel)
{
    if (!list || !name) return NULL;

    tx_pkg_entry_t *best = NULL;
    int best_priority = -1;

    for (size_t i = 0; i < list->count; i++) {
        if (!list->repos[i].enabled) continue;
        if (channel && strcmp(list->repos[i].channel, channel) != 0)
            continue;

        tx_pkg_entry_t *entries = NULL;
        size_t count = 0;
        if (tx_repo_parse_packages(&list->repos[i], &entries, &count) != TX_OK)
            continue;

        for (size_t j = 0; j < count; j++) {
            if (strcmp(entries[j].name, name) == 0) {
                if (list->repos[i].priority > best_priority) {
                    best_priority = list->repos[i].priority;
                    /* Return a copy - caller must free */
                    tx_pkg_entry_t *copy = malloc(sizeof(tx_pkg_entry_t));
                    if (copy) memcpy(copy, &entries[j], sizeof(*copy));
                    free(entries);
                    return copy;
                }
            }
        }

        free(entries);
    }

    return best;
}

/* ==================================================================
 * Utility
 * ================================================================== */

void tx_repo_list_print(const tx_repo_list_t *list)
{
    if (!list) return;

    printf("\nConfigured repositories:\n");
    printf("========================\n\n");

    if (list->count == 0) {
        printf("(none)\n");
        return;
    }

    for (size_t i = 0; i < list->count; i++) {
        tx_repository_t *r = &list->repos[i];
        printf("  [%c] %-20s %-8s prio=%d  %s\n",
               r->enabled ? 'E' : 'D',
               r->name,
               r->channel,
               r->priority,
               r->url);
    }

    printf("\n  Legend: [E] Enabled  [D] Disabled\n");
}

tx_status_t tx_repo_setup_defaults(tx_repo_list_t *list)
{
    if (!list) return TX_ERROR_INVALID_ARG;

    tx_repo_add(list, "tx-main",
                TX_DEFAULT_REPO_URL,
                TX_DEFAULT_CHANNEL, 100);

    return TX_OK;
}
