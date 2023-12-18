#include "libcmt/rollup.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    cmt_rollup_t rollup;
    if (!cmt_rollup_init(&rollup))
        return EXIT_FAILURE;
    // cmt_rollup_load_merkle(rollup, "/tmp/merkle.dat");

    for (;;) {
        int rc;
        cmt_rollup_finish_t finish = {.accept_previous_request = true};
        cmt_rollup_advance_t advance;
        // cmt_rollup_inspect_t inspect;

        if (cmt_rollup_finish(&rollup, &finish) < 0)
            goto teardown;

        switch (finish.next_request_type) {
            case CMT_IO_REASON_ADVANCE:
                rc = cmt_rollup_read_advance_state(&rollup, &advance);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on advance %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }

                rc = cmt_rollup_emit_voucher(&rollup, advance.sender, advance.length, advance.data);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on voucher %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }

                rc = cmt_rollup_emit_notice(&rollup, advance.length, advance.data);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on notice %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }

                rc = cmt_rollup_emit_report(&rollup, advance.length, advance.data);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on notice %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }
                break;
            case CMT_IO_REASON_INSPECT:
                break;
        }
    }

teardown:
    // cmt_rollup_save_merkle(&rollup, "/tmp/merkle.dat");
    cmt_rollup_fini(&rollup);
    return 0;
}
