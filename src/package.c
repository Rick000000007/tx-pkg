/*
 * TX Package Manager v1.0 - Package Data Structures Implementation
 */

#include "package.h"
#include "error.h"
#include "vendor/cjson.h"
#include <ctype.h>
#include <sys/stat.h>

/* ==================================================================
 * Manifest Operations
 * ================================================================== */

void tx_manifest_init(tx_manifest_t *m)
{
    if (!m) return;
    memset(m, 0, sizeof(*m));
}

void tx_manifest_free(tx_manifest_t *m)
{
    if (!m) return;

    tx_dep_free_list(m->depends);
    tx_dep_free_list(m->pre_depends);
    tx_dep_free_list(m->provides);
    tx_dep_free_list(m->conflicts);
    tx_dep_free_list(m->replaces);
    tx_dep_free_list(m->breaks);
    tx_dep_free_list(m->suggests);
    tx_dep_free_list(m->recommends);
    tx_dep_free_list(m->optional);
    tx_dep_free_list(m->enhances);

    free(m->files);
    m->files = NULL;
    m->file_count = 0;
    m->file_capacity = 0;
}

tx_status_t tx_manifest_add_dependency(tx_manifest_t *m,
    tx_dep_kind_t kind, const char *name,
    tx_relation_t rel, const char *version_str)
{
    if (!m || !name)
        return TX_ERROR_INVALID_ARG;

    tx_dependency_t *new_dep = calloc(1, sizeof(tx_dependency_t));
    if (!new_dep)
        return TX_ERROR_GENERAL;

    strncpy(new_dep->name, name, TX_MAX_PACKAGE_NAME - 1);
    new_dep->relation = rel;
    new_dep->kind = kind;

    if (version_str && version_str[0]) {
        tx_status_t s = tx_version_parse(&new_dep->version, version_str);
        if (s != TX_OK) {
            free(new_dep);
            return s;
        }
    }

    tx_dependency_t **list = NULL;
    switch (kind) {
        case TX_DEP_DEPENDS:     list = &m->depends; break;
        case TX_DEP_PRE_DEPENDS: list = &m->pre_depends; break;
        case TX_DEP_PROVIDES:    list = &m->provides; break;
        case TX_DEP_CONFLICTS:   list = &m->conflicts; break;
        case TX_DEP_REPLACES:    list = &m->replaces; break;
        case TX_DEP_BREAKS:      list = &m->breaks; break;
        case TX_DEP_SUGGESTS:    list = &m->suggests; break;
        case TX_DEP_RECOMMENDS:  list = &m->recommends; break;
        case TX_DEP_OPTIONAL:    list = &m->optional; break;
        case TX_DEP_ENHANCES:    list = &m->enhances; break;
        default:
            free(new_dep);
            return TX_ERROR_INVALID_ARG;
    }

    /* Append to list */
    new_dep->next = *list;
    *list = new_dep;

    return TX_OK;
}

tx_dependency_t *tx_manifest_get_deps(const tx_manifest_t *m,
    tx_dep_kind_t kind)
{
    if (!m) return NULL;

    switch (kind) {
        case TX_DEP_DEPENDS:     return m->depends;
        case TX_DEP_PRE_DEPENDS: return m->pre_depends;
        case TX_DEP_PROVIDES:    return m->provides;
        case TX_DEP_CONFLICTS:   return m->conflicts;
        case TX_DEP_REPLACES:    return m->replaces;
        case TX_DEP_BREAKS:      return m->breaks;
        case TX_DEP_SUGGESTS:    return m->suggests;
        case TX_DEP_RECOMMENDS:  return m->recommends;
        case TX_DEP_OPTIONAL:    return m->optional;
        case TX_DEP_ENHANCES:    return m->enhances;
        default:                 return NULL;
    }
}

/* ==================================================================
 * Dependency Parsing
 * ================================================================== */

tx_relation_t tx_relation_parse(const char *str)
{
    if (!str) return TX_REL_ANY;
    if (strcmp(str, ">=") == 0 || strcmp(str, ">>=") == 0) return TX_REL_GE;
    if (strcmp(str, "<=") == 0 || strcmp(str, "<<=") == 0) return TX_REL_LE;
    if (strcmp(str, ">>") == 0 || strcmp(str, ">") == 0)   return TX_REL_GT;
    if (strcmp(str, "<<") == 0 || strcmp(str, "<") == 0)   return TX_REL_LT;
    if (strcmp(str, "=") == 0 || strcmp(str, "==") == 0)   return TX_REL_EQ;
    if (strcmp(str, "!=") == 0) return TX_REL_NE;
    return TX_REL_ANY;
}

