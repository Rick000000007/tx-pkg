#include <stdio.h>
#include <string.h>

int find_package_filename(
    const char *package,
    char *filename,
    size_t size)
{
    FILE *fp;
    char line[512];
    int found = 0;

    fp = fopen(
        "/data/data/com.termux/files/home/.cache/tx-pkg/Packages.json",
        "r"
    );

    if (fp == NULL)
        return 0;

    while (fgets(line, sizeof(line), fp)) {

        if (strstr(line, "\"name\"") &&
            strstr(line, package)) {

            found = 1;
            continue;
        }

        if (found &&
            strstr(line, "\"filename\"")) {

            char *start = strchr(line, ':');

            if (start == NULL)
                break;

            start++;

            while (*start == ' ' || *start == '"')
                start++;

            char *end = strchr(start, '"');

            if (end == NULL)
                break;

            *end = '\0';

            strncpy(filename, start, size - 1);
            filename[size - 1] = '\0';

            fclose(fp);
            return 1;
        }
    }

    fclose(fp);

    return 0;
}
int find_package_sha256(const char *package,
                        char *sha256,
                        int size)
{
    char command[512];
    char line[256];

    snprintf(
        command,
        sizeof(command),
        "grep -A 5 '\"name\": \"%s\"' ~/.cache/tx-pkg/Packages.json | grep '\"sha256\"'",
        package
    );

    FILE *fp = popen(command, "r");
    if (!fp)
        return 0;

    if (!fgets(line, sizeof(line), fp)) {
        pclose(fp);
        return 0;
    }

    pclose(fp);

    char *start = strchr(line, ':');
    if (!start)
        return 0;

    start++;

    while (*start == ' ' || *start == '"' )
        start++;

    char *end = strchr(start, '"');
    if (!end)
        return 0;

    *end = '\0';

    strncpy(sha256, start, size - 1);
    sha256[size - 1] = '\0';

    return 1;
}
