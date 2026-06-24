#ifndef TX_DB_H
#define TX_DB_H

int db_init(void);
int db_add_package(const char *name);
int db_remove_package(const char *name);
int db_is_installed(const char *name);

#endif