const char *tx_relation_to_string(tx_relation_t rel)
{
    switch (rel) {
        case TX_REL_EQ: return "=";
        case TX_REL_LT: return "<<";
        case TX_REL_LE: return "<=";
        case TX_REL_GT: return ">>";
        case TX_REL_GE: return ">=";
        case TX_REL_NE: return "!=";
        default:        return "";
    }
}

/*
 * Parse a single dependency declaration.
 * Format: "pkgname" or "pkgname (>= version)"
 */
tx_status_t tx_dep_parse(const char *str, tx_dependency_t *dep)
{
    if (!str || !dep)
        return TX_ERROR_INVALID_ARG;

    memset(dep, 0, sizeof(*dep));

    char buf[TX_MAX_DEPENDENCY];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Trim leading whitespace */
    char *p = buf;
    while (isspace((unsigned char)*p)) p++;

    /* Extract package name */
    char *name_end = p;
    while (*name_end && !isspace((unsigned char)*name_end) &&
           *name_end != '(' && *name_end != ',')
        name_end++;

    size_t name_len = name_end - p;
    if (name_len == 0 || name_len >= TX_MAX_PACKAGE_NAME) {
        TX_ERROR_SET_SIMPLE(TX_ERROR_PARSE,
            "Invalid dependency name in declaration");
        return TX_ERROR_PARSE;
    }

    strncpy(dep->name, p, name_len);
    dep->name[name_len] = '\0';

    /* Check for version relation */
    p = name_end;
    while (isspace((unsigned char)*p)) p++;

    if (*p == '(') {
        p++;
        while (isspace((unsigned char)*p)) p++;

        /* Extract relation operator */
        char rel_str[8] = "";
        int rel_i = 0;
        while (*p && !isspace((unsigned char)*p) && *p != ')' && rel_i < 7) {
            rel_str[rel_i++] = *p++;
        }
        rel_str[rel_i] = '\0';

        dep->relation = tx_relation_parse(rel_str);

        while (isspace((unsigned char)*p)) p++;

        /* Extract version */
        char ver_str[TX_MAX_VERSION] = "";
        int ver_i = 0;
        while (*p && *p != ')' && ver_i < TX_MAX_VERSION - 1) {
            ver_str[ver_i++] = *p++;
        }
        ver_str[ver_i] = '\0';

        /* Trim trailing whitespace from version */
        char *vend = ver_str + strlen(ver_str) - 1;
        while (vend > ver_str && isspace((unsigned char)*vend))
            *vend-- = '\0';

        if (ver_str[0]) {
            tx_status_t s = tx_version_parse(&dep->version, ver_str);
            if (s != TX_OK)
                return s;
        }
    }

    return TX_OK;
}

/*
 * Parse a comma-separated list of dependencies.
 */
