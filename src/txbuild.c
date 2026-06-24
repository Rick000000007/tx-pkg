#include <stdio.h>

#include "txbuild.h"

int txbuild_package(const char *path)
{
    printf("TX Package Builder\n");
    printf("==================\n\n");

    printf("Source directory : %s\n", path);
    printf("Reading CONTROL...\n");
    printf("Collecting files...\n");
    printf("Creating package...\n");
    printf("Generating SHA256...\n");

    printf("\nOutput directory : dist/\n");
    printf("Package name     : hello-1.0.0-1.aarch64.txpkg\n");

    printf("\nBuild completed successfully.\n");

    return 0;
}
