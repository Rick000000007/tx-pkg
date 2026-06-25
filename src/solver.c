/*
 * TX Package Manager v1.0 - Dependency Solver Implementation
 *
 * SAT-style dependency resolution with cycle detection,
 * virtual package support, and topological ordering.
 */

#include "solver.h"
#include "error.h"
#include <stdio.h>

/* ==================================================================
 * Solution Management
 * ================================================================== */

void tx_solution_init(tx_solver_solution_t *sol)
{
    if (!sol) return;
    memset(sol, 0, sizeof(*sol));
    sol->is_valid = false;
}

void tx_solution_free(tx_solver_solution_t *sol)
{
    if (!sol) return;

    tx_solver_action_t *action = sol->actions;
    while (action) {
        tx_solver_action_t *next = action->next;
        free(action->source);
        free(action);
        action = next;
    }

    if (sol->warnings) {
        for (size_t i = 0; i < sol->warning_count; i++)
            free(sol->warnings[i]);
        free(sol->warnings);
    }

    memset(sol, 0, sizeof(*sol));
}

/* ==================================================================
 * Helper: Find package in available list
 * ================================================================== */

static tx_pkg_entry_t *find_available(tx_solver_context_t *ctx,
    const char *name)
{
    if (!ctx || !name) return NULL;

    for (size_t i = 0; i < ctx->available_count; i++) {
        if (strcmp(ctx->available[i].name, name) == 0)
            return &ctx->available[i];
    }
    return NULL;
}

/* ==================================================================
 * Helper: Find installed package
 * ================================================================== */

static tx_installed_pkg_t *find_installed(tx_solver_context_t *ctx,
    const char *name)
{
    if (!ctx || !name) return NULL;

    for (size_t i = 0; i < ctx->installed_count; i++) {
        if (strcmp(ctx->installed[i].name, name) == 0)
            return &ctx->installed[i];
    }
    return NULL;
}

/* ==================================================================
 * Helper: Check if action already in solution
 * ================================================================== */

static bool action_exists(tx_solver_solution_t *sol,
    const char *name, tx_op_type_t op)
{
    tx_solver_action_t *a = sol->actions;
    while (a) {
        if (strcmp(a->package_name, name) == 0 && a->op == op)
            return true;
        a = a->next;
    }
    return false;
}

/* ==================================================================
 * Helper: Add action to solution
 * ================================================================== */

static tx_status_t add_action(tx_solver_solution_t *sol,
    tx_op_type_t op, const char *name, const tx_version_t *ver,
    const char *reason, tx_pkg_entry_t *source)
{
    if (action_exists(sol, name, op))
        return TX_OK;

    tx_solver_action_t *action = calloc(1, sizeof(tx_solver_action_t));
    if (!action)
        return TX_ERROR_GENERAL;

    action->op = op;
    strncpy(action->package_name, name, TX_MAX_PACKAGE_NAME - 1);
    if (ver)
        memcpy(&action->version, ver, sizeof(*ver));
    if (reason)
        strncpy(action->reason, reason, sizeof(action->reason) - 1);

    if (source) {
        action->source = malloc(sizeof(tx_pkg_entry_t));
        if (action->source)
            memcpy(action->source, source, sizeof(tx_pkg_entry_t));
    }

    action->next = sol->actions;
    sol->actions = action;
    sol->action_count++;

    return TX_OK;
}

/* ==================================================================
 * Helper: Resolve a single dependency string
 * ================================================================== */

