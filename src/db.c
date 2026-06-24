#include <stdio.h>
#include <stdlib.h>

#include "db.h"

int db_init(void)
{
    system("mkdir -p ~/.tx/var/lib/txpkg");

    system(
        "if [ ! -f ~/.tx/var/lib/txpkg/installed.json ]; then "
        "printf '{\n  \"packages\": []\n}\n' > "
        "~/.tx/var/lib/txpkg/installed.json; "
        "fi"
    );

    return 0;
}

int db_add_package(const char *name)
{
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "printf '{\n"
        "  \"packages\": [\n"
        "    {\"name\":\"%s\"}\n"
        "  ]\n"
        "}\n' > ~/.tx/var/lib/txpkg/installed.json",
        name
    );

    system(command);

    return 0;
}
int db_remove_package(const char *name)
{
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "printf '{\n"
        "  \"packages\": []\n"
        "}\n' > ~/.tx/var/lib/txpkg/installed.json"
    );

    system(command);

    return 0;
}
int db_is_installed(const char *name)
{
    char command[512];

    snprintf(
        command,
        sizeof(command),
        "grep -q '\"name\":\"%s\"' ~/.tx/var/lib/txpkg/installed.json",
        name
    );

    if (system(command) == 0)
        return 1;

    return 0;
}

