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
#include "buf.h"
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    /* init ------------------------------------------------------------- */
    union cmt_io_driver io[1];
    if (cmt_io_init(io)) {
        (void) fprintf(stderr, "%s:%d failed to init\n", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    cmt_buf_t tx = cmt_io_get_tx(io);

    /* prepare exception ------------------------------------------------ */
    const char message[] = "exception contents\n";
    size_t n = strlen(message);
    tx.begin = (uint8_t *) message;
    tx.end = tx.begin + n;

    /* exception -------------------------------------------------------- */
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION,
        .data = n,
    }};
    if (cmt_io_yield(io, req)) {
        (void) fprintf(stderr, "%s:%d failed to yield\n", __FILE__, __LINE__);
        return -1;
    }

    /* fini ------------------------------------------------------------- */
    cmt_io_fini(io);
    return 0;
}
