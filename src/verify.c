#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "verify.h"

int verify_package(
    const char *package_file,
    const char *expected_sha256)
{
    char command[1024];
    char actual[65];
    FILE *fp;

    if (!package_file || !expected_sha256)
        return 0;

    if (strlen(expected_sha256) == 0) {
        printf("Repository does not provide SHA256.\n");
        printf("Skipping verification.\n");
        return 1;
    }

    snprintf(
        command,
        sizeof(command),
        "sha256sum \"%s\"",
        package_file
    );

    fp = popen(command, "r");

    if (!fp) {
        printf("Unable to calculate SHA256.\n");
        return 0;
    }

    if (fscanf(fp, "%64s", actual) != 1) {
        pclose(fp);
        return 0;
    }

    pclose(fp);

    if (strcmp(actual, expected_sha256) != 0) {

        printf("\nSHA256 verification FAILED!\n");
        printf("Expected: %s\n", expected_sha256);
        printf("Actual:   %s\n", actual);

        return 0;
    }

    printf("SHA256 verification PASSED.\n");

    return 1;
}
