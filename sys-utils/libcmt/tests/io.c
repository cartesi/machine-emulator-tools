#include "io.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void invalid_parameters(void) {
    assert(cmt_io_init(NULL) == -EINVAL);
    cmt_io_fini(NULL);

    cmt_buf_t tx = cmt_io_get_tx(NULL);
    assert(tx.begin == NULL && tx.end == NULL);

    cmt_buf_t rx = cmt_io_get_rx(NULL);
    assert(rx.begin == NULL && rx.end == NULL);

    setenv("CMT_INPUTS", "invalid", 1);
    cmt_io_driver_t io[1];
    assert(cmt_io_init(io) == 0);

    { // invalid cmd + reason
        cmt_io_yield_t rr[1] = {{
            .cmd = HTIF_YIELD_CMD_AUTOMATIC,
            .reason = UINT16_MAX,
        }};
        assert(cmt_io_yield(io, rr) == -EINVAL);
    }

    { // invalid cmd
        cmt_io_yield_t rr[1] = {{
            .cmd = 0xff,
            .reason = 0,
        }};
        assert(cmt_io_yield(io, rr) == -EINVAL);
    }

    { // invalid args
        cmt_io_yield_t rr[1] = {{
            .reason = HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED,
            .cmd = HTIF_YIELD_CMD_MANUAL,
            .data = 0,
        }};
        assert(cmt_io_yield(io, NULL) == -EINVAL);
        assert(cmt_io_yield(NULL, rr) == -EINVAL);
        assert(cmt_io_yield(io, rr) == -ENODATA);
    }
    cmt_io_fini(io);
}

void file_too_large(void) {
    // create a 4MB file (any value larger than buffer works)
    char valid[] = "/tmp/tmp.XXXXXX";
    assert(mkstemp(valid) > 0);
    assert(truncate(valid, 4 << 20) == 0);

    { // setup input
        char buf[64];
        (void) snprintf(buf, sizeof buf, "0:%s", valid);
        setenv("CMT_DEBUG", "yes", 1);
        setenv("CMT_INPUTS", buf, 1);
    }

    cmt_io_driver_t io[1];
    cmt_io_yield_t rr[1] = {{
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED,
    }};
    assert(cmt_io_init(io) == 0);
    assert(cmt_io_yield(io, rr) == -ENODATA);
    cmt_io_fini(NULL);

    (void) remove(valid);
}

int main(void) {
    invalid_parameters();
    file_too_large();
    return 0;
}
