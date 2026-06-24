#ifndef TX_PARSER_H
#define TX_PARSER_H

#include <stddef.h>

int find_package_filename(
    const char *package,
    char *filename,
    size_t size
);
int find_package_sha256(
    const char *package,
    char *sha256,
    int size
);
#endif
