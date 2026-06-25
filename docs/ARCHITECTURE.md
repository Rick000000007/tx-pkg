# TX Package Manager Architecture Guide

## Overview

TX Package Manager v1.0 is a production-grade package management system
designed for the TX Terminal ecosystem. It implements a modular architecture
with clean separation of concerns, comprehensive error handling, and robust
data integrity.

## Design Principles

1. **Single Responsibility** - Each module has one clear purpose
2. **Fail-Safe** - Every operation is atomic with rollback capability
3. **Self-Healing** - Automatic recovery from crashes and interruptions
4. **Secure** - Path traversal protection, integrity verification, input validation
5. **Compatible** - Generic Linux + Android support with configurable paths

## Module Architecture

```
                    +------------------+
                    |     CLI Layer    |
                    |   (main.c)       |
                    +--------+---------+
                             |
                    +--------v---------+
                    | Command Dispatch |
                    | (src/commands/)  |
                    +--------+---------+
                             |
        +--------------------+--------------------+
        |                    |                    |
+-------v------+  +---------v---------+  +-------v-------+
|  Repository  |  |   Package / Meta  |  |   Database    |
|   Manager    |  |     Parser        |  |    Layer      |
| (repository) |  |  (package.c)      |  | (database.c)  |
+-------+------+  +---------+---------+  +-------+-------+
        |                    |                    |
        |          +---------v---------+         |
        |          |   Version Engine  |         |
        |          |   (version.c)     |         |
        |          +---------+---------+         |
        |                    |                    |
        +--------------------+--------------------+
                             |
                    +--------v---------+
                    |  Dependency      |
                    |  Solver          |
                    |  (solver.c)      |
                    +--------+---------+
                             |
                    +--------v---------+
                    |  Transaction     |
                    |  Manager         |
                    |  (transaction.c) |
                    +--------+---------+
                             |
        +--------------------+--------------------+
        |                    |                    |
+-------v------+  +---------v---------+  +-------v-------+
|   Package    |  |   Verification    |  |    Rollback   |
|  Extractor   |  |     Engine        |  |    Engine     |
|  (extract.c) |  |   (verify.c)      |  | (rollback.c)  |
+-------+------+  +---------+---------+  +-------+-------+
        |                    |                    |
        +--------------------+--------------------+
                             |
        +--------------------+--------------------+
        |                    |                    |
+-------v------+  +---------v---------+  +-------v-------+
|    Cache     |  |    Lock Manager   |  |   Recovery    |
|   Manager    |  |    (lock.c)       |  |    Engine     |
|  (cache.c)   |  +-------------------+  | (recovery.c)  |
+--------------+                         +---------------+
```

## Module Descriptions

### 1. Configuration Layer (config.h)

Central configuration defining:
- Version information
- Installation prefixes (configurable at compile time)
- Runtime paths (cache, database, lock files)
- Package format constants
- Repository format constants
- Operational limits and security settings

**Key Feature:** Android/Generic Linux dual support through compile-time
prefix selection. Build with `-DTX_PREFIX=/your/path` for custom installations.

### 2. Common Layer (common.h)

Foundation types and enumerations:
- Status codes (`tx_status_t`) - comprehensive error classification
- Package status states (`tx_pkg_status_t`) - installation lifecycle tracking
- Dependency relations (`tx_relation_t`) - version comparison operators
- Dependency kinds (`tx_dep_kind_t`) - all dependency types
- Transaction operation types (`tx_op_type_t`)
- Transaction states (`tx_state_t`) - operation progress tracking
- Utility macros for safe memory management and error propagation

### 3. Error Handling (error.c/h)

Structured error reporting system:
- Thread-local error state storage
- Error classification with status codes
- Contextual diagnostics (what, where, why, how to resolve)
- Automatic resolution suggestions per error type
- Silent failure prevention - every error must be explained

### 4. Version Engine (version.c/h)

Debian-style version comparison implementing:
- Parsing: `[epoch:]upstream[-release]` format
- Comparison: epoch first, then upstream (with tilde sorting), then release
- Satisfaction checking: `>=`, `<=`, `=`, `>>`, `<<`, `!=`
- Downgrade detection
- Normalization

### 5. Package Data Model (package.c/h)

Complete package representation:
- **Manifest** (`tx_manifest_t`) - full package metadata
- **Dependency** (`tx_dependency_t`) - linked list of relations
- **File Entry** (`tx_file_entry_t`) - tracked installed files
- **Installed Package** (`tx_installed_pkg_t`) - database record
- **CONTROL file parsing** - Debian-style key-value format
- **manifest.json parsing** - JSON metadata format

### 6. Dependency Solver (solver.c/h)

SAT-inspired dependency resolution:
- Forward dependency resolution (Depends, Pre-Depends)
- Reverse dependency tracking
- Virtual package resolution through Provides
- Conflict detection
- Cycle detection
- Topological ordering for correct install sequence
- Multiple operation support: install, remove, upgrade, autoremove

