#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "verify.h"

int verify_package(const char *package_file,
                   const char *expected_sha256)
{
    char actual[129];
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "sha256sum %s | cut -d' ' -f1",
        package_file
    );

    FILE *fp = popen(command, "r");
    if (!fp) {
        printf("ERROR: Unable to calculate SHA256.\n");
        return 0;
    }

    if (!fgets(actual, sizeof(actual), fp)) {
        pclose(fp);
        return 0;
    }

    pclose(fp);

    actual[strcspn(actual, "\n")] = 0;

    if (strcmp(actual, expected_sha256) == 0)
        return 1;

    printf("\nSHA256 verification failed.\n");
    printf("Expected: %s\n", expected_sha256);
    printf("Actual:   %s\n", actual);

    return 0;
}