tx_status_t tx_dep_parse_list(const char *str,
    tx_dep_kind_t kind, tx_manifest_t *m)
{
    if (!str || !m)
        return TX_ERROR_INVALID_ARG;

    if (str[0] == '\0')
        return TX_OK;

    char *buf = strdup(str);
    if (!buf)
        return TX_ERROR_GENERAL;

    char *saveptr = NULL;
    char *token = strtok_r(buf, ",", &saveptr);

    while (token) {
        tx_dependency_t dep;
        tx_status_t s = tx_dep_parse(token, &dep);
        if (s == TX_OK) {
            dep.kind = kind;
            tx_manifest_add_dependency(m, kind, dep.name,
                dep.relation,
                dep.version.upstream[0] ? dep.version.upstream : NULL);
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    free(buf);
    return TX_OK;
}

void tx_dep_free_list(tx_dependency_t *deps)
{
    while (deps) {
        tx_dependency_t *next = deps->next;
        free(deps);
        deps = next;
    }
}

tx_status_t tx_dep_copy(tx_dependency_t *dst,
    const tx_dependency_t *src)
{
    if (!dst || !src)
        return TX_ERROR_INVALID_ARG;
    memcpy(dst, src, sizeof(*dst));
    dst->next = NULL;
    return TX_OK;
}

/* ==================================================================
 * Installed Package Operations
 * ================================================================== */

void tx_installed_pkg_init(tx_installed_pkg_t *pkg)
{
    if (!pkg) return;
    memset(pkg, 0, sizeof(*pkg));
    pkg->status = TX_PKG_STATUS_NOT_INSTALLED;
}

void tx_installed_pkg_free(tx_installed_pkg_t *pkg)
{
    if (!pkg) return;
    free(pkg->files);
    if (pkg->reverse_deps) {
        for (size_t i = 0; i < pkg->reverse_dep_count; i++)
            free(pkg->reverse_deps[i]);
        free(pkg->reverse_deps);
    }
    memset(pkg, 0, sizeof(*pkg));
}

tx_status_t tx_manifest_to_installed(const tx_manifest_t *m,
    tx_installed_pkg_t *pkg)
{
    if (!m || !pkg)
        return TX_ERROR_INVALID_ARG;

    tx_installed_pkg_init(pkg);

    strncpy(pkg->name, m->name, TX_MAX_PACKAGE_NAME - 1);
    memcpy(&pkg->version, &m->version, sizeof(tx_version_t));
    strncpy(pkg->architecture, m->architecture, 31);
    strncpy(pkg->description, m->description,
            TX_MAX_DESCRIPTION - 1);
    strncpy(pkg->repository, m->repository, TX_MAX_PACKAGE_NAME - 1);
    strncpy(pkg->channel, m->channel, 31);
    strncpy(pkg->package_sha256, m->package_sha256, 64);
    pkg->installed_size = m->installed_size;
    pkg->is_essential = m->is_essential;
    pkg->status = TX_PKG_STATUS_INSTALLED;
    pkg->install_date = time(NULL);
    pkg->update_date = pkg->install_date;

    return TX_OK;
}

/* ==================================================================
 * CONTROL File Parsing
 * ================================================================== */

tx_status_t tx_parse_control_file(const char *path,
    tx_manifest_t *m)
{
    if (!path || !m)
        return TX_ERROR_INVALID_ARG;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        TX_ERROR_SET(TX_ERROR_IO, "Cannot open CONTROL file",
                     path, strerror(errno),
                     "Check that the file exists and is readable.");
        return TX_ERROR_IO;
    }

    char line[1024];
    char current_key[64] = "";
    char current_val[1024] = "";

    while (fgets(line, sizeof(line), fp)) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        /* Continuation line */
        if (line[0] == ' ' || line[0] == '\t') {
            if (current_key[0]) {
                strncat(current_val, " ",
                        sizeof(current_val) - strlen(current_val) - 1);
                char *val_start = line;
                while (*val_start == ' ' || *val_start == '\t')
                    val_start++;
                strncat(current_val, val_start,
                        sizeof(current_val) - strlen(current_val) - 1);
            }
            continue;
        }

        /* Process previous key-value pair */
        if (current_key[0]) {
            if (strcmp(current_key, "Package") == 0)
                strncpy(m->name, current_val, TX_MAX_PACKAGE_NAME - 1);
            else if (strcmp(current_key, "Version") == 0)
                tx_version_parse(&m->version, current_val);
            else if (strcmp(current_key, "Release") == 0) {
                char full_ver[TX_MAX_VERSION * 2];
                snprintf(full_ver, sizeof(full_ver), "%s-%s",
                         m->version.upstream, current_val);
                tx_version_parse(&m->version, full_ver);
            }
            else if (strcmp(current_key, "Architecture") == 0)
                strncpy(m->architecture, current_val, 31);
            else if (strcmp(current_key, "License") == 0)
                strncpy(m->license, current_val, 63);
            else if (strcmp(current_key, "Description") == 0)
                strncpy(m->description, current_val,
                        TX_MAX_DESCRIPTION - 1);
            else if (strcmp(current_key, "Homepage") == 0)
                strncpy(m->homepage, current_val, TX_MAX_URL - 1);
            else if (strcmp(current_key, "Maintainer") == 0)
                strncpy(m->maintainer, current_val, 255);
            else if (strcmp(current_key, "Category") == 0)
                strncpy(m->category, current_val, 63);
            else if (strcmp(current_key, "Section") == 0)
                strncpy(m->section, current_val, 63);
            else if (strcmp(current_key, "Depends") == 0)
                tx_dep_parse_list(current_val, TX_DEP_DEPENDS, m);
            else if (strcmp(current_key, "Pre-Depends") == 0)
                tx_dep_parse_list(current_val, TX_DEP_PRE_DEPENDS, m);
            else if (strcmp(current_key, "Provides") == 0)
                tx_dep_parse_list(current_val, TX_DEP_PROVIDES, m);
            else if (strcmp(current_key, "Conflicts") == 0)
                tx_dep_parse_list(current_val, TX_DEP_CONFLICTS, m);
            else if (strcmp(current_key, "Replaces") == 0)
                tx_dep_parse_list(current_val, TX_DEP_REPLACES, m);
            else if (strcmp(current_key, "Breaks") == 0)
                tx_dep_parse_list(current_val, TX_DEP_BREAKS, m);
            else if (strcmp(current_key, "Suggests") == 0)
                tx_dep_parse_list(current_val, TX_DEP_SUGGESTS, m);
            else if (strcmp(current_key, "Recommends") == 0)
                tx_dep_parse_list(current_val, TX_DEP_RECOMMENDS, m);
            else if (strcmp(current_key, "Optional") == 0)
                tx_dep_parse_list(current_val, TX_DEP_OPTIONAL, m);
            else if (strcmp(current_key, "Essential") == 0)
                m->is_essential =
                    (strcmp(current_val, "yes") == 0);
        }

        /* Parse new key-value */
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            strncpy(current_key, line, 63);
            current_key[63] = '\0';

            /* Trim key */
            char *kend = current_key + strlen(current_key) - 1;
            while (kend > current_key &&
                   isspace((unsigned char)*kend))
                *kend-- = '\0';

            char *val = colon + 1;
            while (*val == ' ') val++;
            strncpy(current_val, val, sizeof(current_val) - 1);
            current_val[sizeof(current_val) - 1] = '\0';
        } else {
            current_key[0] = '\0';
            current_val[0] = '\0';
        }
    }

    /* Process last key-value pair */
    if (current_key[0]) {
        if (strcmp(current_key, "Package") == 0)
            strncpy(m->name, current_val, TX_MAX_PACKAGE_NAME - 1);
        else if (strcmp(current_key, "Version") == 0)
            tx_version_parse(&m->version, current_val);
    }

    fclose(fp);
    return TX_OK;
}

