#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "install.h"
#include "db.h"
#include "verify.h"

int pkg_install(int argc, char *argv[])
{
    printf("TX Package Manager\n");
    printf("==================\n\n");

    if (argc < 3) {
        printf("Usage: pkg install <package>\n");
        return 1;
    }

    /* Prevent duplicate installation */
    if (db_is_installed(argv[2])) {
        printf("Package '%s' is already installed.\n", argv[2]);
        printf("Nothing to do.\n");
        return 0;
    }

    printf("Installing: %s\n\n", argv[2]);

    char filename[256];
    char expected_sha256[128];
    char package_path[512];
    char command[1024];

    /* Find package filename */
    if (!find_package_filename(
            argv[2],
            filename,
            sizeof(filename))) {

        printf("Package not found: %s\n", argv[2]);
        return 1;
    }

    /* Read SHA256 if available */
    expected_sha256[0] = '\0';

    find_package_sha256(
        argv[2],
        expected_sha256,
        sizeof(expected_sha256)
    );

    printf("Package file: %s\n\n", filename);

    /* Create download directory */
    system("mkdir -p ~/.cache/tx-pkg/packages");

    /* Download package */
    snprintf(
        command,
        sizeof(command),
        "curl -L -o ~/.cache/tx-pkg/packages/%s.txpkg "
        "https://rick000000007.github.io/tx-packages/repo/stable/%s",
        argv[2],
        filename
    );

    system(command);

    /* Build absolute package path */
    snprintf(
        package_path,
        sizeof(package_path),
        "%s/.cache/tx-pkg/packages/%s.txpkg",
        getenv("HOME"),
        argv[2]
    );

    printf("\nVerifying package...\n");

    if (strlen(expected_sha256) == 0) {

        printf("Repository does not provide SHA256.\n");
        printf("Skipping verification.\n");

    } else {

        if (!verify_package(
                package_path,
                expected_sha256)) {

            printf("\nInstallation aborted.\n");
            return 1;
        }
    }

    /* Extract package */
    system("mkdir -p ~/.cache/tx-pkg/extract");
    system("rm -rf ~/.cache/tx-pkg/extract/*");

    snprintf(
        command,
        sizeof(command),
        "tar -xf ~/.cache/tx-pkg/packages/%s.txpkg "
        "-C ~/.cache/tx-pkg/extract",
        argv[2]
    );

    system(command);

    printf("\nPackage extracted successfully.\n");

    /* Execute install script */
    system("chmod +x ~/.cache/tx-pkg/extract/install.sh");
    system("cd ~/.cache/tx-pkg/extract && ./install.sh");

    printf("\nInstall script executed successfully.\n");
    printf("\nPackage installed successfully.\n");

    /* Register package */
    db_add_package(argv[2]);

    printf("\nPackage registered successfully.\n");

    printf("\nDownload completed.\n");
    printf("Saved to:\n");
    printf("~/.cache/tx-pkg/packages/%s.txpkg\n", argv[2]);

    return 0;
}

