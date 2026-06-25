# TX Repository Specification v2.0

## Overview

This document specifies the repository format for TX Package Manager v1.0.
A repository is an HTTP-accessible directory structure containing package
metadata and binary packages.

## Repository Structure

```
repo/
|-- <channel>/
|   |-- index.json          # Repository metadata
|   |-- Packages.json       # Package catalog
|   |-- SHA256SUMS          # Package checksums
|   `-- packages/
|       |-- pkg1-1.0.txpkg
|       |-- pkg1-1.0.json
|       |-- pkg1-1.0.txpkg.sha256
|       |-- pkg2-2.0.txpkg
|       `-- ...
`-- <another-channel>/
    `-- ...
```

## index.json

Repository metadata file:

```json
{
  "repository": "TX Official Repository",
  "format_version": 2,
  "metadata_version": 2,
  "architecture": "aarch64",
  "channel": "stable",
  "url": "https://rick000000007.github.io/tx-packages",
  "packages": 150,
  "generated": "2026-06-26T00:00:00Z",
  "valid_until": "2026-07-26T00:00:00Z"
}
```

Fields:
- `repository` - Human-readable repository name
- `format_version` - Repository format version (2)
- `metadata_version` - Package metadata version (2)
- `architecture` - Primary architecture
- `channel` - Channel name
- `url` - Repository base URL
- `packages` - Number of packages
- `generated` - ISO 8601 generation timestamp
- `valid_until` - Optional expiration timestamp

## Packages.json

Package catalog containing metadata for all available packages:

```json
{
  "packages": [
    {
      "name": "nano",
      "version": "1.0.0",
      "release": 1,
      "architecture": "aarch64",
      "category": "editors",
      "description": "Lightweight terminal editor",
      "filename": "nano-1.0.0-1.aarch64.txpkg",
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
      "size": 102400,
      "installed_size": 204800,
      "depends": "busybox",
      "provides": "editor",
      "conflicts": "pico"
    }
  ]
}
```

### Package Entry Fields

**Required:**
- `name` - Package name
- `version` - Version string
- `release` - Release number
- `architecture` - Target architecture
- `filename` - Package archive filename

**Optional:**
- `sha256` - Package SHA-256 checksum
- `size` - Download size in bytes
- `installed_size` - Installed size in bytes
- `category` - Package category
- `description` - Short description
- `depends` - Comma-separated dependencies
- `pre_depends` - Pre-dependencies
- `provides` - Virtual packages provided
- `conflicts` - Conflicting packages
- `replaces` - Replaced packages
- `breaks` - Broken by this package
- `suggests` - Suggested packages
- `recommends` - Recommended packages

## SHA256SUMS

Checksum file for all packages:

```
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  packages/nano-1.0.0-1.aarch64.txpkg
a3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b856  packages/busybox-1.37.0-1.aarch64.txpkg
```

Format: `<sha256>  <path>` (two spaces between hash and path)

## Per-Package Files

Each package may have additional files alongside the `.txpkg` archive:

### Package Metadata JSON

`packages/<name>-<version>-<release>.<arch>.json`:

```json
{
  "name": "nano",
  "version": "1.0.0",
  "release": 1,
  "architecture": "aarch64",
  "description": "Lightweight terminal editor",
  "depends": "busybox"
}
```

### Package Checksum

`packages/<name>-<version>-<release>.<arch>.txpkg.sha256`:

```
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
```

## Channels

Repositories support multiple channels:

| Channel    | Purpose                              |
|------------|--------------------------------------|
| stable     | Production-ready packages            |
| testing    | Pre-release quality packages         |
| nightly    | Latest development builds            |
| community  | Community-contributed packages       |
| local      | Local/custom packages                |

Channel URLs:
```
<repo-url>/repo/<channel>/Packages.json
```

Example:
```
https://example.com/tx-packages/repo/stable/Packages.json
https://example.com/tx-packages/repo/testing/Packages.json
```

## Repository Priorities

When multiple repositories provide the same package, the one with the
highest priority is chosen. Priority is a positive integer (default: 50,
main repository: 100).

## Repository Configuration

Configured in `~/.tx/etc/txpkg/repositories.conf`:

```json
{
  "repositories": [
    {
      "name": "tx-main",
      "url": "https://rick000000007.github.io/tx-packages",
      "channel": "stable",
      "priority": 100,
      "enabled": true
    },
    {
      "name": "tx-testing",
      "url": "https://rick000000007.github.io/tx-packages",
      "channel": "testing",
      "priority": 50,
      "enabled": false
    }
  ]
}
```

## Building a Repository

The `pkg txrepo` command generates repository metadata:

```bash
# Generate repo metadata from packages directory
pkg txrepo ./repo/stable/

# This creates:
#   - Packages.json (package catalog)
#   - SHA256SUMS (checksums)
#   - index.json (repo metadata)
```

## Signature Support (Future)

The repository format is designed for future GPG signature support:

```
repo/stable/
|-- Packages.json
|-- Packages.json.sig       # GPG signature
|-- SHA256SUMS
|-- SHA256SUMS.sig
|-- Release                  # Contains checksums of metadata
|-- Release.sig
`-- packages/
```

This will be activated with the `TX_ENABLE_SIGNATURES` compile-time flag.

## Migration from v1

Repositories using v1 format are automatically compatible. The package
manager reads both formats. To upgrade to v2:

1. Add `index.json` with format_version: 2
2. Add per-package .json files (optional)
3. Update Packages.json to include new fields

## Example Complete Repository

```
https://packages.example.com/tx/
|-- repo/
|   |-- stable/
|   |   |-- index.json
|   |   |-- Packages.json
|   |   |-- SHA256SUMS
|   |   `-- packages/
|   |       |-- busybox-1.37.0-1.aarch64.txpkg
|   |       |-- nano-1.0.0-1.aarch64.txpkg
|   |       `-- git-2.50.0-1.aarch64.txpkg
|   `-- testing/
|       |-- index.json
|       |-- Packages.json
|       |-- SHA256SUMS
|       `-- packages/
`-- install.sh
```