/* ==================================================================
 * JSON Manifest Parsing
 * ================================================================== */

tx_status_t tx_parse_manifest_json(const char *path,
    tx_manifest_t *m)
{
    if (!path || !m)
        return TX_ERROR_INVALID_ARG;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        TX_ERROR_SET(TX_ERROR_IO, "Cannot open manifest.json",
                     path, strerror(errno),
                     "Ensure the package archive is valid.");
        return TX_ERROR_IO;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        fclose(fp);
        return TX_ERROR_GENERAL;
    }

    fread(json_str, 1, fsize, fp);
    json_str[fsize] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        TX_ERROR_SET_SIMPLE(TX_ERROR_PARSE,
            "Invalid JSON in manifest file");
        return TX_ERROR_PARSE;
    }

    cJSON *item;

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "name")))
        strncpy(m->name, item->valuestring, TX_MAX_PACKAGE_NAME - 1);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "version"))) {
        char ver_str[TX_MAX_VERSION * 2] = "";
        strncpy(ver_str, item->valuestring, sizeof(ver_str) - 1);

        cJSON *rel = cJSON_GetObjectItemCaseSensitive(root, "release");
        if (rel && cJSON_IsNumber(rel)) {
            char tmp[TX_MAX_VERSION * 2];
            snprintf(tmp, sizeof(tmp), "%s-%d",
                     ver_str, rel->valueint);
            strncpy(ver_str, tmp, sizeof(ver_str) - 1);
        }

        tx_version_parse(&m->version, ver_str);
    }

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "architecture")))
        strncpy(m->architecture, item->valuestring, 31);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "license")))
        strncpy(m->license, item->valuestring, 63);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "description")))
        strncpy(m->description, item->valuestring,
                TX_MAX_DESCRIPTION - 1);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "homepage")))
        strncpy(m->homepage, item->valuestring, TX_MAX_URL - 1);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "maintainer")))
        strncpy(m->maintainer, item->valuestring, 255);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "category")))
        strncpy(m->category, item->valuestring, 63);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "section")))
        strncpy(m->section, item->valuestring, 63);

    /* Parse dependencies */
    if ((item = cJSON_GetObjectItemCaseSensitive(root, "depends")))
        tx_dep_parse_list(item->valuestring, TX_DEP_DEPENDS, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "pre_depends")))
        tx_dep_parse_list(item->valuestring, TX_DEP_PRE_DEPENDS, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "provides")))
        tx_dep_parse_list(item->valuestring, TX_DEP_PROVIDES, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "conflicts")))
        tx_dep_parse_list(item->valuestring, TX_DEP_CONFLICTS, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "replaces")))
        tx_dep_parse_list(item->valuestring, TX_DEP_REPLACES, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "breaks")))
        tx_dep_parse_list(item->valuestring, TX_DEP_BREAKS, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "suggests")))
        tx_dep_parse_list(item->valuestring, TX_DEP_SUGGESTS, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "recommends")))
        tx_dep_parse_list(item->valuestring, TX_DEP_RECOMMENDS, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "optional")))
        tx_dep_parse_list(item->valuestring, TX_DEP_OPTIONAL, m);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "sha256")))
        strncpy(m->package_sha256, item->valuestring, 64);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "size")))
        m->package_size = (size_t)item->valueint;

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "installed_size")))
        m->installed_size = (size_t)item->valueint;

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "essential")))
        m->is_essential = cJSON_IsTrue(item);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "repository")))
        strncpy(m->repository, item->valuestring, TX_MAX_PACKAGE_NAME - 1);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "channel")))
        strncpy(m->channel, item->valuestring, 31);

    if ((item = cJSON_GetObjectItemCaseSensitive(root, "filename")))
        strncpy(m->filename, item->valuestring, TX_MAX_FILENAME - 1);

    cJSON_Delete(root);
    return TX_OK;
}

