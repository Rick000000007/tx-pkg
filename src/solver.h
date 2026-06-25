/*
 * TX Package Manager v1.0 - Dependency Solver
 *
 * Implements a SAT-inspired dependency solver that:
 *   - Resolves dependency graphs
 *   - Detects circular dependencies
 *   - Handles virtual packages
 *   - Enforces conflicts and breaks
 *   - Suggests optimal installation order
 *   - Provides clear error messages for unsolvable situations
 */

#ifndef TX_SOLVER_H
#define TX_SOLVER_H

#include "package.h"

/* ==================================================================
 * Solver Action
 *
 * A single resolved action in the solution plan.
 * ================================================================== */

typedef struct tx_solver_action {
    tx_op_type_t op;
    char package_name[TX_MAX_PACKAGE_NAME];
    tx_version_t version;
    char reason[512];         /* Why this action is needed */
    int priority;             /* For ordering (higher = earlier) */

    /* For install/upgrade: the source package entry */
    tx_pkg_entry_t *source;

    struct tx_solver_action *next;
} tx_solver_action_t;

/* ==================================================================
 * Solver Solution
 *
 * A complete plan of actions to satisfy a request.
 * ================================================================== */

typedef struct tx_solver_solution {
    tx_solver_action_t *actions;
    size_t action_count;

    /* Unsatisfied recommendations/suggestions (non-fatal) */
    char **warnings;
    size_t warning_count;

    /* Solver status */
    bool is_valid;
    char error_message[1024];
} tx_solver_solution_t;

/* ==================================================================
 * Solver Context
 *
 * Provides the solver with repository data and installed state.
 * ================================================================== */

typedef struct tx_solver_context {
    /* Available packages from repositories */
    tx_pkg_entry_t *available;
    size_t available_count;

    /* Currently installed packages */
    tx_installed_pkg_t *installed;
    size_t installed_count;

    /* Package database for queries */
    void *db_handle;
} tx_solver_context_t;

/* ==================================================================
 * Solver API
 * ================================================================== */

/**
 * Initialize a solver solution.
 */
void tx_solution_init(tx_solver_solution_t *sol);

/**
 * Free a solver solution and all associated data.
 */
void tx_solution_free(tx_solver_solution_t *sol);

/**
 * Solve for installing a package and all its dependencies.
 *
 * Returns TX_OK with a valid solution, or an error code if
 * the request cannot be satisfied.
 */
tx_status_t tx_solve_install(tx_solver_context_t *ctx,
    const char *package_name, tx_solver_solution_t *sol);

/**
 * Solve for removing a package, respecting reverse dependencies.
 */
tx_status_t tx_solve_remove(tx_solver_context_t *ctx,
    const char *package_name, tx_solver_solution_t *sol,
    bool force);

/**
 * Solve for upgrading all installed packages.
 */
tx_status_t tx_solve_upgrade(tx_solver_context_t *ctx,
    tx_solver_solution_t *sol);

/**
 * Solve for upgrading a specific package.
 */
tx_status_t tx_solve_upgrade_one(tx_solver_context_t *ctx,
    const char *package_name, tx_solver_solution_t *sol);

/**
 * Find packages that can be auto-removed (no reverse deps).
 */
tx_status_t tx_solve_autoremove(tx_solver_context_t *ctx,
    tx_solver_solution_t *sol);

/**
 * Check if installing a package would cause conflicts.
 */
bool tx_solver_would_conflict(tx_solver_context_t *ctx,
    const char *package_name);

/**
 * Find which package provides a virtual package name.
 */
tx_pkg_entry_t *tx_solver_find_provider(
    tx_solver_context_t *ctx, const char *virtual_name);

/**
 * Detect circular dependencies in a solution.
 * Returns true if circular deps found, fills circle_names.
 */
bool tx_solver_detect_cycles(tx_solver_solution_t *sol,
    char *circle_names, size_t names_size);

/**
 * Topological sort of actions by dependency order.
 * Ensures dependencies are installed before dependents.
 */
tx_status_t tx_solution_order(tx_solver_solution_t *sol);

/**
 * Print a solution plan (for dry-run / verbose output).
 */
void tx_solution_print(const tx_solver_solution_t *sol);

#endif /* TX_SOLVER_H */