### 7. Database Layer (database.c/h)

SQLite-based persistent storage:
- **packages table** - installed package records
- **files table** - file ownership with SHA-256 checksums
- **reverse_deps table** - reverse dependency mapping
- **transactions table** - operation journal for recovery
- Schema versioning for future migrations
- WAL mode for better concurrency
- Full ACID transactions

### 8. Repository Manager (repository.c/h)

Multi-repository support:
- Repository CRUD operations
- Channel management (stable, testing, nightly, community, local)
- Priority-based package selection
- Enable/disable repositories
- Metadata download with SHA-256SUMS verification
- Incremental update support architecture

### 9. Cache Manager (cache.c/h)

Download cache management:
- Package file caching
- SHA-256 verification of cached files
- Automatic expiration (configurable age)
- Size limiting with LRU-style eviction
- Cache statistics

### 10. Transaction Engine (transaction.c/h)

Atomic package operations:
- Transaction planning from solver solutions
- Step-by-step execution with progress callbacks
- Journaling for crash recovery
- Automatic rollback on failure
- State machine for operation lifecycle

### 11. Package Extractor (extract.c/h)

Safe package extraction:
- tar.xz archive handling
- Path traversal attack prevention
- Symlink validation (no escapes from prefix)
- Script execution with prefix environment
- Atomic extract-to-temp-then-move

### 12. Verification Engine (verify.c/h)

Comprehensive integrity checking:
- SHA-256 file verification
- Archive validity checking
- Manifest completeness validation
- Installed file integrity verification
- Database integrity checking
- System-wide verification

### 13. Rollback Engine (rollback.c/h)

Complete state restoration:
- Package file backup before modification
- Database state backup
- File-level and package-level restore
- Backup cleanup after successful operations

### 14. Recovery Engine (recovery.c/h)

Crash recovery:
- Journal file scanning for incomplete transactions
- Half-installed package detection and repair
- Stale lock detection and removal
- Temporary file cleanup
- Automatic and manual recovery modes

### 15. Lock Manager (lock.c/h)

Concurrent operation prevention:
- PID-based file locking
- Non-blocking try-lock
- Timeout with configurable retry interval
- Stale lock detection (dead process identification)
- Safe lock breaking

### 16. Self Update (selfupdate.c/h)

Atomic self-updating:
- Version checking against repository
- Binary download with integrity verification
- Atomic swap via rename operations
- Rollback to previous version
- Channel-aware updates

## Data Flow

### Package Installation

```
User -> CLI -> cmd_install -> Lock Acquire -> DB Open
  -> Solver Context Setup -> tx_solve_install
    -> Dependency Resolution -> Solution Plan
      -> User Confirmation -> Transaction Execute
        -> Download (with cache check)
        -> Verify SHA-256
        -> Extract to temp
        -> Check paths (traversal protection)
        -> Run preinst
        -> Install files
        -> Run postinst
        -> Register in DB
        -> Journal write (each step)
      -> Transaction Commit
  -> Lock Release
```

### Package Removal

```
User -> CLI -> cmd_remove -> Lock Acquire -> DB Open
  -> Check installed -> Check reverse deps
    -> Check essential flag
      -> Get file list -> Remove files
      -> Unregister from DB
  -> Lock Release
```

### Crash Recovery

```
Startup -> tx_recovery_check
  -> Scan journal directory
  -> Check for stale locks
  -> Query DB for broken packages
  -> If needed: tx_recovery_run
    -> Fix stale locks
    -> Process journals (rollback incomplete)
    -> Fix broken packages
    -> Clean temp files
```

## Security Model

### Threats Mitigated

1. **Path Traversal** - `..` detection, symlink validation, prefix enforcement
2. **Malformed Packages** - Archive validation, manifest verification
3. **Concurrent Modification** - File locking with PID tracking
4. **Interrupted Operations** - Journal-based recovery
5. **Cache Poisoning** - SHA-256 verification of all downloads

### Trust Model

- Repository metadata is trusted once downloaded
- Package integrity verified via SHA-256 before extraction
- Installation scripts run with user privileges
- Future: Repository signature verification (architecture ready)

## Performance Considerations

- **SQLite WAL mode** - Better concurrent read performance
- **Indexed queries** - Fast package and file lookups
- **Cached metadata** - Repository data parsed once, reused
- **Lazy loading** - Components initialized on demand
- **Minimal system calls** - Batch operations where possible

## Future Extensibility

The architecture supports:
- Repository signature verification (`TX_ENABLE_SIGNATURES` flag)
- Delta updates (`TX_ENABLE_DELTA_UPDATES` flag)
- Additional dependency types
- Plugin architecture for custom commands
- Network transport abstraction (for mirrors)
