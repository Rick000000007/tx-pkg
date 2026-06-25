/*
 * TX Package Manager v1.0 - Build Configuration
 *
 * Copyright (c) 2026 TX Terminal Project
 * Licensed under the MIT License
 */

#ifndef TX_CONFIG_H
#define TX_CONFIG_H

/* ==================================================================
 * Version Information
 * ================================================================== */

#define TX_PKG_NAME        "tx-pkg"
#define TX_PKG_VERSION     "1.0.0"
#define TX_PKG_RELEASE     1
#define TX_PKG_AUTHOR      "TX Terminal Project"
#define TX_PKG_LICENSE     "MIT"
#define TX_PKG_HOMEPAGE    "https://txterminal.dev"

/* ==================================================================
 * Installation Prefix Configuration
 *
 * Build-time prefix selection:
 *   - Linux standard:     /usr
 *   - Termux/TX Terminal: /data/data/com.termux/files/usr
 *   - Custom:             override at compile time
 *
 * Compile with -DTX_PREFIX=/your/prefix to override.
 * ================================================================== */

#ifndef TX_PREFIX
#  ifdef __ANDROID__
#    define TX_PREFIX      "/data/data/com.termux/files/usr"
#  else
#    define TX_PREFIX      "/usr/local"
#  endif
#endif

#define TX_PREFIX_BIN      TX_PREFIX "/bin"
#define TX_PREFIX_LIB      TX_PREFIX "/lib"
#define TX_PREFIX_SHARE    TX_PREFIX "/share"
#define TX_PREFIX_ETC      TX_PREFIX "/etc"

/* ==================================================================
 * TX Package Manager Runtime Paths
 * ================================================================== */

#ifndef TX_ROOT
#  ifdef __ANDROID__
#    define TX_ROOT        "/data/data/com.termux/files/home/.tx"
#  else
#    define TX_ROOT        TX_PREFIX_ETC "/tx"
#  endif
#endif

#define TX_VAR_DIR         TX_ROOT "/var"
#define TX_LIB_DIR         TX_VAR_DIR "/lib/txpkg"
#define TX_DB_FILE         TX_LIB_DIR "/txpkg.db"
#define TX_CACHE_DIR       TX_ROOT "/.cache/tx-pkg"
#define TX_PKG_CACHE       TX_CACHE_DIR "/packages"
#define TX_EXTRACT_DIR     TX_CACHE_DIR "/extract"
#define TX_LOCK_DIR        TX_VAR_DIR "/run"
#define TX_LOCK_FILE       TX_LOCK_DIR "/txpkg.lock"
#define TX_BACKUP_DIR      TX_VAR_DIR "/backup/txpkg"
#define TX_JOURNAL_DIR     TX_VAR_DIR "/journal/txpkg"
#define TX_CONFIG_FILE     TX_ETC_DIR "/txpkg.conf"
#define TX_ETC_DIR         TX_ROOT "/etc/txpkg"
#define TX_REPO_CONFIG     TX_ETC_DIR "/repositories.conf"

/* ==================================================================
 * Default Repository Configuration
 * ================================================================== */

#define TX_DEFAULT_REPO_URL      "https://rick000000007.github.io/tx-packages"
#define TX_DEFAULT_CHANNEL       "stable"
#define TX_DEFAULT_ARCH          "aarch64"

/* ==================================================================
 * Package Format Configuration
 * ================================================================== */

#define TX_PKG_EXTENSION         ".txpkg"
#define TX_PKG_ARCHIVE_FORMAT    "tar.xz"
#define TX_CONTROL_FILE          "CONTROL"
#define TX_MANIFEST_FILE         "manifest.json"
#define TX_CHECKSUMS_FILE        "checksums"
#define TX_PREINST_SCRIPT        "preinst"
#define TX_POSTINST_SCRIPT       "postinst"
#define TX_PRERM_SCRIPT          "prerm"
#define TX_POSTRM_SCRIPT         "postrm"
#define TX_PAYLOAD_DIR           "files/"

/* ==================================================================
 * Repository Metadata Configuration
 * ================================================================== */

#define TX_REPO_PACKAGES_FILE    "Packages.json"
#define TX_REPO_INDEX_FILE       "index.json"
#define TX_REPO_CHECKSUMS_FILE   "SHA256SUMS"
#define TX_REPO_FORMAT_VERSION   2
#define TX_PKG_METADATA_VERSION  2

/* ==================================================================
 * Operational Limits
 * ================================================================== */

#define TX_MAX_PACKAGE_NAME      128
#define TX_MAX_VERSION           64
#define TX_MAX_DESCRIPTION       1024
#define TX_MAX_FILENAME          512
#define TX_MAX_PATH              4096
#define TX_MAX_URL               2048
#define TX_MAX_DEPENDENCY        512
#define TX_MAX_PACKAGES          8192
#define TX_MAX_REPOSITORIES      32
#define TX_MAX_CHANNELS          8
#define TX_MAX_FILES_PER_PKG     65536
#define TX_MAX_SCRIPT_SIZE       65536

/* ==================================================================
 * Cache Configuration
 * ================================================================== */

#define TX_CACHE_MAX_AGE_DAYS    30
#define TX_CACHE_CLEAN_INTERVAL  7

/* ==================================================================
 * Lock Configuration
 * ================================================================== */

#define TX_LOCK_TIMEOUT_SECONDS  300
#define TX_LOCK_RETRY_INTERVAL_MS 100
#define TX_LOCK_MAX_RETRIES      3000

/* ==================================================================
 * Download Configuration
 * ================================================================== */

#define TX_DOWNLOAD_TIMEOUT      300
#define TX_DOWNLOAD_MAX_RETRIES  3
#define TX_DOWNLOAD_USER_AGENT   "tx-pkg/" TX_PKG_VERSION

/* ==================================================================
 * Security Configuration
 * ================================================================== */

#define TX_VERIFY_SHA256         1
#define TX_VERIFY_MANIFEST       1
#define TX_VERIFY_PATHS          1
#define TX_VERIFY_SCRIPTS        1

/* ==================================================================
 * Feature Flags
 * ================================================================== */

#ifndef TX_ENABLE_SIGNATURES
#define TX_ENABLE_SIGNATURES     0
#endif

#ifndef TX_ENABLE_DELTA_UPDATES
#define TX_ENABLE_DELTA_UPDATES  0
#endif

/* ==================================================================
 * Debug and Development
 * ================================================================== */

#ifdef TX_DEBUG
#define TX_DEBUG_LOG(...)        fprintf(stderr, "[DEBUG] " __VA_ARGS__)
#else
#define TX_DEBUG_LOG(...)        ((void)0)
#endif

#endif /* TX_CONFIG_H */
