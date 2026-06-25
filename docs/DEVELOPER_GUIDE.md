# TX Package Manager Developer Guide

## Getting Started

### Prerequisites

- C11-compatible compiler (clang 10+ or gcc 8+)
- SQLite3 development library (`libsqlite3-dev`)
- Make
- curl, tar, sha256sum (runtime dependencies)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Rick000000007/tx-pkg.git
cd tx-pkg

# Standard build
make

# Debug build with assertions and debug output
make debug

# Run tests
make test

# Install
sudo make install
```

### Development Build

```bash
# Build with debug symbols and no optimization
make debug

# Check for issues
make check

# Format code
make format
```

## Project Structure

```
tx-pkg/
|-- src/
|   |-- main.c              # Entry point, command dispatch
|   |-- config.h            # Compile-time configuration
|   |-- common.h            # Shared types and constants
|   |-- error.c/h           # Error handling system
|   |-- version.c/h         # Version comparison engine
|   |-- package.c/h         # Package data structures and parsing
|   |-- solver.c/h          # Dependency solver
|   |-- database.c/h        # SQLite database layer
|   |-- repository.c/h      # Repository management
|   |-- cache.c/h           # Download cache
|   |-- transaction.c/h     # Atomic transaction engine
|   |-- extract.c/h         # Package extraction
|   |-- verify.c/h          # Verification engine
|   |-- rollback.c/h        # Rollback engine
|   |-- recovery.c/h        # Crash recovery
|   |-- lock.c/h            # File locking
|   |-- selfupdate.c/h      # Self-update mechanism
|   |-- commands/           # CLI command implementations
|   |   |-- cmd_*.c/h       # Individual commands
|   |-- vendor/             # Embedded dependencies
|   |   |-- cjson.c/h       # Minimal JSON parser
|-- tests/                  # Test suite
|-- docs/                   # Documentation
|-- package-template/       # Example package template
|-- repo/                   # Example repository
|-- Makefile
|-- README.md
|-- LICENSE
```

## Coding Standards

### Naming Conventions

- **Files:** `module.c` / `module.h` - lowercase with underscore
- **Functions:** `tx_module_function()` - `tx_` prefix, lowercase with underscore
- **Types:** `tx_type_t` - `tx_` prefix, lowercase, `_t` suffix
- **Constants:** `TX_CONSTANT_NAME` - uppercase with underscore
- **Macros:** `TX_MACRO_NAME` - uppercase with underscore
- **Globals:** `tx_global_name` - `tx_` prefix, lowercase with underscore

### Error Handling

Always use the error system for failures:

```c
tx_status_t some_function(...) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        TX_ERROR_SET(TX_ERROR_IO, "Cannot open file",
                     path, strerror(errno),
                     "Check permissions and try again.");
        return TX_ERROR_IO;
    }
    // ... use fp ...
    fclose(fp);
    return TX_OK;
}
```

For simple errors:

```c
TX_ERROR_SET_SIMPLE(TX_ERROR_NOT_FOUND, "Package not found");
```

### Memory Management

- Use `calloc()` for structures (zero-initialization)
- Free in reverse order of allocation
- Set pointers to NULL after free
- Check allocation results

```c
tx_manifest_t *m = calloc(1, sizeof(tx_manifest_t));
if (!m) return TX_ERROR_GENERAL;

// ... use m ...

tx_manifest_free(m);
free(m);
```

### Database Operations

Always wrap modifications in transactions:

```c
tx_db_begin(db);
// ... multiple operations ...
tx_db_commit(db);

// On error:
tx_db_rollback(db);
```

## Adding a New Command

1. Create `src/commands/cmd_mycmd.h` and `src/commands/cmd_mycmd.c`
2. Include and call the command in `src/main.c`
3. Add the source file to `Makefile` `CMD_SRCS`

Example command implementation:

```c
// cmd_mycmd.h
#ifndef TX_CMD_MYCMD_H
#define TX_CMD_MYCMD_H
#include "../common.h"
int cmd_mycmd(int argc, char *argv[]);
#endif

// cmd_mycmd.c
#include "cmd_mycmd.h"
#include "../config.h"
#include "../database.h"
#include <stdio.h>

int cmd_mycmd(int argc, char *argv[]) {
    printf("TX Package Manager v%s\n", TX_PKG_VERSION);
    // ... implementation ...
    return 0;
}
```

## Testing

### Unit Testing

Tests should be added to the `tests/` directory:

```c
// tests/test_version.c
#include "../src/version.h"
#include <assert.h>
#include <stdio.h>

int main() {
    tx_version_t a, b;
    
    tx_version_parse(&a, "1.0.0");
    tx_version_parse(&b, "1.0.1");
    
    assert(tx_version_compare(&a, &b) < 0);
    
    printf("PASS: version comparison\n");
    return 0;
}
```

### Integration Testing

Integration tests verify end-to-end workflows:

```bash
#!/bin/bash
# tests/integration/test_install.sh
set -e

pkg update
pkg install test-pkg
pkg list | grep test-pkg
pkg remove test-pkg
```

## Debugging

Enable debug logging:

```bash
# Build with debug
make debug

# Run with debug output
TX_DEBUG=1 ./pkg install some-package
```

Use `TX_DEBUG_LOG()` macro in code:

```c
TX_DEBUG_LOG("Entering function with param=%s\n", param);
```

## Common Patterns

### Opening the Database

```c
char db_path[TX_MAX_PATH];
snprintf(db_path, sizeof(db_path), "%s", TX_DB_FILE);

tx_db_t db;
if (tx_db_open(&db, db_path) != TX_OK) {
    fprintf(stderr, "Cannot open database\n");
    return 1;
}
// ... use db ...
tx_db_close(&db);
```

### Loading Repositories

```c
tx_repo_list_t list;
tx_repo_list_init(&list);

// Access repositories...

// Save changes
tx_repo_list_save(&list, TX_REPO_CONFIG);
tx_repo_list_free(&list);
```

### Solving Dependencies

```c
tx_solver_context_t ctx = {
    .available = entries,
    .available_count = count,
    .installed = installed,
    .installed_count = installed_count,
    .db_handle = &db,
};

tx_solver_solution_t sol;
tx_status_t s = tx_solve_install(&ctx, "package-name", &sol);
if (s == TX_OK && sol.is_valid) {
    tx_solution_print(&sol);
}
tx_solution_free(&sol);
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Follow coding standards
4. Add tests for new functionality
5. Update documentation
6. Submit a pull request

## Resources

- [Architecture Guide](ARCHITECTURE.md)
- [.txpkg Specification](TXPKG_SPEC.md)
- [Repository Specification](REPOSITORY_SPEC.md)
