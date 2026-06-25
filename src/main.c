/*
 * TX Package Manager v1.0 - Main Entry Point
 *
 * Command-line interface and command dispatcher.
 */

#include "config.h"
#include "common.h"
#include "error.h"
#include "selfupdate.h"

#include "commands/cmd_update.h"
#include "commands/cmd_search.h"
#include "commands/cmd_info.h"
#include "commands/cmd_list.h"
#include "commands/cmd_install.h"
#include "commands/cmd_remove.h"
#include "commands/cmd_reinstall.h"
#include "commands/cmd_upgrade.h"
#include "commands/cmd_autoremove.h"
#include "commands/cmd_clean.h"
#include "commands/cmd_verify.h"
#include "commands/cmd_doctor.h"
#include "commands/cmd_repo.h"
#include "commands/cmd_channel.h"

#include <stdio.h>
#include <string.h>

/* ==================================================================
 * Usage and Help
 * ================================================================== */

static void print_usage(const char *prog)
{
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    printf("======================\n\n");
    printf("Usage: %s <command> [args...]\n\n", prog);
    printf("Commands:\n");
    printf("  update              Update repository metadata\n");
    printf("  search <query>      Search for packages\n");
    printf("  info <package>      Show package information\n");
    printf("  list                List installed packages\n");
    printf("  install <pkg>       Install a package\n");
    printf("  remove <pkg>        Remove a package\n");
    printf("  reinstall <pkg>     Reinstall a package\n");
    printf("  upgrade [pkg]       Upgrade all or specific package\n");
    printf("  autoremove          Remove orphaned packages\n");
    printf("  clean [--all]       Clean package cache\n");
    printf("  verify [pkg]        Verify package integrity\n");
    printf("  doctor [--fix]      Diagnose and fix issues\n");
    printf("  repo <subcmd>       Manage repositories\n");
    printf("  channel             Show channel information\n");
    printf("  self-update         Update tx-pkg itself\n");
    printf("  --version           Show version information\n");
    printf("  --help              Show this help\n");
    printf("\nFor more information: %s\n", TX_PKG_HOMEPAGE);
}

static void print_version(void)
{
    printf("tx-pkg %s-%d\n", TX_PKG_VERSION, TX_PKG_RELEASE);
    printf("Architecture: %s\n", TX_DEFAULT_ARCH);
    printf("Channel: %s\n", TX_DEFAULT_CHANNEL);
    printf("License: %s\n", TX_PKG_LICENSE);
    printf("Homepage: %s\n", TX_PKG_HOMEPAGE);
}

/* ==================================================================
 * Main
 * ================================================================== */

int main(int argc, char *argv[])
{
    /* No arguments - show usage */
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    const char *cmd = argv[1];

    /* Global options */
    if (strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0) {
        print_version();
        return 0;
    }

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    /* Initialize error system */
    tx_error_clear();

    /* Ensure required directories exist */
    char mkdir_cmd[TX_MAX_PATH + 32];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd),
             "mkdir -p '%s' '%s' '%s' '%s' '%s' '%s'",
             TX_ROOT, TX_VAR_DIR, TX_LIB_DIR,
             TX_CACHE_DIR, TX_LOCK_DIR, TX_ETC_DIR);
    system(mkdir_cmd);

    /* Route to command handlers */
    if (strcmp(cmd, "update") == 0)
        return cmd_update(argc, argv);

    if (strcmp(cmd, "search") == 0)
        return cmd_search(argc, argv);

    if (strcmp(cmd, "info") == 0)
        return cmd_info(argc, argv);

    if (strcmp(cmd, "list") == 0)
        return cmd_list(argc, argv);

    if (strcmp(cmd, "install") == 0)
        return cmd_install(argc, argv);

    if (strcmp(cmd, "remove") == 0)
        return cmd_remove(argc, argv);

    if (strcmp(cmd, "reinstall") == 0)
        return cmd_reinstall(argc, argv);

    if (strcmp(cmd, "upgrade") == 0)
        return cmd_upgrade(argc, argv);

    if (strcmp(cmd, "autoremove") == 0)
        return cmd_autoremove(argc, argv);

    if (strcmp(cmd, "clean") == 0)
        return cmd_clean(argc, argv);

    if (strcmp(cmd, "verify") == 0)
        return cmd_verify(argc, argv);

    if (strcmp(cmd, "doctor") == 0)
        return cmd_doctor(argc, argv);

    if (strcmp(cmd, "repo") == 0)
        return cmd_repo(argc, argv);

    if (strcmp(cmd, "channel") == 0)
        return cmd_channel(argc, argv);

    if (strcmp(cmd, "self-update") == 0)
        return tx_selfupdate_perform() == TX_OK ? 0 : 1;

    /* Unknown command */
    printf("Unknown command: %s\n\n", cmd);
    print_usage(argv[0]);

    return 1;
}
