# TX Package Specification v2.0

## Overview

A `.txpkg` file is a tar.xz archive containing package metadata, installation
scripts, and payload files. This specification defines the format for TX
Package Manager v1.0.

## File Format

```
package-name-version-release.architecture.txpkg

Example: nano-1.0.0-1.aarch64.txpkg
```

## Archive Structure

```
package.txpkg (tar.xz)
|-- CONTROL
|-- manifest.json
|-- checksums
|-- preinst
|-- postinst
|-- prerm
|-- postrm
`-- files/
    |-- usr/
    |   |-- bin/
    |   |   `-- program
    |   |-- lib/
    |   |   `-- library.so
    |   `-- share/
    |       `-- doc/
    |           `-- package/
    |               `-- README
    `-- etc/
        `-- package.conf
```

## Files

### CONTROL

Debian-style control file with key-value pairs:

```
Package: nano
Version: 1.0.0
Release: 1
Architecture: aarch64
License: GPL-3.0
Description: Lightweight terminal editor
  This is a longer description that can span
  multiple lines with continuation.
Homepage: https://nano-editor.org
Maintainer: TX Project <team@txterminal.dev>
Category: editors
Section: editors
Essential: no
Depends: busybox, libc
Provides: editor
Conflicts: pico
Replaces:
Breaks:
Suggests: vim
Recommends:
Optional: spell
```

**Required fields:**
- `Package` - Package name (max 128 chars)
- `Version` - Upstream version string
- `Release` - Package release number
- `Architecture` - Target architecture

**Optional fields:**
- `License`, `Description`, `Homepage`, `Maintainer`
- `Category`, `Section`
- `Essential` - "yes" marks package as essential
- `Depends`, `Pre-Depends`, `Provides`, `Conflicts`
- `Replaces`, `Breaks`, `Suggests`, `Recommends`, `Optional`

### manifest.json

JSON metadata file with the same information as CONTROL:

```json
{
  "name": "nano",
  "version": "1.0.0",
  "release": 1,
  "architecture": "aarch64",
  "license": "GPL-3.0",
  "description": "Lightweight terminal editor",
  "homepage": "https://nano-editor.org",
  "maintainer": "TX Project",
  "category": "editors",
  "section": "editors",
  "essential": false,
  "depends": "busybox (>= 1.0)",
  "provides": "editor",
  "conflicts": "pico",
  "suggests": "vim",
  "sha256": "abc123...",
  "size": 102400,
  "installed_size": 204800
}
```

### checksums

SHA-256 checksums of all payload files:

```
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  usr/bin/nano
a3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b856  usr/share/doc/nano/README
```

### Installation Scripts

All scripts are optional. When present, they must be executable.

**preinst** - Runs before package extraction
```bash
#!/bin/sh
# Environment variables:
#   PREFIX - Installation prefix
#   PKG_NAME, PKG_VERSION - Package info
echo "Preparing to install $PKG_NAME..."
```

**postinst** - Runs after file installation
```bash
#!/bin/sh
# Update icon cache, register services, etc.
echo "$PKG_NAME installed successfully."
```

**prerm** - Runs before package removal
```bash
#!/bin/sh
# Stop services, unregister, etc.
```

**postrm** - Runs after package removal
```bash
#!/bin/sh
# Clean up remaining files
```

### files/

The `files/` directory contains the actual payload. Files are extracted
relative to the installation prefix (`/usr/local` or Termux prefix).

**Path rules:**
- All paths must be relative (no leading `/`)
- No `..` components (enforced by package manager)
- Symlinks must target paths within the prefix
- Regular files, directories, and symlinks supported

## Version Format

```
[epoch:]upstream-version[-release]
```

Examples:
- `1.2.3` - Simple version
- `1:1.2.3-1` - With epoch and release
- `1.2.3~rc1-2` - Pre-release version

Comparison rules (Debian-style):
1. Epoch compared numerically (higher wins)
2. Upstream compared with tilde-sorting (`~` sorts before everything)
3. Release compared numerically

## Dependency Format

Dependencies in CONTROL use comma-separated declarations:

```
Depends: pkg1, pkg2 (>= 1.0), pkg3 | pkg4
```

Version relations:
- `(>= 1.0)` - Greater than or equal
- `(<= 1.0)` - Less than or equal
- `(= 1.0)` - Equal
- `(>> 1.0)` - Strictly greater
- `(<< 1.0)` - Strictly less
- `(!= 1.0)` - Not equal

Pipe (`|`) indicates alternatives (first available is chosen).

## Virtual Packages

A package can provide virtual names:

```
Package: busybox
Provides: editor, shell, awk
```

Other packages can depend on the virtual name:
```
Package: some-script
Depends: editor
```

The package manager will resolve `editor` to any package that provides it.

## Security Requirements

1. All paths in `files/` are validated before extraction
2. Symlinks are checked to not escape the installation prefix
3. Scripts run with user's privileges (not elevated)
4. SHA-256 checksums verified before installation
5. No setuid/setgid bits preserved during extraction

## Creating Packages

### Manual Method

```bash
mkdir -p mypackage/files/usr/bin
mkdir -p mypackage/files/usr/share/doc/mypackage
cp myprogram mypackage/files/usr/bin/
cp README mypackage/files/usr/share/doc/mypackage/

cat > mypackage/CONTROL << 'EOF'
Package: mypackage
Version: 1.0.0
Release: 1
Architecture: aarch64
Description: My package
EOF

cat > mypackage/manifest.json << 'EOF'
{"name":"mypackage","version":"1.0.0","release":1,"architecture":"aarch64","description":"My package"}
EOF

cd mypackage && find files -type f -exec sha256sum {} \; > checksums
cd ..

tar -cJf mypackage-1.0.0-1.aarch64.txpkg -C mypackage .
```

### Using txbuild

The `pkg txbuild` command automates package creation:

```bash
pkg txbuild ./mypackage-source/
```

## Compatibility

- Format version: 2
- Supported archive format: tar.xz
- Supported architecture: aarch64 (primary), x86_64
- Minimum tx-pkg version: 1.0.0
