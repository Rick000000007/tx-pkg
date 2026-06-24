#include <stdio.h>
#include <stdlib.h>

#include "list.h"

int pkg_list(void)
{
    printf("TX Package Manager\n");
    printf("==================\n\n");

    printf("Installed packages\n\n");

    system("grep '\"name\"' ~/.tx/var/lib/txpkg/installed.json | sed 's/.*\"name\":\"//;s/\".*//'");

    return 0;
}