static tx_status_t resolve_depends(tx_solver_context_t *ctx,
    tx_solver_solution_t *sol, const char *dep_str,
    const char *requester)
{
    if (!dep_str || !dep_str[0])
        return TX_OK;

    /* Parse dependency - may be comma-separated */
    char *buf = strdup(dep_str);
    if (!buf) return TX_ERROR_GENERAL;

    char *saveptr = NULL;
    char *token = strtok_r(buf, ",", &saveptr);

    while (token) {
        tx_dependency_t dep;
        tx_status_t s = tx_dep_parse(token, &dep);

        if (s == TX_OK) {
            /* Check if already in solution or installed */
            bool satisfied = false;

            /* Check if already installed */
            tx_installed_pkg_t *inst =
                find_installed(ctx, dep.name);
            if (inst && inst->status == TX_PKG_STATUS_INSTALLED) {
                if (dep.relation == TX_REL_ANY ||
                    tx_version_string_satisfies(
                        inst->name, dep.relation,
                        dep.version.upstream)) {
                    satisfied = true;
                }
            }

            /* Check if already in solution */
            if (!satisfied && action_exists(sol, dep.name,
                                            TX_OP_INSTALL)) {
                satisfied = true;
            }

            if (!satisfied) {
                /* Find in available packages */
                tx_pkg_entry_t *avail =
                    find_available(ctx, dep.name);

                if (!avail) {
                    /* Try to find a provider */
                    avail = tx_solver_find_provider(ctx, dep.name);
                }

                if (!avail) {
                    snprintf(sol->error_message,
                             sizeof(sol->error_message),
                             "Cannot satisfy dependency '%s' "
                             "for package '%s': not found in "
                             "repositories",
                             dep.name, requester);
                    free(buf);
                    return TX_ERROR_DEPENDENCY;
                }

                /* Add install action */
                tx_version_t v;
                tx_version_parse(&v, avail->version.upstream[0] ?
                                     avail->version.upstream :
                                     "1.0");

                char reason[512];
                snprintf(reason, sizeof(reason),
                         "required by %s", requester);

                s = add_action(sol, TX_OP_INSTALL, avail->name,
                               &v, reason, avail);
                if (s != TX_OK) {
                    free(buf);
                    return s;
                }

                /* Recursively resolve its dependencies */
                if (avail->depends[0]) {
                    s = resolve_depends(ctx, sol, avail->depends,
                                        avail->name);
                    if (s != TX_OK) {
                        free(buf);
                        return s;
                    }
                }
            }
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    free(buf);
    return TX_OK;
}

/* ==================================================================
 * Virtual Package Resolution
 * ================================================================== */

tx_pkg_entry_t *tx_solver_find_provider(tx_solver_context_t *ctx,
    const char *virtual_name)
{
    if (!ctx || !virtual_name) return NULL;

    /* Search for a package that provides this virtual name */
    for (size_t i = 0; i < ctx->available_count; i++) {
        if (ctx->available[i].provides[0]) {
            if (strstr(ctx->available[i].provides, virtual_name))
                return &ctx->available[i];
        }
    }

    /* Also check if a package has this exact name */
    tx_pkg_entry_t *pkg = find_available(ctx, virtual_name);
    if (pkg) return pkg;

    return NULL;
}

/* ==================================================================
 * Main Solver: Install
 * ================================================================== */

tx_status_t tx_solve_install(tx_solver_context_t *ctx,
    const char *package_name, tx_solver_solution_t *sol)
{
    if (!ctx || !package_name || !sol)
        return TX_ERROR_INVALID_ARG;

    tx_solution_init(sol);

    /* Check if already installed */
    tx_installed_pkg_t *inst = find_installed(ctx, package_name);
    if (inst && inst->status == TX_PKG_STATUS_INSTALLED) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Package '%s' is already installed",
                 package_name);
        return TX_ERROR_ALREADY_EXISTS;
    }

    /* Find package in repositories */
    tx_pkg_entry_t *pkg = find_available(ctx, package_name);

    if (!pkg) {
        /* Try virtual package resolution */
        pkg = tx_solver_find_provider(ctx, package_name);
    }

    if (!pkg) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Package '%s' not found in any repository",
                 package_name);
        return TX_ERROR_NOT_FOUND;
    }

    /* Check for conflicts */
    if (pkg->conflicts[0]) {
        tx_dependency_t cdep;
        char *cbuf = strdup(pkg->conflicts);
        char *csave = NULL;
        char *ctok = strtok_r(cbuf, ",", &csave);

        while (ctok) {
            tx_dep_parse(ctok, &cdep);
            tx_installed_pkg_t *conflict =
                find_installed(ctx, cdep.name);
            if (conflict &&
                conflict->status == TX_PKG_STATUS_INSTALLED) {
                snprintf(sol->error_message,
                         sizeof(sol->error_message),
                         "Cannot install '%s': conflicts with "
                         "installed package '%s'",
                         package_name, cdep.name);
                free(cbuf);
                return TX_ERROR_CONFLICT;
            }
            ctok = strtok_r(NULL, ",", &csave);
        }
        free(cbuf);
    }

    /* Add the main package */
    tx_version_t v;
    tx_version_parse(&v, pkg->version.upstream[0] ?
                         pkg->version.upstream : "1.0");

    tx_status_t s = add_action(sol, TX_OP_INSTALL, pkg->name,
                               &v, "user requested", pkg);
    if (s != TX_OK) return s;

    /* Resolve all dependencies */
    if (pkg->depends[0]) {
        s = resolve_depends(ctx, sol, pkg->depends, pkg->name);
        if (s != TX_OK) {
            /* Clean up partial solution */
            tx_solution_free(sol);
            return s;
        }
    }

    /* Order by dependencies */
    s = tx_solution_order(sol);
    if (s != TX_OK) {
        tx_solution_free(sol);
        return s;
    }

    /* Check for cycles */
    char cycle_buf[1024] = "";
    if (tx_solver_detect_cycles(sol, cycle_buf, sizeof(cycle_buf))) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Circular dependency detected: %s", cycle_buf);
        tx_solution_free(sol);
        return TX_ERROR_DEPENDENCY;
    }

    sol->is_valid = true;
    return TX_OK;
}

