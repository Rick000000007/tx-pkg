/*
 * TX Package Manager v1.0 - Channel Command
 * Manages repository channels.
 */

#include "cmd_channel.h"
#include "../config.h"
#include "../repository.h"
#include <stdio.h>
#include <string.h>

int cmd_channel(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");

    printf("Current channel: %s\n\n", TX_DEFAULT_CHANNEL);

    printf("Available channels:\n");
    tx_repo_list_channels();

    printf("\nTo switch channels, update your repository configuration:\n");
    printf("  pkg repo add <name> <url> <channel>\n");

    return 0;
}
