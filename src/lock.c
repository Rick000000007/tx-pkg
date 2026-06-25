/*
 * TX Package Manager v1.0 - File Locking Implementation
 */

#include "lock.h"
#include "error.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <unistd.h>

tx_status_t tx_lock_init(tx_lock_t *lock, const char *lock_file)
{
    if (!lock || !lock_file)
        return TX_ERROR_INVALID_ARG;

    memset(lock, 0, sizeof(*lock));
    strncpy(lock->lock_file, lock_file, TX_MAX_PATH - 1);
    lock->fd = -1;
    lock->is_locked = false;
    lock->owner_pid = 0;

    /* Ensure directory exists */
    char dir[TX_MAX_PATH];
    strncpy(dir, lock_file, TX_MAX_PATH - 1);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        char cmd[TX_MAX_PATH + 32];
        snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dir);
        system(cmd);
    }

    return TX_OK;
}

tx_status_t tx_lock_acquire(tx_lock_t *lock)
{
    if (!lock) return TX_ERROR_INVALID_ARG;

    if (lock->is_locked)
        return TX_OK;

    lock->fd = open(lock->lock_file, O_RDWR | O_CREAT, 0666);
    if (lock->fd < 0)
        return TX_ERROR_IO;

    /* Try non-blocking lock first */
    int retries = 0;
    while (flock(lock->fd, LOCK_EX | LOCK_NB) != 0) {
        if (errno != EWOULDBLOCK) {
            close(lock->fd);
            lock->fd = -1;
            return TX_ERROR_LOCK;
        }

        /* Check for stale lock */
        if (lock->owner_pid > 0 && kill(lock->owner_pid, 0) != 0) {
            /* Owner is dead - break the lock */
            tx_lock_break(lock);
            lock->fd = open(lock->lock_file, O_RDWR | O_CREAT, 0666);
            if (lock->fd < 0)
                return TX_ERROR_IO;
            continue;
        }

        /* Wait and retry */
        usleep(TX_LOCK_RETRY_INTERVAL_MS * 1000);
        retries++;

        if (retries >= TX_LOCK_MAX_RETRIES) {
            close(lock->fd);
            lock->fd = -1;
            TX_ERROR_SET(TX_ERROR_TIMEOUT,
                "Could not acquire package lock",
                lock->lock_file,
                "Another package operation is in progress",
                "Wait for the other operation to complete, or "
                "run 'pkg doctor --fix' to remove stale locks.");
            return TX_ERROR_TIMEOUT;
        }
    }

    /* Write PID to lock file */
    lock->owner_pid = getpid();
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", lock->owner_pid);
    ftruncate(lock->fd, 0);
    write(lock->fd, pid_str, strlen(pid_str));

    lock->is_locked = true;
    return TX_OK;
}

tx_status_t tx_lock_try_acquire(tx_lock_t *lock)
{
    if (!lock) return TX_ERROR_INVALID_ARG;

    if (lock->is_locked)
        return TX_OK;

    lock->fd = open(lock->lock_file, O_RDWR | O_CREAT, 0666);
    if (lock->fd < 0)
        return TX_ERROR_IO;

    if (flock(lock->fd, LOCK_EX | LOCK_NB) != 0) {
        close(lock->fd);
        lock->fd = -1;
        return TX_ERROR_LOCK;
    }

    lock->owner_pid = getpid();
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", lock->owner_pid);
    ftruncate(lock->fd, 0);
    write(lock->fd, pid_str, strlen(pid_str));

    lock->is_locked = true;
    return TX_OK;
}

tx_status_t tx_lock_release(tx_lock_t *lock)
{
    if (!lock) return TX_ERROR_INVALID_ARG;

    if (!lock->is_locked)
        return TX_OK;

    flock(lock->fd, LOCK_UN);
    close(lock->fd);

    /* Remove lock file */
    unlink(lock->lock_file);

    lock->fd = -1;
    lock->is_locked = false;
    lock->owner_pid = 0;

    return TX_OK;
}

bool tx_lock_is_held(const char *lock_file)
{
    if (!lock_file) return false;

    int fd = open(lock_file, O_RDONLY);
    if (fd < 0) return false;

    /* Try to acquire non-blocking lock */
    int can_lock = (flock(fd, LOCK_EX | LOCK_NB) == 0);

    if (can_lock) {
        /* We got the lock, release it */
        flock(fd, LOCK_UN);
    }

    close(fd);
    return !can_lock;
}

tx_status_t tx_lock_break(tx_lock_t *lock)
{
    if (!lock) return TX_ERROR_INVALID_ARG;

    /* Read PID from lock file */
    int fd = open(lock->lock_file, O_RDONLY);
    if (fd < 0) return TX_ERROR_IO;

    char buf[32] = "";
    read(fd, buf, sizeof(buf) - 1);
    close(fd);

    pid_t pid = atoi(buf);

    /* Check if process is still alive */
    if (pid > 0 && kill(pid, 0) == 0) {
        /* Process is alive - don't break */
        TX_ERROR_SET(TX_ERROR_LOCK,
            "Lock is held by a running process",
            lock->lock_file, NULL,
            "Wait for the process to complete.");
        return TX_ERROR_LOCK;
    }

    /* Remove stale lock */
    unlink(lock->lock_file);
    lock->is_locked = false;
    lock->owner_pid = 0;

    return TX_OK;
}

void tx_lock_free(tx_lock_t *lock)
{
    if (!lock) return;
    if (lock->is_locked)
        tx_lock_release(lock);
    memset(lock, 0, sizeof(*lock));
}
