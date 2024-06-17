#if 0
${CC:-cc} -Wall -pedantic -ggdb -O2 -Iinclude -Lbuild/host -o ${0%%.c} $0 -l:libcmt.a
exit $?
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sched.h>

#include "../tests/data.h"
#include "libcmt/sock.h"
#include "libcmt/util.h"
#include "libcmt/buf.h"
#include "libcmt/io.h"

#define DBG(X) debug(X, #X, __FILE__, __LINE__)
static int debug(int rc, const char *expr, const char *file, int line) {
    if (rc >= 0) {
        return 0;
    }

    if (cmt_util_debug_enabled()) {
        (void) fprintf(stdout, "%s:%d Error %s on `%s'\n", file, line, expr, strerror(-rc));
    }
    return rc;
}

typedef struct {
    cmt_buf_t buf[1];
    char path[64];
    int fd;
} cmt_file_t;

// should we handle multiple connections?
typedef struct cmt_io_host {
    cmt_conn_t sock[1];
    cmt_file_t tx[1], rx[1];
} cmt_io_host_t;

int cmt_file_init(cmt_file_t *me, size_t size, int prot)
{
    strncpy(me->path, "/tmp/tmp.XXXXXX", sizeof(me->path));
    me->fd = mkstemp(me->path);
    if (ftruncate(me->fd, size) < 0)
        return -errno;
    me->buf->begin = mmap(NULL, size, prot, MAP_SHARED, me->fd, 0);
    me->buf->end = me->buf->begin + size;
    return 0;
}

void cmt_file_fini(cmt_file_t *me)
{
    munmap(me->buf->begin, cmt_buf_length(me->buf));
    close(me->fd);
    unlink(me->path);
}

int cmt_io_host_init(cmt_io_host_t *me)
{
    size_t size = 2U << 20; // 2MB
    DBG(cmt_file_init(me->tx, size, PROT_READ | PROT_WRITE));
    DBG(cmt_file_init(me->rx, size, PROT_READ | PROT_WRITE));
    DBG(cmt_sock_init(me->sock, cmt_sock_server_get_path(), cmt_sock_client_get_path()));
    return 0;
}

void cmt_io_host_fini(cmt_io_host_t *me)
{
    unlink(me->tx->path);
    unlink(me->rx->path);
    cmt_sock_fini(me->sock);
}

static uint64_t pack(struct cmt_io_yield *rr) {
    // clang-format off
    return ((uint64_t) rr->dev    << 56)
    |      ((uint64_t) rr->cmd    << 56 >>  8)
    |      ((uint64_t) rr->reason << 48 >> 16)
    |      ((uint64_t) rr->data   << 32 >> 32);
    // clang-format on
}

static struct cmt_io_yield unpack(uint64_t x) {
    // clang-format off
    struct cmt_io_yield out = {
        x       >> 56,
        x <<  8 >> 56,
        x << 16 >> 48,
        x << 32 >> 32,
    };
    // clang-format on
    return out;
}

// wait for the client to signal it want the fds
int cmt_io_host_setup_client(cmt_io_host_t *me)
{
    int rc = 0;

    uint64_t req;
    rc = DBG(cmt_sock_recv(me->sock, sizeof(req), &req));
    if (rc < 0) {
        cmt_sock_fini(me->sock);
        return rc;
    }

    int fds[] = {me->rx->fd, me->tx->fd};
    rc = DBG(cmt_sock_send_fds(me->sock, 2, fds));
    if (rc < 0) {
        cmt_sock_fini(me->sock);
        return rc;
    }
    return 0;
}

void cmt_io_yield_print(cmt_io_yield_t *me)
{
    (void) fprintf(stdout,
        "tohost {\n"
        "\t.dev = %d,\n"
        "\t.cmd = %d,\n"
        "\t.reason = %d,\n"
        "\t.data = %u,\n"
        "};\n",
        me->dev, me->cmd, me->reason, me->data);
}

int main(int argc, char *const argv[]) {
    (void)argc;
    cmt_io_host_t io_host[1];
    fprintf(stderr, "%s: start ...\n", argv[0]);
    cmt_io_host_init(io_host);
    fprintf(stderr, "ready ...\n");
    cmt_io_host_setup_client(io_host);
    fprintf(stderr, "connected\n");

    while (true) {
        uint64_t req = 0xa0a0a0a0a0a0a0a0ULL;

        DBG(cmt_sock_recv(io_host->sock, sizeof(req), &req));
        cmt_io_yield_t y = unpack(req);
        cmt_io_yield_print(&y);

        // do things here
        memcpy(io_host->tx->buf->begin, valid_advance_0, sizeof(valid_advance_0));
        y.reason = 0; // advance
        y.data = sizeof(valid_advance_0);

        uint64_t rep = pack(&y);
        cmt_io_yield_print(&y);

        DBG(cmt_sock_send(io_host->sock, sizeof(rep), &rep));

        break; // we are testing, send only once
    }

    cmt_io_host_fini(io_host);
    return 0;
}
