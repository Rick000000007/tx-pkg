#ifndef TX_VERIFY_H
#define TX_VERIFY_H

int verify_package(const char *package_file,
                   const char *expected_sha256);

#endif
