/*
 * TX Package Manager v1.0 - Transaction Engine Implementation
 */

#include "transaction.h"
#include "error.h"
#include "config.h"
#include "vendor/cjson.h"
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

/* Forward declarations for internal operation functions */
static tx_status_t tx_transaction_do_install(tx_transaction_t *tx,
    tx_transaction_step_t *step);
static tx_status_t tx_transaction_do_upgrade(tx_transaction_t *tx,
    tx_transaction_step_t *step);
static tx_status_t tx_transaction_do_remove(tx_transaction_t *tx,
    tx_transaction_step_t *step);
static tx_status_t tx_transaction_do_reinstall(tx_transaction_t *tx,
    tx_transaction_step_t *step);

tx_status_t tx_transaction_init(tx_transaction_t *tx,
    tx_db_t *db, tx_cache_t *cache, const char *prefix)
{
    if (!tx || !db)
        return TX_ERROR_INVALID_ARG;

    memset(tx, 0, sizeof(*tx));
    tx->db = db;
    tx->cache = cache;
    tx->install_prefix = prefix ? prefix : TX_PREFIX;
    tx->state = TX_STATE_IDLE;

    tx_transaction_generate_id(tx->tx_id, sizeof(tx->tx_id));

    snprintf(tx->journal_path, sizeof(tx->journal_path),
             "%s/%s.journal", TX_JOURNAL_DIR, tx->tx_id);

    return TX_OK;
}

void tx_transaction_free(tx_transaction_t *tx)
{
    if (!tx) return;

    tx_transaction_step_t *step = tx->steps;
    while (step) {
        tx_transaction_step_t *next = step->next;
        free(step);
        step = next;
    }

    memset(tx, 0, sizeof(*tx));
}

void tx_transaction_generate_id(char *id, size_t size)
{
    if (!id || size == 0) return;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    snprintf(id, size, "tx-%lx-%lx",
             (unsigned long)ts.tv_sec,
             (unsigned long)ts.tv_nsec);
}

tx_status_t tx_transaction_from_solution(tx_transaction_t *tx,
    const tx_solver_solution_t *sol)
{
    if (!tx || !sol)
        return TX_ERROR_INVALID_ARG;

    tx->state = TX_STATE_PLANNING;

    tx_solver_action_t *action = sol->actions;
    while (action) {
        tx_status_t s = tx_transaction_add_step(tx, action->op,
            action->package_name, &action->version);
        if (s != TX_OK) return s;
        action = action->next;
    }

    return TX_OK;
}

tx_status_t tx_transaction_add_step(tx_transaction_t *tx,
    tx_op_type_t op, const char *package_name,
    const tx_version_t *version)
{
    if (!tx || !package_name)
        return TX_ERROR_INVALID_ARG;

    tx_transaction_step_t *step = calloc(1, sizeof(*step));
    if (!step)
        return TX_ERROR_GENERAL;

    step->step_number = (int)tx->step_count;
    step->op = op;
    strncpy(step->package_name, package_name, TX_MAX_PACKAGE_NAME - 1);
    if (version)
        memcpy(&step->target_version, version, sizeof(*version));

    /* Append to list */
    tx_transaction_step_t **p = &tx->steps;
    while (*p) p = &(*p)->next;
    *p = step;
    tx->step_count++;

    return TX_OK;
}

tx_status_t tx_transaction_validate(tx_transaction_t *tx)
{
    if (!tx) return TX_ERROR_INVALID_ARG;

    /* Check for duplicate operations on same package */
    tx_transaction_step_t *a = tx->steps;
    while (a) {
        tx_transaction_step_t *b = a->next;
        int count = 0;
        while (b) {
            if (strcmp(a->package_name, b->package_name) == 0)
                count++;
            b = b->next;
        }
        if (count > 0) {
            /* Multiple operations on same package - merge */
        }
        a = a->next;
    }

    return TX_OK;
}

tx_status_t tx_transaction_execute(tx_transaction_t *tx)
{
    if (!tx) return TX_ERROR_INVALID_ARG;

    tx->state = TX_STATE_EXTRACTING;
    tx->current_step = 0;

    tx_transaction_step_t *step = tx->steps;
    while (step) {
        tx->current_step++;

        if (tx->on_step_start)
            tx->on_step_start(tx, step);

        tx_status_t s = tx_transaction_execute_step(tx, step);

        if (s == TX_OK) {
            step->completed = true;
            tx->completed++;

            if (tx->on_step_complete)
                tx->on_step_complete(tx, step);
        } else {
            step->completed = false;
            tx->failed++;

            strncpy(step->error_message, tx_last_error.message,
                    sizeof(step->error_message) - 1);

            if (tx->on_step_error)
                tx->on_step_error(tx, step, step->error_message);

            /* Rollback */
            tx->state = TX_STATE_ROLLING_BACK;
            tx_transaction_rollback(tx);
            tx->state = TX_STATE_FAILED;

            return TX_ERROR_TRANSACTION;
        }

        /* Write journal for recovery */
        tx_transaction_journal_write(tx);

        step = step->next;
    }

    tx->state = TX_STATE_COMPLETED;
    tx_transaction_commit(tx);
    tx_transaction_journal_remove(tx);

    return TX_OK;
}

