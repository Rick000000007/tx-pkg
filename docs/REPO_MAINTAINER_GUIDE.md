# TX Repository Maintainer Guide

## Overview

This guide covers setting up and maintaining a TX package repository.

## Repository Setup

### Directory Structure

```
/var/www/tx-repo/
|-- repo/
|   |-- stable/
|   |   |-- index.json
|   |   |-- Packages.json
|   |   |-- SHA256SUMS
|   |   `-- packages/
|   `-- testing/
|       |-- index.json
|       |-- Packages.json
|       |-- SHA256SUMS
|       `-- packages/
```

### Initial Setup

```bash
# Create directories
mkdir -p repo/stable/packages
mkdir -p repo/testing/packages

# Generate initial metadata
pkg txrepo repo/stable/
pkg txrepo repo/testing/
```

## Adding Packages

### Step 1: Copy Package Files

```bash
cp package-1.0.0-1.aarch64.txpkg repo/stable/packages/
```

### Step 2: Generate Per-Package Metadata

```bash
cd repo/stable/packages/

# Extract and create .json metadata
tar -xf package-1.0.0-1.aarch64.txpkg -C /tmp/pkg-extract
cp /tmp/pkg-extract/manifest.json ./package-1.0.0-1.aarch64.json

# Create .sha256 file
sha256sum package-1.0.0-1.aarch64.txpkg > package-1.0.0-1.aarch64.txpkg.sha256
```

### Step 3: Regenerate Repository Index

```bash
pkg txrepo repo/stable/
```

This updates:
- `Packages.json` - Scans packages/ and builds catalog
- `SHA256SUMS` - Checksums all packages
- `index.json` - Updates metadata

## Repository Metadata

### Packages.json

Generated automatically by `pkg txrepo`. Contains all package metadata.

### SHA256SUMS

Format:
```
<sha256>  packages/<filename>
```

Verify with:
```bash
cd repo/stable && sha256sum -c SHA256SUMS
```

### index.json

Update manually or via build script:
```json
{
  "repository": "My TX Repository",
  "format_version": 2,
  "architecture": "aarch64",
  "channel": "stable",
  "url": "https://myrepo.example.com",
  "packages": 42,
  "generated": "2026-06-26T00:00:00Z"
}
```

## Automation

### Build Script

```bash
#!/bin/bash
# build-repo.sh - Automated repository builder

set -e

REPO_DIR="${1:-repo/stable}"
CHANNEL="$(basename $REPO_DIR)"
ARCH="aarch64"

echo "Building repository: $CHANNEL"

# Generate package metadata
cd "$REPO_DIR/packages" || exit 1

for pkg in *.txpkg; do
    [ -f "$pkg" ] || continue
    
    echo "Processing $pkg..."
    
    # Extract manifest if not exists
    json="${pkg%.txpkg}.json"
    if [ ! -f "$json" ]; then
        tar -xf "$pkg" -C /tmp/tx-pkg-tmp manifest.json 2>/dev/null || true
        if [ -f /tmp/tx-pkg-tmp/manifest.json ]; then
            mv /tmp/tx-pkg-tmp/manifest.json "$json"
        fi
    fi
    
    # Generate SHA256 if not exists
    sha="${pkg}.sha256"
    if [ ! -f "$sha" ]; then
        sha256sum "$pkg" > "$sha"
    fi
done

cd ../..

# Build catalog
cat > "$REPO_DIR/Packages.json" << 'HEADER'
{
  "packages": [
HEADER

first=1
for json in "$REPO_DIR/packages"/*.json; do
    [ -f "$json" ] || continue
    
    if [ $first -eq 1 ]; then
        first=0
    else
        echo "," >> "$REPO_DIR/Packages.json"
    fi
    
    cat "$json" >> "$REPO_DIR/Packages.json"
done

cat >> "$REPO_DIR/Packages.json" << 'FOOTER'

  ]
}
FOOTER

# Generate SHA256SUMS
cd "$REPO_DIR"
sha256sum packages/*.txpkg > SHA256SUMS

echo "Repository build complete."
echo "Packages: $(ls packages/*.txpkg 2>/dev/null | wc -l)"
```

### CI/CD Integration

GitHub Actions example:

```yaml
name: Build Repository

on:
  push:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install tx-pkg
        run: |
          git clone https://github.com/Rick000000007/tx-pkg.git
          cd tx-pkg && make && sudo make install

      - name: Build repository
        run: ./scripts/build-repo.sh

      - name: Deploy to GitHub Pages
        uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./repo
```

## Channel Management

### Promoting Packages

```bash
# Copy from testing to stable
cp repo/testing/packages/pkg-*.txpkg repo/stable/packages/

# Regenerate both
pkg txrepo repo/stable/
pkg txrepo repo/testing/
```

### Nightly Builds

```bash
# Automated nightly from CI
CHANNEL=nightly
DATE=$(date +%Y%m%d)

# Build packages with date-stamped versions
for pkg in packages/*/; do
    cd "$pkg"
    sed -i "s/Release: .*/Release: ${DATE}/" CONTROL
    pkg txbuild .
    cd ../..
done

# Update repository
pkg txrepo "repo/$CHANNEL/"
```

## Security

### HTTPS

Always serve repositories over HTTPS:
```
https://repo.example.com/tx/repo/stable/Packages.json
```

### Package Verification

Verify all packages before adding:
```bash
sha256sum -c SHA256SUMS
pkg verify ./package.txpkg
```

### Future: GPG Signing

Repository signing will be supported via `TX_ENABLE_SIGNATURES`:
```bash
# Generate signing key
gpg --gen-key

# Sign metadata
gpg --detach-sign -a Packages.json
gpg --detach-sign -a SHA256SUMS
```

## Monitoring

### Health Checks

```bash
# Verify repository integrity
cd repo/stable
sha256sum -c SHA256SUMS

# Check for broken metadata
python3 -c "import json; json.load(open('Packages.json'))"
```

### Statistics

```bash
# Package count
jq '.packages | length' repo/stable/Packages.json

# Total size
awk '{sum+=$1} END {print sum/1024/1024 " MB"}' repo/stable/SHA256SUMS
```

## Troubleshooting

### Package Not Found

1. Check Packages.json contains the package
2. Verify filename matches
3. Confirm SHA256SUMS entry exists

### Metadata Errors

1. Validate JSON syntax: `python3 -m json.tool Packages.json`
2. Check for required fields
3. Ensure consistent architecture entries

### Download Failures

1. Verify HTTP server configuration
2. Check CORS headers if applicable
3. Ensure correct MIME types:
   - `.txpkg` -> `application/octet-stream`
   - `.json` -> `application/json`
