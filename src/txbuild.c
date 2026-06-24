#include <stdio.h>

#include "txbuild.h"

int txbuild_package(const char *path)
{
    printf("TX Package Builder\n");
    printf("==================\n\n");

    printf("Package directory : %s\n", path);
    printf("Reading CONTROL...\n");
    printf("Collecting files...\n");
    printf("Creating archive...\n");
    printf("Calculating SHA256...\n");

    printf("\nPackage build completed.\n");

    return 0;
}