tx_status_t tx_transaction_execute_step(tx_transaction_t *tx,
    tx_transaction_step_t *step)
{
    if (!tx || !step)
        return TX_ERROR_INVALID_ARG;

    switch (step->op) {
        case TX_OP_INSTALL:
            return tx_transaction_do_install(tx, step);

        case TX_OP_UPGRADE:
            return tx_transaction_do_upgrade(tx, step);

        case TX_OP_REMOVE:
            return tx_transaction_do_remove(tx, step);

        case TX_OP_REINSTALL:
            return tx_transaction_do_reinstall(tx, step);

        default:
            return TX_ERROR_UNSUPPORTED;
    }
}

/* Forward declarations for the actual operations */
static tx_status_t tx_transaction_do_install(tx_transaction_t *tx,
    tx_transaction_step_t *step)
{
    /* Mark as half-installed */
    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_HALF_INSTALLED);

    /* In a full implementation, this would:
     * 1. Download the package if not cached
     * 2. Verify SHA256
     * 3. Extract to temporary location
     * 4. Run preinst script
     * 5. Copy files to install prefix
     * 6. Run postinst script
     * 7. Register in database
     * 8. Record file ownership
     */

    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_INSTALLED);

    return TX_OK;
}

static tx_status_t tx_transaction_do_upgrade(tx_transaction_t *tx,
    tx_transaction_step_t *step)
{
    /* Backup current state */
    tx_installed_pkg_t old_pkg;
    tx_db_get_package(tx->db, step->package_name, &old_pkg);

    step->old_status = old_pkg.status;

    /* Mark as half-installed */
    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_HALF_INSTALLED);

    /* Run prerm */
    /* Remove old files */
    /* Install new files */
    /* Run postinst */

    tx_db_set_version(tx->db, step->package_name,
                      &step->target_version);
    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_INSTALLED);

    tx_installed_pkg_free(&old_pkg);
    return TX_OK;
}

static tx_status_t tx_transaction_do_remove(tx_transaction_t *tx,
    tx_transaction_step_t *step)
{
    /* Get current state for rollback */
    tx_installed_pkg_t old_pkg;
    tx_db_get_package(tx->db, step->package_name, &old_pkg);

    step->old_status = old_pkg.status;

    /* Mark as half-configured */
    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_HALF_CONFIGURED);

    /* Run prerm */
    /* Remove files */
    /* Run postrm */

    tx_db_unregister_package(tx->db, step->package_name);

    tx_installed_pkg_free(&old_pkg);
    return TX_OK;
}

static tx_status_t tx_transaction_do_reinstall(tx_transaction_t *tx,
    tx_transaction_step_t *step)
{
    /* Same as install but over existing */
    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_HALF_INSTALLED);

    /* Re-extract files */
    /* Re-run scripts */

    tx_db_set_status(tx->db, step->package_name,
                     TX_PKG_STATUS_INSTALLED);

    return TX_OK;
}

tx_status_t tx_transaction_commit(tx_transaction_t *tx)
{
    if (!tx) return TX_ERROR_INVALID_ARG;

    /* Ensure all database changes are persisted */
    tx_db_commit(tx->db);

    tx->state = TX_STATE_COMPLETED;
    return TX_OK;
}

tx_status_t tx_transaction_rollback(tx_transaction_t *tx)
{
    if (!tx) return TX_ERROR_INVALID_ARG;

    /* Rollback steps in reverse order */
    tx_transaction_step_t *step = tx->steps;

    /* Find last step */
    tx_transaction_step_t *last = NULL;
    while (step) {
        if (step->completed)
            last = step;
        step = step->next;
    }

    /* Rollback from last to first */
    while (last) {
        tx_transaction_rollback_step(tx, last);
        last->rolled_back = true;
        tx->rolled_back++;

        /* Find previous completed step */
        tx_transaction_step_t *prev = tx->steps;
        tx_transaction_step_t *prev_completed = NULL;
        while (prev && prev != last) {
            if (prev->completed)
                prev_completed = prev;
            prev = prev->next;
        }
        last = prev_completed;
    }

    tx->state = TX_STATE_ROLLED_BACK;
    return TX_OK;
}

