#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "db.h"
#include "list.h"
#include "install.h"
#include "search.h"
#include "info.h"
#include "remove.h"
#include "json.h"
#include "doctor.h"
#include "upgrade.h"
#include "stats.h"
#include "txrepo.h"
#include "txbuild.h"

static void usage(void)
{
    printf("TX Package Manager\n");
    printf("==================\n\n");

    printf("Usage:\n");
    printf("  pkg update\n");
    printf("  pkg search <package>\n");
    printf("  pkg info <package>\n");
    printf("  pkg install <package>\n");
    printf("  pkg list\n");
    printf("  pkg remove <package>\n");
    printf("  pkg doctor\n");
    printf("  pkg stats\n");
    printf("  pkg txrepo [path]\n");
    printf("  pkg txbuild [path]\n");
    printf("  pkg upgrade\n");
}

int main(int argc, char *argv[])
{
    db_init();

    if (argc == 2 && strcmp(argv[1], "json-test") == 0) {
        json_test();
        return 0;
    }

    if (argc < 2) {
        usage();
        return 0;
    }

    if (strcmp(argv[1], "update") == 0) {

        printf("TX Package Manager\n");
        printf("==================\n\n");

        printf("Updating repository...\n\n");

        system("mkdir -p ~/.cache/tx-pkg");

        system(
            "curl -L -o ~/.cache/tx-pkg/Packages.json "
            "https://rick000000007.github.io/tx-packages/repo/stable/Packages.json"
        );

        printf("\nRepository downloaded successfully.\n");
        printf("Cache location:\n");
        printf("~/.cache/tx-pkg/Packages.json\n");

        return 0;
    }

    if (strcmp(argv[1], "doctor") == 0) {
        return pkg_doctor();
    }

    if (strcmp(argv[1], "search") == 0) {
        return pkg_search(argc, argv);
    }

    if (strcmp(argv[1], "info") == 0) {
        return pkg_info(argc, argv);
    }

    if (strcmp(argv[1], "install") == 0) {
        return pkg_install(argc, argv);
    }

    if (strcmp(argv[1], "list") == 0) {
        return pkg_list();
    }

    if (strcmp(argv[1], "remove") == 0) {
        return pkg_remove(argc, argv);
    }

    if (strcmp(argv[1], "upgrade") == 0) {
        return pkg_upgrade();
    }
    if (strcmp(argv[1], "stats") == 0) {
        return pkg_stats();
    }
    if (strcmp(argv[1], "txrepo") == 0) {

    const char *path = "repo/stable";

    if (argc >= 3)
        path = argv[2];

    return txrepo_generate(path);
    }
    if (strcmp(argv[1], "txbuild") == 0) {

    const char *path = "package-template";

    if (argc >= 3)
        path = argv[2];

    return txbuild_package(path);
    }
    printf("Unknown command: %s\n", argv[1]);
    printf("Run 'pkg' to see available commands.\n");

    return 1;
}
