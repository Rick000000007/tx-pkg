# TX Package Manager Migration Guide

## Overview

This guide covers migrating from previous versions of tx-pkg to v1.0.

## Migrating from Pre-1.0 Versions

### Database Migration

The most significant change is the switch from JSON flat files to SQLite.

**Automatic Migration:**
```bash
# On first run, tx-pkg v1.0 will:
# 1. Detect old ~/.tx/var/lib/txpkg/installed.json
# 2. Create new SQLite database
# 3. Import existing package records
# 4. Preserve old file as backup

# Simply run any command
pkg list
```

**Manual Migration:**
```bash
# Backup old database
cp ~/.tx/var/lib/txpkg/installed.json \
   ~/.tx/var/lib/txpkg/installed.json.bak

# Initialize new database (auto-imports old data)
pkg doctor --fix

# Verify migration
pkg list
pkg stats
```

### Path Changes

| Old Path (pre-1.0) | New Path (v1.0) |
|---------------------|-------------------|
| `~/.tx/var/lib/txpkg/installed.json` | `~/.tx/var/lib/txpkg/txpkg.db` |
| `~/.cache/tx-pkg/Packages.json` | `~/.tx/.cache/tx-pkg/repos/*/Packages.json` |
| (none) | `~/.tx/var/run/txpkg.lock` |
| (none) | `~/.tx/var/backup/txpkg/` |
| (none) | `~/.tx/var/journal/txpkg/` |

### Command Changes

| Old Command | New Command | Notes |
|-------------|-------------|-------|
| `pkg txrepo [path]` | `pkg repo generate [path]` | Expanded repo management |
| `pkg txbuild [path]` | `pkg build [path]` | New build system |
| (none) | `pkg autoremove` | New command |
| (none) | `pkg verify [pkg]` | New command |
| (none) | `pkg reinstall pkg` | New command |
| (none) | `pkg repo ...` | New subcommand group |
| (none) | `pkg channel` | New command |
| (none) | `pkg self-update` | New command |

### Configuration Changes

**Old:** No formal configuration
**New:** `~/.tx/etc/txpkg/repositories.conf`

```bash
# Old: Hardcoded single repository
# New: Configurable multiple repositories

# View current config
cat ~/.tx/etc/txpkg/repositories.conf

# Default is auto-generated with tx-main repository
```

### Package Format Changes

`.txpkg` format v2 is backward-compatible with v1. Key additions:

| Feature | v1 | v2 (v1.0) |
|---------|-----|-----------|
| CONTROL file | Optional | Recommended |
| manifest.json | Required | Recommended |
| checksums | Not supported | Supported |
| Pre/post scripts | Limited | Full support |
| Dependency types | Basic | Complete |
| Virtual packages | Not supported | Supported |

**Existing v1 packages continue to work without modification.**

### Repository Format Changes

Repository format v2 additions:

| Feature | v1 | v2 (v1.0) |
|---------|-----|-----------|
| index.json | Not present | Required |
| Per-package .json | Not present | Optional |
| SHA256SUMS | Basic | Standard format |
| Channel support | Basic | Full |

**Existing v1 repositories are automatically compatible.**

## Breaking Changes

### API Changes (for developers)

The C API has been completely redesigned:

```c
// Old API (pre-1.0)
typedef struct {
    char name[64];
    char version[32];
    int release;
} Package;

// New API (v1.0)
typedef struct tx_manifest {
    char name[TX_MAX_PACKAGE_NAME];
    tx_version_t version;
    // ... expanded fields
} tx_manifest_t;
```

All modules now use the `tx_` prefix consistently.

### Database Schema

Old flat JSON is replaced with SQLite schema. If you had tools that read
the old `installed.json` directly, they will need updating.

## Rollback

If migration fails:

```bash
# Restore old database
mv ~/.tx/var/lib/txpkg/installed.json.bak \
   ~/.tx/var/lib/txpkg/installed.json

# Remove new database
rm -f ~/.tx/var/lib/txpkg/txpkg.db

# Remove lock file
rm -f ~/.tx/var/run/txpkg.lock
```

## Verification

After migration, verify everything works:

```bash
# Check database
pkg doctor

# List packages
pkg list

# Verify a package
pkg verify <package-name>

# Check for updates
pkg update
pkg upgrade --dry-run
```

## Getting Help

If you encounter issues during migration:

1. Run `pkg doctor --fix` to diagnose and repair
2. Check the [Developer Guide](DEVELOPER_GUIDE.md)
3. Report issues at: https://github.com/Rick000000007/tx-pkg/issues
