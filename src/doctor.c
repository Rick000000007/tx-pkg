#include <stdio.h>
#include <unistd.h>

#include "doctor.h"

int pkg_doctor(void)
{
    printf("TX Package Manager Doctor\n");
    printf("=========================\n\n");

    if (access("/data/data/com.termux/files/home/.tx/var/lib/txpkg/installed.json", F_OK) == 0)
        printf("[OK] installed.json\n");
    else
        printf("[FAIL] installed.json\n");

    if (access("/data/data/com.termux/files/home/.cache/tx-pkg", F_OK) == 0)
        printf("[OK] package cache\n");
    else
        printf("[FAIL] package cache\n");

    if (access("/data/data/com.termux/files/home/.cache/tx-pkg/packages", F_OK) == 0)
        printf("[OK] download directory\n");
    else
        printf("[FAIL] download directory\n");

    return 0;
}
