/*
 * TX Package Manager v1.0 - Version Comparison Implementation
 *
 * Implements Debian dpkg-style version comparison.
 * Reference: Debian Policy Manual, Chapter 5 (Control Files and Their Fields)
 */

#include "version.h"
#include "error.h"
#include <ctype.h>

/*
 * Parse version string into components.
 * Format: [epoch:]upstream-version[-release]
 */
tx_status_t tx_version_parse(tx_version_t *ver, const char *str)
{
    if (!ver || !str)
        return TX_ERROR_INVALID_ARG;

    memset(ver, 0, sizeof(*ver));

    char buf[TX_MAX_VERSION * 2];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Trim leading/trailing whitespace */
    char *start = buf;
    while (isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) *end-- = '\0';

    if (*start == '\0') {
        TX_ERROR_SET_SIMPLE(TX_ERROR_PARSE, "Empty version string");
        return TX_ERROR_PARSE;
    }

    /* Check for epoch */
    char *colon = strchr(start, ':');
    if (colon) {
        *colon = '\0';
        char *epoch_end;
        unsigned long e = strtoul(start, &epoch_end, 10);
        if (*epoch_end != '\0' || epoch_end == start) {
            TX_ERROR_SET_SIMPLE(TX_ERROR_PARSE,
                "Invalid epoch in version string");
            return TX_ERROR_PARSE;
        }
        ver->epoch = (uint32_t)e;
        start = colon + 1;
    }

    /* Check for release (last hyphen) */
    char *hyphen = strrchr(start, '-');
    if (hyphen) {
        *hyphen = '\0';
        strncpy(ver->release, hyphen + 1, TX_MAX_VERSION - 1);
        ver->release[TX_MAX_VERSION - 1] = '\0';
    }

    /* Remaining is upstream version */
    strncpy(ver->upstream, start, TX_MAX_VERSION - 1);
    ver->upstream[TX_MAX_VERSION - 1] = '\0';

    if (ver->upstream[0] == '\0') {
        TX_ERROR_SET_SIMPLE(TX_ERROR_PARSE,
            "Missing upstream version");
        return TX_ERROR_PARSE;
    }

    return TX_OK;
}

/*
 * Convert version back to string representation.
 */
tx_status_t tx_version_to_string(const tx_version_t *ver,
                                  char *buf, size_t buf_size)
{
    if (!ver || !buf || buf_size == 0)
        return TX_ERROR_INVALID_ARG;

    if (ver->epoch > 0) {
        snprintf(buf, buf_size, "%u:%s-%s",
                 ver->epoch, ver->upstream, ver->release);
    } else if (ver->release[0]) {
        snprintf(buf, buf_size, "%s-%s",
                 ver->upstream, ver->release);
    } else {
        strncpy(buf, ver->upstream, buf_size - 1);
        buf[buf_size - 1] = '\0';
    }

    return TX_OK;
}

/*
 * Compare two version revision components character by character
 * following Debian version comparison rules.
 *
 * Tilde (~) sorts before anything, including empty string.
 * Letters sort after numbers but before other characters.
 */
static int verrevcmp(const char *a, const char *b)
{
    if (a == NULL) a = "";
    if (b == NULL) b = "";

    while (*a || *b) {
        int first_diff = 0;

        /* Skip non-alphanumeric in both */
        while ((*a && !isalnum((unsigned char)*a)) ||
               (*b && !isalnum((unsigned char)*b))) {
            int ac = 0, bc = 0;

            /* Tilde has special meaning - sorts before everything */
            if (*a == '~') ac = -1;
            else if (*a) ac = (unsigned char)*a;

            if (*b == '~') bc = -1;
            else if (*b) bc = (unsigned char)*b;

            if (ac != bc)
                return ac - bc;

            if (*a) a++;
            if (*b) b++;
        }

        /* Compare numeric blocks */
        if (isdigit((unsigned char)*a) || isdigit((unsigned char)*b)) {
            unsigned long na = 0, nb = 0;
            char *enda, *endb;

            na = strtoul(a, &enda, 10);
            nb = strtoul(b, &endb, 10);

            if (na != nb)
                return (na < nb) ? -1 : 1;

            /* Equal numerics - compare by length (leading zeros) */
            int la = (int)(enda - a);
            int lb = (int)(endb - b);
            if (la != lb && !first_diff)
                first_diff = la - lb;

            a = enda;
            b = endb;
        } else {
            /* Compare alphabetic blocks */
            while ((*a && *b) &&
                   (!isdigit((unsigned char)*a)) &&
                   (!isdigit((unsigned char)*b))) {
                int ac = (unsigned char)*a;
                int bc = (unsigned char)*b;

                /* Case-insensitive comparison for letters */
                if (isalpha((unsigned char)ac) && isalpha((unsigned char)bc)) {
                    ac = tolower((unsigned char)ac);
                    bc = tolower((unsigned char)bc);
                }

                if (ac != bc)
                    return ac - bc;

                a++;
                b++;
            }
        }
    }

    return 0;
}

