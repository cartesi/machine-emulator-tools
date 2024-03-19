#include "libcmt/rollup.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    cmt_rollup_t rollup;
    if (cmt_rollup_init(&rollup))
        return EXIT_FAILURE;

    for (;;) {
        int rc;
        cmt_rollup_finish_t finish = {.accept_previous_request = true};
        cmt_rollup_advance_t advance;
        // cmt_rollup_inspect_t inspect;
        if (cmt_rollup_finish(&rollup, &finish) < 0)
            return EXIT_FAILURE;
        switch (finish.next_request_type) {
            case HTIF_YIELD_REASON_ADVANCE:
                rc = cmt_rollup_read_advance_state(&rollup, &advance);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on advance %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }

                rc = cmt_rollup_emit_voucher(&rollup, sizeof advance.sender, advance.sender, 0, NULL, advance.length, advance.data);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on voucher %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }

                rc = cmt_rollup_emit_notice(&rollup, advance.length, advance.data);
                if (rc < 0) {
                    fprintf(stderr, "%s:%d Error on voucher %s (%d)\n", __FILE__, __LINE__, strerror(-rc), (-rc));
                    break;
                }
                break;
            case HTIF_YIELD_REASON_INSPECT:
                break;
        }
    }

    return 0;
}