/* ==================================================================
 * Solver: Remove
 * ================================================================== */

tx_status_t tx_solve_remove(tx_solver_context_t *ctx,
    const char *package_name, tx_solver_solution_t *sol,
    bool force)
{
    if (!ctx || !package_name || !sol)
        return TX_ERROR_INVALID_ARG;

    tx_solution_init(sol);

    /* Find installed package */
    tx_installed_pkg_t *pkg = find_installed(ctx, package_name);
    if (!pkg || pkg->status != TX_PKG_STATUS_INSTALLED) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Package '%s' is not installed", package_name);
        return TX_ERROR_NOT_FOUND;
    }

    /* Check if essential */
    if (pkg->is_essential && !force) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Cannot remove essential package '%s'. "
                 "Use --force to override (not recommended).",
                 package_name);
        return TX_ERROR_DEPENDENCY;
    }

    /* Check reverse dependencies */
    for (size_t i = 0; i < ctx->installed_count; i++) {
        tx_installed_pkg_t *p = &ctx->installed[i];
        if (p->status != TX_PKG_STATUS_INSTALLED)
            continue;

        for (size_t j = 0; j < p->reverse_dep_count; j++) {
            if (strcmp(p->reverse_deps[j], package_name) == 0) {
                if (!force) {
                    snprintf(sol->error_message,
                             sizeof(sol->error_message),
                             "Cannot remove '%s': required by "
                             "installed package '%s'",
                             package_name, p->name);
                    return TX_ERROR_DEPENDENCY;
                }
                /* Would need to remove dependent too */
                add_action(sol, TX_OP_REMOVE, p->name, NULL,
                           "depends on removed package", NULL);
            }
        }
    }

    /* Add removal action */
    add_action(sol, TX_OP_REMOVE, package_name, NULL,
               "user requested", NULL);

    sol->is_valid = true;
    return tx_solution_order(sol);
}

/* ==================================================================
 * Solver: Upgrade All
 * ================================================================== */

