#include<stdio.h>
#include<string.h>
#include "libcmt/rollup.h"

int main()
{
    cmt_rollup_t rollup;
    if (cmt_rollup_init(&rollup))
        return -1;

    cmt_buf_t tx[1] = {cmt_io_get_tx(rollup.io)};
    cmt_buf_t rx[1] = {cmt_io_get_rx(rollup.io)};

    int m = snprintf((char *)tx->begin, cmt_buf_length(tx), "request");
    struct cmt_io_yield req[1] = {{
        .dev    = CMT_IO_DEV,
        .cmd    = CMT_IO_CMD_MANUAL,
        .reason = 10,
        .data   = m,
    }};
    int rc = cmt_io_yield(rollup.io, req);
    printf("%d:%s \"%.*s\"\n", rc, strerror(rc), req->data, (char *)rx->begin);

    return 0;
}

