/*
 * TX Package Manager v1.0 - Update Command
 * Downloads repository metadata from all enabled repositories.
 */

#include "cmd_update.h"
#include "../config.h"
#include "../repository.h"
#include "../error.h"
#include <stdio.h>

int cmd_update(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");
    printf("Updating package lists...\n\n");

    tx_repo_list_t list;
    tx_status_t s = tx_repo_list_init(&list);
    if (s != TX_OK) {
        fprintf(stderr, "Error: Cannot initialize repository list.\n");
        tx_error_print();
        return 1;
    }

    s = tx_repo_update(&list);

    /* Save updated state */
    char config_path[TX_MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s", TX_REPO_CONFIG);
    tx_repo_list_save(&list, config_path);

    tx_repo_list_free(&list);

    printf("\nDone.\n");
    return (s == TX_OK) ? 0 : 1;
}
