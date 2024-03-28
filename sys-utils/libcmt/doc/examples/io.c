#include "libcmt/io.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int exception(union cmt_io_driver *io) {
    /* init ------------------------------------------------------------- */
    if (cmt_io_init(io)) {
        fprintf(stderr, "%s:%d failed to init\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    cmt_buf_t tx = cmt_io_get_tx(io);

    /* prepare exception ------------------------------------------------ */
    const char message[] = "exception contents\n";
    size_t n = strlen(message);
    memcpy(tx.begin, message, n);

    /* exception -------------------------------------------------------- */
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION,
        .data = n,
    }};
    if (cmt_io_yield(io, req)) {
        fprintf(stderr, "%s:%d failed to yield\n", __FILE__, __LINE__);
        return -1;
    }

    /* fini ------------------------------------------------------------- */
    cmt_io_fini(io);
    return 0;
}
