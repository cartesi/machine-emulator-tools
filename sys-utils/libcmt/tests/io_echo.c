/* Copyright Cartesi and individual authors (see AUTHORS)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int next(union cmt_io_driver *io, uint32_t *n) {
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED,
        .data = *n,
    }};
    if (cmt_io_yield(io, req)) {
        (void) fprintf(stderr, "%s:%d failed to yield\n", __FILE__, __LINE__);
        return -1;
    }
    *n = req->data;
    return req->reason;
}

int voucher(union cmt_io_driver *io, uint32_t n) {
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_AUTOMATIC,
        .reason = HTIF_YIELD_AUTOMATIC_REASON_TX_OUTPUT,
        .data = n,
    }};
    return cmt_io_yield(io, req);
}

int report(union cmt_io_driver *io, uint32_t n) {
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_AUTOMATIC,
        .reason = HTIF_YIELD_AUTOMATIC_REASON_TX_REPORT,
        .data = n,
    }};
    return cmt_io_yield(io, req);
}

int exception(union cmt_io_driver *io, uint32_t n) {
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION,
        .data = n,
    }};
    return cmt_io_yield(io, req);
}

int main(void) {
    /* init ------------------------------------------------------------- */
    union cmt_io_driver io[1];
    int rc = cmt_io_init(io);
    if (rc) {
        (void) fprintf(stderr, "%s:%d failed to init with: %s\n", __FILE__, __LINE__, strerror(-rc));
        return EXIT_FAILURE;
    }

    cmt_buf_t tx = cmt_io_get_tx(io);
    cmt_buf_t rx = cmt_io_get_rx(io);

    uint32_t n = 0;
    while (next(io, &n) >= 0) {
        memcpy(tx.begin, rx.begin, n);
        voucher(io, n);
        report(io, n);
        exception(io, n);
    }

    /* fini ------------------------------------------------------------- */
    cmt_io_fini(io);
    return 0;
}
