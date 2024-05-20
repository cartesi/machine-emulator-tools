#include "data.h"
#include "libcmt/rollup.h"
#include "libcmt/util.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    uint8_t buffer[1024];
    size_t buffer_length = 0;
    cmt_rollup_t rollup;

    assert(cmt_util_write_whole_file("gio.bin", sizeof valid_gio_reply_0, valid_gio_reply_0) == 0);
    assert(setenv("CMT_INPUTS", "42:gio.bin", 1) == 0);
    assert(cmt_rollup_init(&rollup) == 0);

    char request[] = "gio-request-0";
    char reply[] = "gio-reply-0";

    cmt_gio_t req = {
        .domain = 0xFEFE,
        .id = request,
        .id_length = strlen(request),
    };

    assert(cmt_gio_request(&rollup, &req) == 0);
    assert(cmt_util_read_whole_file("none.gio-0.bin", sizeof buffer, buffer, &buffer_length) == 0);
    assert(req.response_code == 42);
    assert(req.response_data_length == strlen(reply));
    assert(memcmp(req.response_data, reply, req.response_data_length) == 0);

    cmt_rollup_fini(&rollup);
    printf("test_gio_request passed!\n");
    return 0;
}
