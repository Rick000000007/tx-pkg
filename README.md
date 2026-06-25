# TX Package Manager v1.0

**tx-pkg** is the official package manager for TX Terminal. It provides a
complete, production-grade package management system supporting repository
management, dependency resolution, atomic transactions, rollback capability,
and comprehensive package verification.

## Features

- **Complete Repository Management** - Multiple repositories, channels,
  priorities, enable/disable support
- **Full Dependency Resolution** - Depends, Provides, Conflicts, Replaces,
  Breaks, Suggests, Recommends, Optional dependencies, Virtual packages
- **Atomic Transactions** - All-or-nothing operations with automatic rollback
- **Rollback Engine** - Complete state restoration on failure
- **SQLite Package Database** - Fast, reliable package and file tracking
- **SHA-256 Verification** - Package integrity and corruption detection
- **Path Traversal Protection** - Safe package extraction
- **File Locking** - Prevents concurrent operations
- **Crash Recovery** - Journal-based recovery from interrupted operations
- **Self Update** - Atomic self-updating with rollback support
- **Generic Linux + Android** - Configurable installation prefix

## Commands

```
pkg update              Update repository metadata
pkg search <query>      Search for packages
pkg info <package>      Show package information
pkg list                List installed packages
pkg install <pkg>       Install a package
pkg remove <pkg>        Remove a package
pkg reinstall <pkg>     Reinstall a package
pkg upgrade [pkg]       Upgrade packages
pkg autoremove          Remove orphaned packages
pkg clean [--all]       Clean package cache
pkg verify [pkg]        Verify package integrity
pkg doctor [--fix]      Diagnose and fix issues
pkg repo <cmd>          Manage repositories
pkg channel             Show channel information
pkg self-update         Update tx-pkg itself
```

## Quick Start

```bash
# Build
make

# Install
sudo make install

# Update package lists
pkg update

# Search for packages
pkg search editor

# Install a package
pkg install nano

# List installed packages
pkg list

# Check system health
pkg doctor
```

## Building

### Requirements

- C compiler (clang or gcc)
- SQLite3 development library
- Make
- curl (for downloads)
- tar (for package extraction)
- sha256sum (for verification)

### Build Options

```bash
# Standard build
make

# Debug build
make debug

# Static build
make static

# Install to custom prefix
make PREFIX=/custom/path install
```

### Platform-Specific Notes

**Android/Termux:**
```bash
# The default prefix is automatically set for Android
make
```

**Generic Linux:**
```bash
# Default prefix is /usr/local
make
# Or override:
make PREFIX=/usr
```

## Architecture

```
CLI (main.c)
    |
Command Dispatcher (src/commands/)
    |
Repository Manager (src/repository.c)
Metadata Parser    (src/package.c)
    |
Dependency Solver  (src/solver.c)
Version Manager    (src/version.c)
    |
Transaction Manager (src/transaction.c)
Package Extractor   (src/extract.c)
    |
Ownership Database  (src/database.c)
Cache Manager       (src/cache.c)
Verification Engine (src/verify.c)
Rollback Engine     (src/rollback.c)
Recovery Engine     (src/recovery.c)
Lock Manager        (src/lock.c)
```

## Documentation

See the `docs/` directory for complete documentation:

- [Architecture Guide](docs/ARCHITECTURE.md)
- [.txpkg Specification](docs/TXPKG_SPEC.md)
- [Repository Specification](docs/REPOSITORY_SPEC.md)
- [Developer Guide](docs/DEVELOPER_GUIDE.md)
- [Package Author Guide](docs/PACKAGE_AUTHOR_GUIDE.md)
- [Repository Maintainer Guide](docs/REPO_MAINTAINER_GUIDE.md)
- [Migration Guide](docs/MIGRATION_GUIDE.md)

## License

MIT License - see [LICENSE](LICENSE) file.

## Project

TX Terminal Project - https://txterminal.dev
