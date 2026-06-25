/*
 * TX Package Manager v1.0 - Cache Manager Implementation
 */

#include "cache.h"
#include "error.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

tx_status_t tx_cache_init(tx_cache_t *cache, const char *cache_dir)
{
    if (!cache || !cache_dir)
        return TX_ERROR_INVALID_ARG;

    memset(cache, 0, sizeof(*cache));
    strncpy(cache->cache_dir, cache_dir, TX_MAX_PATH - 1);
    cache->max_size_mb = 1024;
    cache->max_age_days = TX_CACHE_MAX_AGE_DAYS;

    /* Create cache directory */
    char cmd[TX_MAX_PATH + 32];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", cache_dir);
    system(cmd);

    return TX_OK;
}

void tx_cache_free(tx_cache_t *cache)
{
    if (!cache) return;
    memset(cache, 0, sizeof(*cache));
}

bool tx_cache_has_package(tx_cache_t *cache, const char *filename)
{
    if (!cache || !filename) return false;

    char path[TX_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", cache->cache_dir, filename);

    return access(path, F_OK) == 0;
}

tx_status_t tx_cache_get_path(tx_cache_t *cache, const char *filename,
    char *path, size_t path_size)
{
    if (!cache || !filename || !path || path_size == 0)
        return TX_ERROR_INVALID_ARG;

    snprintf(path, path_size, "%s/%s", cache->cache_dir, filename);

    if (access(path, F_OK) != 0)
        return TX_ERROR_NOT_FOUND;

    return TX_OK;
}

tx_status_t tx_cache_add(tx_cache_t *cache, const char *source,
    const char *filename)
{
    if (!cache || !source || !filename)
        return TX_ERROR_INVALID_ARG;

    char dest[TX_MAX_PATH];
    snprintf(dest, sizeof(dest), "%s/%s", cache->cache_dir, filename);

    /* Use mv for efficiency */
    char cmd[TX_MAX_PATH * 2 + 32];
    snprintf(cmd, sizeof(cmd), "mv '%s' '%s' 2>/dev/null || "
             "cp '%s' '%s'", source, dest, source, dest);

    if (system(cmd) != 0)
        return TX_ERROR_IO;

    return TX_OK;
}

tx_status_t tx_cache_remove(tx_cache_t *cache, const char *filename)
{
    if (!cache || !filename)
        return TX_ERROR_INVALID_ARG;

    char path[TX_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", cache->cache_dir, filename);

    if (unlink(path) != 0 && errno != ENOENT)
        return TX_ERROR_IO;

    return TX_OK;
}

bool tx_cache_verify(tx_cache_t *cache, const char *filename,
    const char *expected_sha256)
{
    if (!cache || !filename || !expected_sha256)
        return false;

    char path[TX_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", cache->cache_dir, filename);

    if (access(path, F_OK) != 0)
        return false;

    /* Calculate SHA256 */
    char cmd[TX_MAX_PATH + 256];
    snprintf(cmd, sizeof(cmd), "sha256sum '%s' 2>/dev/null", path);

    FILE *fp = popen(cmd, "r");
    if (!fp) return false;

    char actual[65] = "";
    if (fscanf(fp, "%64s", actual) != 1) {
        pclose(fp);
        return false;
    }
    pclose(fp);

    return strcmp(actual, expected_sha256) == 0;
}

tx_status_t tx_cache_clean_expired(tx_cache_t *cache)
{
    if (!cache) return TX_ERROR_INVALID_ARG;

    DIR *dir = opendir(cache->cache_dir);
    if (!dir) return TX_ERROR_IO;

    time_t now = time(NULL);
    time_t max_age = cache->max_age_days * 24 * 3600;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        char path[TX_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s",
                 cache->cache_dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0) {
            if ((now - st.st_mtime) > max_age)
                unlink(path);
        }
    }

    closedir(dir);
    return TX_OK;
}

tx_status_t tx_cache_clean_all(tx_cache_t *cache)
{
    if (!cache) return TX_ERROR_INVALID_ARG;

    DIR *dir = opendir(cache->cache_dir);
    if (!dir) return TX_ERROR_IO;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        char path[TX_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s",
                 cache->cache_dir, entry->d_name);
        unlink(path);
    }

    closedir(dir);
    return TX_OK;
}

size_t tx_cache_get_size(tx_cache_t *cache)
{
    if (!cache) return 0;

    DIR *dir = opendir(cache->cache_dir);
    if (!dir) return 0;

    size_t total = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        char path[TX_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s",
                 cache->cache_dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0)
            total += (size_t)st.st_size;
    }

    closedir(dir);
    return total;
}

tx_status_t tx_cache_limit_size(tx_cache_t *cache, size_t max_bytes)
{
    if (!cache) return TX_ERROR_INVALID_ARG;

    while (tx_cache_get_size(cache) > max_bytes) {
        /* Find oldest file and remove it */
        DIR *dir = opendir(cache->cache_dir);
        if (!dir) return TX_ERROR_IO;

        char oldest[TX_MAX_PATH] = "";
        time_t oldest_time = 0;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;

            char path[TX_MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s",
                     cache->cache_dir, entry->d_name);

            struct stat st;
            if (stat(path, &st) == 0) {
                if (oldest_time == 0 || st.st_atime < oldest_time) {
                    oldest_time = st.st_atime;
                    strncpy(oldest, path, TX_MAX_PATH - 1);
                }
            }
        }
        closedir(dir);

        if (oldest[0])
            unlink(oldest);
        else
            break;
    }

    return TX_OK;
}

void tx_cache_stats(tx_cache_t *cache)
{
    if (!cache) return;

    size_t size = tx_cache_get_size(cache);
    size_t count = 0;

    DIR *dir = opendir(cache->cache_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.')
                count++;
        }
        closedir(dir);
    }

    printf("Cache directory: %s\n", cache->cache_dir);
    printf("Cached files:    %zu\n", count);
    printf("Total size:      %.2f MB\n", size / (1024.0 * 1024.0));
    printf("Max age:         %d days\n", cache->max_age_days);
}
