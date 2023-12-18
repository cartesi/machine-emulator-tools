#include "libcmt/io.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int next(union cmt_io_driver *io, uint32_t *n) {
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_MANUAL,
        .reason = CMT_IO_REASON_RX_ACCEPTED,
        .data = *n,
    }};
    if (cmt_io_yield(io, req)) {
        fprintf(stderr, "%s:%d failed to yield\n", __FILE__, __LINE__);
        return -1;
    }
    *n = req->data;
    return req->reason;
}

int voucher(union cmt_io_driver *io, uint32_t n) {
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_AUTOMATIC,
        .reason = CMT_IO_REASON_TX_OUTPUT,
        .data = n,
    }};
    return cmt_io_yield(io, req);
}

int report(union cmt_io_driver *io, uint32_t n) {
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_AUTOMATIC,
        .reason = CMT_IO_REASON_TX_REPORT,
        .data = n,
    }};
    return cmt_io_yield(io, req);
}

int exception(union cmt_io_driver *io, uint32_t n) {
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_MANUAL,
        .reason = CMT_IO_REASON_TX_EXCEPTION,
        .data = n,
    }};
    return cmt_io_yield(io, req);
}

int main(void) {
    /* init ------------------------------------------------------------- */
    union cmt_io_driver io[1];
    int rc = cmt_io_init(io);
    if (rc) {
        fprintf(stderr, "%s:%d failed to init with: %s\n", __FILE__, __LINE__, strerror(-rc));
        return EXIT_FAILURE;
    }

    cmt_buf_t tx = cmt_io_get_tx(io);
    cmt_buf_t rx = cmt_io_get_rx(io);

    int type;
    uint32_t n = 0;
    while ((type = next(io, &n)) >= 0) {
        memcpy(tx.begin, rx.begin, n);
        voucher(io, n);
        report(io, n);
        exception(io, n);
    }

    /* fini ------------------------------------------------------------- */
    cmt_io_fini(io);
    return 0;
}
