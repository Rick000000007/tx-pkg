/*
 * TX Package Manager v1.0 - Clean Command
 * Cleans the package cache.
 */

#include "cmd_clean.h"
#include "../config.h"
#include "../cache.h"
#include <stdio.h>
#include <string.h>

int cmd_clean(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    tx_cache_t cache;
    tx_cache_init(&cache, TX_PKG_CACHE);

    size_t before = tx_cache_get_size(&cache);

    if (argc >= 3 && strcmp(argv[2], "--all") == 0) {
        printf("Removing all cached packages...\n");
        tx_cache_clean_all(&cache);
    } else {
        printf("Removing expired cached packages...\n");
        tx_cache_clean_expired(&cache);
    }

    size_t after = tx_cache_get_size(&cache);
    size_t freed = (before > after) ? before - after : 0;

    printf("\nFreed %.2f MB of cache.\n", freed / (1024.0 * 1024.0));

    tx_cache_free(&cache);
    return 0;
}