tx_status_t tx_write_manifest_json(const char *path,
    const tx_manifest_t *m)
{
    if (!path || !m)
        return TX_ERROR_INVALID_ARG;

    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", m->name);

    char ver_str[TX_MAX_VERSION * 2];
    tx_version_to_string(&m->version, ver_str, sizeof(ver_str));
    cJSON_AddStringToObject(root, "version", m->version.upstream);
    cJSON_AddNumberToObject(root, "release",
        atoi(m->version.release));

    cJSON_AddStringToObject(root, "architecture", m->architecture);
    cJSON_AddStringToObject(root, "license", m->license);
    cJSON_AddStringToObject(root, "description", m->description);
    cJSON_AddStringToObject(root, "homepage", m->homepage);
    cJSON_AddStringToObject(root, "maintainer", m->maintainer);
    cJSON_AddStringToObject(root, "category", m->category);
    cJSON_AddStringToObject(root, "section", m->section);

    /* Serialize dependencies */
    tx_dependency_t *dep;
    char dep_str[TX_MAX_DEPENDENCY] = "";

    dep = m->depends;
    dep_str[0] = '\0';
    while (dep) {
        if (dep_str[0]) strncat(dep_str, ", ",
                                sizeof(dep_str) - strlen(dep_str) - 1);
        strncat(dep_str, dep->name, sizeof(dep_str) - strlen(dep_str) - 1);
        if (dep->relation != TX_REL_ANY && dep->version.upstream[0]) {
            char rel_ver[TX_MAX_VERSION + 8];
            snprintf(rel_ver, sizeof(rel_ver), " (%s %s)",
                     tx_relation_to_string(dep->relation),
                     dep->version.upstream);
            strncat(dep_str, rel_ver,
                    sizeof(dep_str) - strlen(dep_str) - 1);
        }
        dep = dep->next;
    }
    if (dep_str[0])
        cJSON_AddStringToObject(root, "depends", dep_str);

    if (m->package_sha256[0])
        cJSON_AddStringToObject(root, "sha256", m->package_sha256);

    cJSON_AddNumberToObject(root, "size", (double)m->package_size);
    cJSON_AddNumberToObject(root, "installed_size",
                            (double)m->installed_size);
    cJSON_AddBoolToObject(root, "essential", m->is_essential);

    if (m->repository[0])
        cJSON_AddStringToObject(root, "repository", m->repository);

    if (m->channel[0])
        cJSON_AddStringToObject(root, "channel", m->channel);

    if (m->filename[0])
        cJSON_AddStringToObject(root, "filename", m->filename);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str)
        return TX_ERROR_GENERAL;

    FILE *fp = fopen(path, "w");
    if (!fp) {
        free(json_str);
        return TX_ERROR_IO;
    }

    fprintf(fp, "%s\n", json_str);
    fclose(fp);
    free(json_str);

    return TX_OK;
}