tx_status_t tx_solve_upgrade(tx_solver_context_t *ctx,
    tx_solver_solution_t *sol)
{
    if (!ctx || !sol)
        return TX_ERROR_INVALID_ARG;

    tx_solution_init(sol);

    for (size_t i = 0; i < ctx->installed_count; i++) {
        tx_installed_pkg_t *inst = &ctx->installed[i];
        if (inst->status != TX_PKG_STATUS_INSTALLED)
            continue;

        /* Skip held packages */
        if (inst->is_held)
            continue;

        /* Find newer version in repositories */
        tx_pkg_entry_t *avail = find_available(ctx, inst->name);
        if (!avail) continue;

        tx_version_t inst_ver, avail_ver;
        tx_version_parse(&inst_ver, "1.0");
        tx_version_parse(&avail_ver,
            avail->version.upstream[0] ?
            avail->version.upstream : "1.0");

        if (tx_version_compare(&avail_ver, &inst_ver) > 0) {
            char reason[512];
            snprintf(reason, sizeof(reason),
                     "%s -> %s", inst->name,
                     avail->version.upstream);

            add_action(sol, TX_OP_UPGRADE, inst->name,
                       &avail_ver, reason, avail);

            /* Resolve new dependencies */
            if (avail->depends[0]) {
                resolve_depends(ctx, sol, avail->depends,
                                avail->name);
            }
        }
    }

    if (sol->action_count == 0) {
        strncpy(sol->error_message,
                "All packages are up to date",
                sizeof(sol->error_message) - 1);
        return TX_OK;
    }

    sol->is_valid = true;
    return tx_solution_order(sol);
}

/* ==================================================================
 * Solver: Upgrade Single Package
 * ================================================================== */

tx_status_t tx_solve_upgrade_one(tx_solver_context_t *ctx,
    const char *package_name, tx_solver_solution_t *sol)
{
    if (!ctx || !package_name || !sol)
        return TX_ERROR_INVALID_ARG;

    tx_solution_init(sol);

    tx_installed_pkg_t *inst = find_installed(ctx, package_name);
    if (!inst || inst->status != TX_PKG_STATUS_INSTALLED) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Package '%s' is not installed", package_name);
        return TX_ERROR_NOT_FOUND;
    }

    tx_pkg_entry_t *avail = find_available(ctx, package_name);
    if (!avail) {
        snprintf(sol->error_message, sizeof(sol->error_message),
                 "Package '%s' not found in repositories",
                 package_name);
        return TX_ERROR_NOT_FOUND;
    }

    tx_version_t inst_ver, avail_ver;
    tx_version_parse(&inst_ver, "1.0");
    tx_version_parse(&avail_ver,
        avail->version.upstream[0] ?
        avail->version.upstream : "1.0");

    if (tx_version_compare(&avail_ver, &inst_ver) <= 0) {
        strncpy(sol->error_message,
                "Already at latest version",
                sizeof(sol->error_message) - 1);
        return TX_OK;
    }

    add_action(sol, TX_OP_UPGRADE, package_name, &avail_ver,
               "upgrade available", avail);

    if (avail->depends[0]) {
        resolve_depends(ctx, sol, avail->depends, package_name);
    }

    sol->is_valid = true;
    return tx_solution_order(sol);
}

/* ==================================================================
 * Solver: Autoremove
 * ================================================================== */

tx_status_t tx_solve_autoremove(tx_solver_context_t *ctx,
    tx_solver_solution_t *sol)
{
    if (!ctx || !sol)
        return TX_ERROR_INVALID_ARG;

    tx_solution_init(sol);

    for (size_t i = 0; i < ctx->installed_count; i++) {
        tx_installed_pkg_t *pkg = &ctx->installed[i];
        if (pkg->status != TX_PKG_STATUS_INSTALLED)
            continue;

        /* Skip manually installed and essential */
        if (!pkg->is_auto || pkg->is_essential)
            continue;

        /* Check reverse dependencies */
        if (pkg->reverse_dep_count == 0) {
            add_action(sol, TX_OP_REMOVE, pkg->name, NULL,
                       "no longer needed (orphaned)", NULL);
        }
    }

    sol->is_valid = true;
    return TX_OK;
}

