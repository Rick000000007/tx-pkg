# TX Package Author Guide

## Introduction

This guide explains how to create packages for the TX Package ecosystem.
Packages are `.txpkg` files containing software, metadata, and installation
scripts.

## Prerequisites

- A working TX Package Manager installation
- Basic knowledge of shell scripting
- Target architecture access (for testing)

## Creating Your First Package

### Step 1: Prepare the Directory Structure

```bash
mkdir -p mypkg/files/usr/bin
mkdir -p mypkg/files/usr/share/doc/mypkg
```

### Step 2: Add Your Files

```bash
cp your-program mypkg/files/usr/bin/
cp README LICENSE mypkg/files/usr/share/doc/mypkg/
```

### Step 3: Create the CONTROL File

```bash
cat > mypkg/CONTROL << 'EOF'
Package: mypkg
Version: 1.0.0
Release: 1
Architecture: aarch64
License: MIT
Description: My first TX package
  A longer description can go here
  across multiple lines.
Homepage: https://example.com/mypkg
Maintainer: Your Name <you@example.com>
Category: utils
Depends: busybox
EOF
```

### Step 4: Create the Manifest

```bash
cat > mypkg/manifest.json << 'EOF'
{
  "name": "mypkg",
  "version": "1.0.0",
  "release": 1,
  "architecture": "aarch64",
  "license": "MIT",
  "description": "My first TX package",
  "homepage": "https://example.com/mypkg",
  "maintainer": "Your Name",
  "category": "utils",
  "depends": "busybox"
}
EOF
```

### Step 5: Create Checksums

```bash
cd mypkg && find files -type f -exec sha256sum {} \; > checksums && cd ..
```

### Step 6: Build the Package

```bash
# Method 1: Manual
tar -cJf mypkg-1.0.0-1.aarch64.txpkg -C mypkg .

# Method 2: Using txbuild
pkg txbuild ./mypkg
```

### Step 7: Test the Package

```bash
pkg install ./mypkg-1.0.0-1.aarch64.txkg
pkg list | grep mypkg
pkg remove mypkg
```

## CONTROL File Reference

### Required Fields

| Field | Description | Example |
|-------|-------------|---------|
| Package | Package name (lowercase, alphanumeric) | `mypkg` |
| Version | Upstream version | `1.0.0` |
| Release | Package revision | `1` |
| Architecture | Target CPU | `aarch64`, `x86_64` |

### Optional Fields

| Field | Description |
|-------|-------------|
| License | Software license (SPDX identifier) |
| Description | Short description (one line) |
| Homepage | Project URL |
| Maintainer | Name and email |
| Category | Functional category |
| Section | Repository section |
| Essential | `yes` for essential packages |

### Dependency Fields

| Field | Purpose |
|-------|---------|
| Depends | Required packages |
| Pre-Depends | Must be installed first |
| Provides | Virtual packages provided |
| Conflicts | Incompatible packages |
| Replaces | Replaced package names |
| Breaks | Packages broken by this |
| Suggests | Optional enhancements |
| Recommends | Strongly recommended |
| Optional | Nice-to-have features |

### Dependency Syntax

```
# Simple dependency
Depends: otherpkg

# With version requirement
Depends: otherpkg (>= 1.0)

# Multiple dependencies
Depends: pkg1, pkg2, pkg3

# Alternative (first available)
Depends: pkg1 | pkg2

# Complex
Depends: pkg1 (>= 1.0), pkg2 | pkg3, pkg4 (<< 2.0)
```

## Installation Scripts

Scripts are optional. When present, they must be executable.

### preinst

Runs before file extraction. Use for preparation:

```bash
#!/bin/sh
# Pre-installation script

# Check for conflicts
if [ -f "$PREFIX/bin/old-name" ]; then
    echo "Please remove old-name first"
    exit 1
fi

# Create required directories
mkdir -p "$PREFIX/var/lib/mypkg"
```

### postinst

Runs after files are installed. Use for configuration:

```bash
#!/bin/sh
# Post-installation script

# Generate default config if not exists
if [ ! -f "$PREFIX/etc/mypkg.conf" ]; then
    cat > "$PREFIX/etc/mypkg.conf" << 'EOF'
# Default configuration
debug=false
EOF
fi

# Update library cache if needed
if command -v ldconfig >/dev/null 2>&1; then
    ldconfig "$PREFIX/lib"
fi

echo "$PKG_NAME installed successfully"
```

### prerm

Runs before removal. Use for cleanup:

```bash
#!/bin/sh
# Pre-removal script

# Stop any running instances
if command -v pkill >/dev/null 2>&1; then
    pkill -f "$PKG_NAME" 2>/dev/null || true
fi
```

### postrm

Runs after removal. Use for final cleanup:

```bash
#!/bin/sh
# Post-removal script

# Remove empty directories
rmdir "$PREFIX/var/lib/mypkg" 2>/dev/null || true
```

## Best Practices

### File Placement

```
files/
|-- usr/
|   |-- bin/          # Executables
|   |-- lib/          # Libraries
|   |-- include/      # Header files
|   |-- share/
|   |   |-- doc/      # Documentation
|   |   |-- man/      # Man pages
|   |   `-- <pkg>/    # Package data
|   `-- etc/          # Configuration files
```

### Version Management

- Use semantic versioning (MAJOR.MINOR.PATCH)
- Increment Release when packaging changes
- Use epoch (`1:`) for major upstream version resets

### Dependencies

- Only list direct dependencies
- Don't over-specify version requirements
- Use virtual packages for common interfaces
- Test with minimal dependency set

### Security

- Never include setuid/setgid binaries
- Validate all inputs in scripts
- Use `$PREFIX` for all paths
- Don't assume specific system configuration

## Examples

### Simple Binary Package

```
mycalc/
|-- CONTROL
|-- manifest.json
|-- checksums
|-- postinst
|-- prerm
`-- files/
    `-- usr/
        `-- bin/
            `-- mycalc
```

### Library Package

```
mylib/
|-- CONTROL
|-- manifest.json
|-- checksums
|-- postinst
`-- files/
    `-- usr/
        |-- include/
        |   `-- mylib.h
        |-- lib/
        |   |-- libmylib.so
        |   |-- libmylib.so.1
        |   `-- libmylib.so.1.0.0
        `-- share/
            `-- doc/
                `-- mylib/
                    `-- README
```

### Data Package

```
mypkg-data/
|-- CONTROL
|-- manifest.json
|-- checksums
`-- files/
    `-- usr/
        `-- share/
            `-- mypkg/
                |-- fonts/
                |   `-- custom.ttf
                `-- themes/
                    `-- default.css
```

## Troubleshooting

### Package Won't Install

1. Check archive format: `tar -tf package.txpkg`
2. Verify CONTROL file syntax
3. Ensure manifest.json is valid JSON
4. Check file permissions in archive

### Files Not Found After Install

1. Verify `files/` directory structure
2. Check installation prefix
3. Look for path issues (no leading `/`)

### Script Errors

1. Test scripts independently
2. Check for missing `$PREFIX` usage
3. Ensure scripts are executable
4. Test with `#!/bin/sh -x` for debugging
