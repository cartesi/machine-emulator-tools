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
#include "rollup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Data encoding:
 * - cast calldata "EvmAdvance(address,uint256,uint256,uint256,bytes)" \
 *   0x0000000000000000000000000000000000000000 \
 *   0x0000000000000000000000000000000000000001 \
 *   0x0000000000000000000000000000000000000002 \
 *   0x0000000000000000000000000000000000000003 \
 *   0x`echo "hello world" | xxd -r -p -c0` > "<input.bin>"
 *
 * Data decoding:
 * - cast calldata-decode "Voucher(address,uint256,bytes)" 0x`xxd -p -c0 "<output.bin>"`
 *
 */
int main(void) {
    cmt_rollup_t rollup;

    if (cmt_rollup_init(&rollup)) {
        return EXIT_FAILURE;
    }
    // cmt_rollup_load_merkle(rollup, "/tmp/merkle.dat");

    uint8_t small[] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
    };
    for (;;) {
        int rc = 0;
        cmt_rollup_finish_t finish = {.accept_previous_request = true};
        cmt_rollup_advance_t advance;
        // cmt_rollup_inspect_t inspect;

        if (cmt_rollup_finish(&rollup, &finish) < 0) {
            goto teardown;
        }

        switch (finish.next_request_type) {
            case HTIF_YIELD_REASON_ADVANCE:
                rc = cmt_rollup_read_advance_state(&rollup, &advance);
                if (rc < 0) {
                    (void) fprintf(stderr, "%s:%d Error on advance %s (%d)\n", __FILE__, __LINE__, strerror(-rc),
                        (-rc));
                    break;
                }

                rc = cmt_rollup_emit_voucher(&rollup, sizeof advance.msg_sender, advance.msg_sender, sizeof small,
                    small, advance.payload_length, advance.payload, NULL);
                if (rc < 0) {
                    (void) fprintf(stderr, "%s:%d Error on voucher %s (%d)\n", __FILE__, __LINE__, strerror(-rc),
                        (-rc));
                    break;
                }

                rc = cmt_rollup_emit_notice(&rollup, advance.payload_length, advance.payload, NULL);
                if (rc < 0) {
                    (void) fprintf(stderr, "%s:%d Error on notice %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }

                rc = cmt_rollup_emit_report(&rollup, advance.payload_length, advance.payload);
                if (rc < 0) {
                    (void) fprintf(stderr, "%s:%d Error on notice %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }
                break;
            case HTIF_YIELD_REASON_INSPECT:
            default:
                break;
        }
    }

teardown:
    // cmt_rollup_save_merkle(&rollup, "/tmp/merkle.dat");
    cmt_rollup_fini(&rollup);
    return 0;
}