/*
 * Compare two versions.
 * Returns: -1 if a < b, 0 if a == b, 1 if a > b
 */
int tx_version_compare(const tx_version_t *a, const tx_version_t *b)
{
    int r;

    /* Compare epochs first */
    if (a->epoch != b->epoch) {
        return (a->epoch < b->epoch) ? -1 : 1;
    }

    /* Compare upstream versions */
    r = verrevcmp(a->upstream, b->upstream);
    if (r != 0)
        return (r < 0) ? -1 : 1;

    /* Compare release numbers */
    r = verrevcmp(a->release, b->release);
    if (r != 0)
        return (r < 0) ? -1 : 1;

    return 0;
}

/*
 * Check if a version satisfies a relation.
 */
bool tx_version_satisfies(const tx_version_t *installed,
                          tx_relation_t rel,
                          const tx_version_t *required)
{
    int cmp = tx_version_compare(installed, required);

    switch (rel) {
        case TX_REL_EQ: return cmp == 0;
        case TX_REL_LT: return cmp < 0;
        case TX_REL_LE: return cmp <= 0;
        case TX_REL_GT: return cmp > 0;
        case TX_REL_GE: return cmp >= 0;
        case TX_REL_NE: return cmp != 0;
        case TX_REL_ANY: return true;
        default: return false;
    }
}

/*
 * Compare two version strings directly.
 */
int tx_version_compare_strings(const char *a, const char *b)
{
    tx_version_t va, vb;

    if (tx_version_parse(&va, a) != TX_OK)
        return 0;
    if (tx_version_parse(&vb, b) != TX_OK)
        return 0;

    return tx_version_compare(&va, &vb);
}

/*
 * Check if version string satisfies relation.
 */
bool tx_version_string_satisfies(const char *installed,
                                  tx_relation_t rel,
                                  const char *required)
{
    tx_version_t vi, vr;

    if (tx_version_parse(&vi, installed) != TX_OK)
        return false;
    if (tx_version_parse(&vr, required) != TX_OK)
        return false;

    return tx_version_satisfies(&vi, rel, &vr);
}

/*
 * Check if a string is a valid version format.
 */
bool tx_version_is_valid(const char *str)
{
    tx_version_t ver;
    return tx_version_parse(&ver, str) == TX_OK;
}

/*
 * Normalize a version string.
 */
tx_status_t tx_version_normalize(char *str, size_t len)
{
    if (!str || len == 0)
        return TX_ERROR_INVALID_ARG;

    tx_version_t ver;
    tx_status_t status = tx_version_parse(&ver, str);
    if (status != TX_OK)
        return status;

    return tx_version_to_string(&ver, str, len);
}

/*
 * Get the epoch from a version string.
 */
uint32_t tx_version_get_epoch(const char *str)
{
    tx_version_t ver;
    if (tx_version_parse(&ver, str) != TX_OK)
        return 0;
    return ver.epoch;
}

/*
 * Check if a version represents a downgrade.
 */
bool tx_version_is_downgrade(const tx_version_t *current,
                              const tx_version_t *candidate)
{
    return tx_version_compare(candidate, current) < 0;
}
