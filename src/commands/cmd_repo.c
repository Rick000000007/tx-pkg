/*
 * TX Package Manager v1.0 - Repo Command
 * Manages package repositories.
 */

#include "cmd_repo.h"
#include "../config.h"
#include "../repository.h"
#include "../error.h"
#include <stdio.h>
#include <string.h>

int cmd_repo(int argc, char *argv[])
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    tx_repo_list_t list;
    tx_repo_list_init(&list);

    if (argc < 3 || strcmp(argv[2], "list") == 0) {
        tx_repo_list_print(&list);
        tx_repo_list_free(&list);
        return 0;
    }

    if (strcmp(argv[2], "add") == 0) {
        if (argc < 5) {
            printf("Usage: pkg repo add <name> <url> [channel]\n");
            tx_repo_list_free(&list);
            return 1;
        }

        const char *name = argv[3];
        const char *url = argv[4];
        const char *channel = (argc >= 6) ? argv[5] : TX_DEFAULT_CHANNEL;

        tx_status_t s = tx_repo_add(&list, name, url, channel, 50);
        if (s == TX_OK) {
            char config_path[TX_MAX_PATH];
            snprintf(config_path, sizeof(config_path), "%s", TX_REPO_CONFIG);
            tx_repo_list_save(&list, config_path);
            printf("Repository '%s' added.\n", name);
        } else {
            printf("Failed to add repository: %s\n", tx_last_error.message);
        }

        tx_repo_list_free(&list);
        return (s == TX_OK) ? 0 : 1;
    }

    if (strcmp(argv[2], "remove") == 0) {
        if (argc < 4) {
            printf("Usage: pkg repo remove <name>\n");
            tx_repo_list_free(&list);
            return 1;
        }

        tx_status_t s = tx_repo_remove(&list, argv[3]);
        if (s == TX_OK) {
            char config_path[TX_MAX_PATH];
            snprintf(config_path, sizeof(config_path), "%s", TX_REPO_CONFIG);
            tx_repo_list_save(&list, config_path);
            printf("Repository '%s' removed.\n", argv[3]);
        } else {
            printf("Repository '%s' not found.\n", argv[3]);
        }

        tx_repo_list_free(&list);
        return (s == TX_OK) ? 0 : 1;
    }

    if (strcmp(argv[2], "enable") == 0) {
        if (argc < 4) {
            printf("Usage: pkg repo enable <name>\n");
            tx_repo_list_free(&list);
            return 1;
        }

        tx_repo_set_enabled(&list, argv[3], true);
        printf("Repository '%s' enabled.\n", argv[3]);
        tx_repo_list_free(&list);
        return 0;
    }

    if (strcmp(argv[2], "disable") == 0) {
        if (argc < 4) {
            printf("Usage: pkg repo disable <name>\n");
            tx_repo_list_free(&list);
            return 1;
        }

        tx_repo_set_enabled(&list, argv[3], false);
        printf("Repository '%s' disabled.\n", argv[3]);
        tx_repo_list_free(&list);
        return 0;
    }

    if (strcmp(argv[2], "priority") == 0) {
        if (argc < 5) {
            printf("Usage: pkg repo priority <name> <value>\n");
            tx_repo_list_free(&list);
            return 1;
        }

        int prio = atoi(argv[4]);
        tx_repo_set_priority(&list, argv[3], prio);
        printf("Repository '%s' priority set to %d.\n", argv[3], prio);
        tx_repo_list_free(&list);
        return 0;
    }

    printf("Unknown repo subcommand: %s\n", argv[2]);
    printf("Usage: pkg repo [list|add|remove|enable|disable|priority]\n");

    tx_repo_list_free(&list);
    return 1;
}
