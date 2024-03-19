#include "io.h"
#include "libcmt/io.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    /* init ------------------------------------------------------------- */
    struct cmt_io_driver io[1];
    if (cmt_io_init(io)) {
        fprintf(stderr, "%s:%d failed to init\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    size_t tx_sz, rx_sz;
    uint8_t *tx, *rx;
    tx = cmt_io_get_tx(io, &tx_sz), rx = cmt_io_get_rx(io, &rx_sz);

    /* prepare exception ------------------------------------------------ */
    const char message[] = "exception contents\n";
    size_t n = strlen(message);
    memcpy(tx, message, n);

    /* exception -------------------------------------------------------- */
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_MANUAL,
        .reason = CMT_IO_MANUAL_REASON_TX_EXCEPTION,
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
