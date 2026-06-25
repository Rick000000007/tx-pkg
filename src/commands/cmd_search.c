/*
 * TX Package Manager v1.0 - Search Command
 * Searches for packages across all enabled repositories.
 */

#include "cmd_search.h"
#include "../config.h"
#include "../repository.h"
#include "../package.h"
#include "../error.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

int cmd_search(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    if (argc < 3) {
        printf("Usage: pkg search <query>\n");
        printf("\nSearch for packages by name or description.\n");
        return 1;
    }

    const char *query = argv[2];

    printf("Searching for '%s'...\n\n", query);

    tx_repo_list_t list;
    tx_status_t s = tx_repo_list_init(&list);
    if (s != TX_OK) {
        fprintf(stderr, "Error: Cannot load repositories.\n");
        return 1;
    }

    tx_pkg_entry_t *entries = NULL;
    size_t count = 0;

    s = tx_repo_get_all_packages(&list, &entries, &count);
    if (s != TX_OK || count == 0) {
        printf("No packages found.\n");
        printf("Run 'pkg update' to refresh the package list.\n");
        tx_repo_list_free(&list);
        return 0;
    }

    /* Search */
    size_t matches = 0;
    for (size_t i = 0; i < count; i++) {
        if (strcasestr(entries[i].name, query) != NULL ||
            strcasestr(entries[i].description, query) != NULL) {
            if (matches == 0)
                printf("%-20s %-12s %s\n",
                       "Package", "Version", "Description");
            printf("%-20s %-12s %s\n",
                   entries[i].name,
                   entries[i].version.upstream,
                   entries[i].description);
            matches++;
        }
    }

    free(entries);
    tx_repo_list_free(&list);

    printf("\n%zu match(es) found.\n", matches);
    return 0;
}