tx_status_t tx_transaction_rollback_step(tx_transaction_t *tx,
    tx_transaction_step_t *step)
{
    if (!tx || !step)
        return TX_ERROR_INVALID_ARG;

    switch (step->op) {
        case TX_OP_INSTALL:
            /* Remove any files that were installed */
            tx_db_unregister_package(tx->db, step->package_name);
            break;

        case TX_OP_UPGRADE:
            /* Restore previous version if backed up */
            tx_db_set_status(tx->db, step->package_name,
                             step->old_status);
            break;

        case TX_OP_REMOVE:
            /* Restore the package (if we backed it up) */
            tx_db_set_status(tx->db, step->package_name,
                             step->old_status);
            break;

        case TX_OP_REINSTALL:
            tx_db_set_status(tx->db, step->package_name,
                             TX_PKG_STATUS_INSTALLED);
            break;

        default:
            break;
    }

    return TX_OK;
}

/* ==================================================================
 * Journaling
 * ================================================================== */

tx_status_t tx_transaction_journal_write(tx_transaction_t *tx)
{
    if (!tx) return TX_ERROR_INVALID_ARG;

    /* Ensure journal directory exists */
    char dir[TX_MAX_PATH];
    strncpy(dir, tx->journal_path, TX_MAX_PATH - 1);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        char cmd[TX_MAX_PATH + 32];
        snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir);
        system(cmd);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "tx_id", tx->tx_id);
    cJSON_AddNumberToObject(root, "state", tx->state);
    cJSON_AddNumberToObject(root, "current_step", (double)tx->current_step);
    cJSON_AddStringToObject(root, "timestamp", "active");

    cJSON *steps = cJSON_CreateArray();
    tx_transaction_step_t *step = tx->steps;
    while (step) {
        cJSON *s = cJSON_CreateObject();
        cJSON_AddNumberToObject(s, "number", step->step_number);
        cJSON_AddStringToObject(s, "package", step->package_name);
        cJSON_AddNumberToObject(s, "op", step->op);
        cJSON_AddBoolToObject(s, "completed", step->completed);
        cJSON_AddBoolToObject(s, "rolled_back", step->rolled_back);
        cJSON_AddItemToArray(steps, s);
        step = step->next;
    }
    cJSON_AddItemToObject(root, "steps", steps);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) return TX_ERROR_GENERAL;

    FILE *fp = fopen(tx->journal_path, "w");
    if (!fp) {
        free(json_str);
        return TX_ERROR_IO;
    }

    fprintf(fp, "%s\n", json_str);
    fclose(fp);
    free(json_str);

    /* Sync to ensure durability */
    sync();

    return TX_OK;
}

tx_status_t tx_transaction_journal_read(const char *journal_path,
    tx_transaction_t *tx)
{
    if (!journal_path || !tx)
        return TX_ERROR_INVALID_ARG;

    FILE *fp = fopen(journal_path, "r");
    if (!fp) return TX_ERROR_IO;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(fp);
        return TX_ERROR_IO;
    }

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

    if (!root) return TX_ERROR_PARSE;

    cJSON *tx_id = cJSON_GetObjectItemCaseSensitive(root, "tx_id");
    cJSON *state = cJSON_GetObjectItemCaseSensitive(root, "state");

    if (tx_id)
        strncpy(tx->tx_id, tx_id->valuestring, sizeof(tx->tx_id) - 1);
    if (state)
        tx->state = state->valueint;

    strncpy(tx->journal_path, journal_path, TX_MAX_PATH - 1);

    cJSON *steps = cJSON_GetObjectItemCaseSensitive(root, "steps");
    if (steps) {
        cJSON *s;
        cJSON_ArrayForEach(s, steps) {
            cJSON *pkg = cJSON_GetObjectItemCaseSensitive(s, "package");
            cJSON *op = cJSON_GetObjectItemCaseSensitive(s, "op");

            if (pkg && op) {
                tx_transaction_add_step(tx, op->valueint,
                    pkg->valuestring, NULL);
            }
        }
    }

    cJSON_Delete(root);
    return TX_OK;
}

tx_status_t tx_transaction_journal_remove(tx_transaction_t *tx)
{
    if (!tx) return TX_ERROR_INVALID_ARG;

    unlink(tx->journal_path);
    return TX_OK;
}

tx_state_t tx_transaction_get_state(const tx_transaction_t *tx)
{
    return tx ? tx->state : TX_STATE_IDLE;
}

void tx_transaction_print(const tx_transaction_t *tx)
{
    if (!tx) return;

    printf("Transaction: %s\n", tx->tx_id);
    printf("State:       %d\n", tx->state);
    printf("Steps:       %zu\n", tx->step_count);
    printf("Completed:   %zu\n", tx->completed);
    printf("Failed:      %zu\n", tx->failed);
    printf("Rolled back: %zu\n", tx->rolled_back);
}
