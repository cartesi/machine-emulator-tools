#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "libcmt/io.h"

typedef struct {
    char *exe;
    uint16_t key;
} hook_entry_t;

static void *array_push(void **data, size_t m, size_t *n, size_t *max) {
    if ((*n == *max) || (*data == NULL)) {
        *max = *data == NULL ? 32 : *max * 2;
        *data = realloc(*data, *max * m);
        if (*data == NULL) {
            perror("Failed to resize array with:");
            exit(1);
        }
    }
    return &((uint8_t *) *data)[(*n)++ * m];
}

static int __hook_entry_cmp(const hook_entry_t *a, const hook_entry_t *b) {
    return (int) a->key - (int) b->key;
}
static int hook_entry_cmp(const void *a, const void *b) {
    return __hook_entry_cmp(a, b);
}

static int write_whole_buffer(int wr, uint32_t n, cmt_buf_t *tx) {
    ssize_t offset = 0;
    ssize_t left = n;

    while (left) {
        ssize_t ret = write(wr, tx->begin + offset, left);
        if (ret >= 0) {
            offset += ret;
            left -= ret;
        } else
            switch (errno) {
                case EINTR:
                case EAGAIN:
                    continue;
                default:
                    return -errno;
            }
    }
    return 0;
}

static int read_whole_buffer(int rd, uint32_t *n, cmt_buf_t *rx) {
    ssize_t offset = 0;
    ssize_t max = cmt_buf_length(rx);

    while (max) {
        ssize_t ret = read(rd, rx->begin + offset, max);
        if (ret == 0)
            break;
        if (ret > 0) {
            offset += ret;
            max -= ret;
        } else
            switch (errno) {
                case EINTR:
                case EAGAIN:
                    continue;
                default:
                    return -errno;
            }
    }
    *n = offset;
    return 0;
}

static int parent(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr, pid_t child, int pipes[2][2]) {
    int rc;
    int rd = pipes[1][0];
    if (close(pipes[1][1]) < 0)
        perror("Failed to close pipes[1][1]: ");

    int wr = pipes[0][1];
    if (close(pipes[0][0]) < 0)
        perror("Failed to close pipes[0][0]: ");

    if ((rc = write_whole_buffer(wr, rr->data, me->tx)))
        fprintf(stderr, "Failed to write whole buffer: %s\n", strerror(-rc));
    if (close(wr) < 0)
        perror("Failed to close wr: ");

    if ((rc = read_whole_buffer(rd, &rr->data, me->rx)))
        fprintf(stderr, "Failed to write whole buffer: %s\n", strerror(-rc));
    if (close(rd) < 0)
        perror("Failed to close rd: ");

    if ((rc = waitpid(child, NULL, 0)) < 0)
        return -errno;
    return 0;
}

static int child(int pipes[2][2], uint16_t key, char *exe) {
    int rd = pipes[0][0];
    if (dup2(rd, STDIN_FILENO) < 0)
        perror("dup2 failed: ");
    if (close(pipes[0][1]) < 0)
        perror("Failed close(pipes[0][1]): ");

    int wr = pipes[1][1];
    if (dup2(wr, STDOUT_FILENO) < 0)
        perror("dup2 failed: ");
    if (close(pipes[1][0]) < 0)
        perror("Failed close(pipes[1][0]): ");

    char skey[16];
    snprintf(skey, sizeof skey, "%hu", key);
    char *const argv[] = {exe, skey, NULL};
    char *const envp[] = {NULL};
    if (execve(argv[0], argv, envp) < 0)
        perror("Failed: ");
    return -errno;
}

int cmt_io_driver_mock_hook_load(cmt_io_driver_mock_t *me, char *env) {
    cmt_buf_t x;
    cmt_buf_t xs;
    hook_entry_t *hooks = NULL;
    size_t n = 0, max = 0;

    if (!env)
        goto end;
    size_t envlen = strlen(env);

    cmt_buf_init(&xs, envlen, env);
    while (cmt_buf_split_by_comma(&x, &xs)) {
        uint16_t key = 0;
        char exe[128] = "";

        if (sscanf((char *) x.begin, "%hu:%127[^,]", &key, exe) != 2) {
            fprintf(stderr, "Failed to parse: `%.*s', skipped.\n", (int) cmt_buf_length(&x), x.begin);
            continue;
        }
        fprintf(stderr, "Adding hook: [%d] = \"%s\"\n", key, exe);
        hook_entry_t *hook = array_push((void **) &hooks, sizeof hooks[0], &n, &max);
        hook->key = key;
        hook->exe = strdup(exe);
    }
    qsort(hooks, n, sizeof hooks[0], hook_entry_cmp);
end:
    fprintf(stderr, "Added %lu hook%s\n", n, n > 1 ? "s" : "");
    me->hooks = hooks;
    me->hooks_num = n;
    return 0;
}

int cmt_io_driver_mock_hook_dispatch(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    hook_entry_t key[1] = {{NULL, rr->reason}};
    hook_entry_t *hook = bsearch(key, me->hooks, me->hooks_num, sizeof key, hook_entry_cmp);
    if (!hook)
        return -ENOKEY;

    int pipes[2][2];
    if (pipe(pipes[0]) < 0)
        perror("Failed to create pipes[0]");
    if (pipe(pipes[1]) < 0)
        perror("Failed to create pipes[1]");

    pid_t pid = fork();
    return pid ? parent(me, rr, pid, pipes) : child(pipes, hook->key, hook->exe);
}