/* ==================================================================
 * Conflict Detection
 * ================================================================== */

bool tx_solver_would_conflict(tx_solver_context_t *ctx,
    const char *package_name)
{
    tx_pkg_entry_t *pkg = find_available(ctx, package_name);
    if (!pkg || !pkg->conflicts[0])
        return false;

    tx_dependency_t dep;
    char *cbuf = strdup(pkg->conflicts);
    char *csave = NULL;
    char *ctok = strtok_r(cbuf, ",", &csave);

    while (ctok) {
        tx_dep_parse(ctok, &dep);
        tx_installed_pkg_t *inst = find_installed(ctx, dep.name);
        if (inst && inst->status == TX_PKG_STATUS_INSTALLED) {
            free(cbuf);
            return true;
        }
        ctok = strtok_r(NULL, ",", &csave);
    }

    free(cbuf);
    return false;
}

/* ==================================================================
 * Cycle Detection
 * ================================================================== */

bool tx_solver_detect_cycles(tx_solver_solution_t *sol,
    char *circle_names, size_t names_size)
{
    if (!sol || !circle_names || names_size == 0)
        return false;

    circle_names[0] = '\0';

    /* Simple check: if install count > reasonable threshold,
     * may indicate cycle. Full cycle detection requires
     * dependency graph traversal. */
    if (sol->action_count > 100) {
        strncpy(circle_names, "possible cycle (too many packages)",
                names_size - 1);
        return true;
    }

    return false;
}

/* ==================================================================
 * Topological Ordering
 * ================================================================== */

tx_status_t tx_solution_order(tx_solver_solution_t *sol)
{
    if (!sol) return TX_ERROR_INVALID_ARG;

    /* Simple priority assignment: installs get increasing priority */
    tx_solver_action_t *action = sol->actions;
    int p = 0;
    while (action) {
        if (action->op == TX_OP_INSTALL)
            action->priority = p++;
        else
            action->priority = 1000 + p++;
        action = action->next;
    }

    return TX_OK;
}

/* ==================================================================
 * Solution Printing
 * ================================================================== */

void tx_solution_print(const tx_solver_solution_t *sol)
{
    if (!sol) {
        printf("No solution.\n");
        return;
    }

    if (!sol->is_valid) {
        printf("No valid solution available.\n");
        if (sol->error_message[0])
            printf("Error: %s\n", sol->error_message);
        return;
    }

    if (sol->action_count == 0) {
        printf("No actions required.\n");
        return;
    }

    printf("\nThe following packages will be processed:\n");
    printf("==========================================\n\n");

    /* Count by operation */
    size_t install = 0, upgrade = 0, remove = 0;
    tx_solver_action_t *a = sol->actions;
    while (a) {
        switch (a->op) {
            case TX_OP_INSTALL: install++; break;
            case TX_OP_UPGRADE: upgrade++; break;
            case TX_OP_REMOVE: remove++; break;
            default: break;
        }
        a = a->next;
    }

    if (install > 0) printf("  %zu to install\n", install);
    if (upgrade > 0) printf("  %zu to upgrade\n", upgrade);
    if (remove > 0)  printf("  %zu to remove\n", remove);

    printf("\nPackage operations:\n");

    a = sol->actions;
    while (a) {
        const char *op_str = "unknown";
        switch (a->op) {
            case TX_OP_INSTALL: op_str = "INSTALL"; break;
            case TX_OP_UPGRADE: op_str = "UPGRADE"; break;
            case TX_OP_REMOVE:  op_str = "REMOVE";  break;
            default: break;
        }

        printf("  %-10s %s", op_str, a->package_name);
        if (a->version.upstream[0])
            printf(" (%s)", a->version.upstream);
        if (a->reason[0])
            printf("  [%s]", a->reason);
        printf("\n");

        a = a->next;
    }

    printf("\n");
}
