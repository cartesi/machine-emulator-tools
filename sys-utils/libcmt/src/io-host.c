#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include "libcmt/io.h"
#include "libcmt/util.h"
#include "libcmt/sock.h"

static int mmapfd(int fd, int prot, uint8_t **begin, uint8_t **end)
{
    struct stat stat;
    if (fstat(fd, &stat)) {
        return -errno;
    }
    size_t size = stat.st_size;

    *begin = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    *end = *begin + size;
    if (*begin == MAP_FAILED) {
        return -errno;
    }
    return 0;
}

int cmt_io_init(cmt_io_driver_t *_me) {
    int rc = 0;

    if (!_me) {
        return -EINVAL;
    }
    cmt_io_driver_host_t *me = &_me->host;

    rc = cmt_sock_init(&me->sock, cmt_sock_client_get_path(), cmt_sock_server_get_path());
    if (rc) {
        return rc;
    }

    // signal the server we want the fds
    rc = cmt_sock_send(&me->sock, 0, NULL);
    if (rc) {
        cmt_sock_fini(&me->sock);
        return rc;
    }

    rc = cmt_sock_recv_fds(&me->sock, sizeof(me->fds) / sizeof(int), me->fds);
    if (rc) {
        cmt_sock_fini(&me->sock);
        return rc;
    }

    rc = mmapfd(me->fds[0], PROT_READ | PROT_WRITE, &me->tx->begin, &me->tx->end);
    if (rc) {
        cmt_sock_fini(&me->sock);
        return rc;
    }
    rc = mmapfd(me->fds[1], PROT_READ, &me->rx->begin, &me->rx->end);
    if (rc) {
        munmap(me->tx->begin, cmt_buf_length(me->tx));
        cmt_sock_fini(&me->sock);
        return rc;
    }

    return 0;
}

void cmt_io_fini(cmt_io_driver_t *_me) {
    if (!_me) {
        return;
    }
    cmt_io_driver_host_t *me = &_me->host;

    munmap(me->tx->begin, cmt_buf_length(me->tx));
    munmap(me->rx->begin, cmt_buf_length(me->rx));

    close(me->fds[0]);
    close(me->fds[1]);

    cmt_sock_fini(&me->sock);
}

cmt_buf_t cmt_io_get_tx(cmt_io_driver_t *me) {
    const cmt_buf_t empty = {NULL, NULL};
    if (!me) {
        return empty;
    }
    return *me->host.tx;
}

cmt_buf_t cmt_io_get_rx(cmt_io_driver_t *me) {
    const cmt_buf_t empty = {NULL, NULL};
    if (!me) {
        return empty;
    }
    return *me->host.rx;
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

int cmt_io_yield(cmt_io_driver_t *_me, cmt_io_yield_t *rr)
{
    int rc = 0;
    if (!_me) {
        return -EINVAL;
    }
    if (!rr) {
        return -EINVAL;
    }
    cmt_io_driver_host_t *me = &_me->host;

    bool debug = cmt_util_debug_enabled();
    if (debug) {
        (void) fprintf(stderr,
            "tohost {\n"
            "\t.dev = %d,\n"
            "\t.cmd = %d,\n"
            "\t.reason = %d,\n"
            "\t.data = %d,\n"
            "};\n",
            rr->dev, rr->cmd, rr->reason, rr->data);
    }

    uint64_t req = pack(rr);
    rc = cmt_sock_send(&me->sock, sizeof(req), &req);
    if (rc < 0) return rc;

    uint64_t rep;
    rc = cmt_sock_recv(&me->sock, sizeof(rep), &rep);
    if (rc < 0) return rc;
    *rr = unpack(rep);

    if (debug) {
        (void) fprintf(stderr,
            "fromhost {\n"
            "\t.dev = %d,\n"
            "\t.cmd = %d,\n"
            "\t.reason = %d,\n"
            "\t.data = %d,\n"
            "};\n",
            rr->dev, rr->cmd, rr->reason, rr->data);
    }
    return 0;
}
