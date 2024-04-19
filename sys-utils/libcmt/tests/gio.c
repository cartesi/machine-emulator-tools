#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "rollup.h"
#include "data.h"

static int read_whole_file(const char *name, size_t max, void *data, size_t *length) {
    int rc = 0;

    FILE *file = fopen(name, "rb");
    if (!file) {
        return -errno;
    }

    *length = fread(data, 1, max, file);
    if (!feof(file)) {
        rc = -ENOBUFS;
    }
    if (fclose(file) != 0) {
        rc = -errno;
    }
    return rc;
}

static int write_whole_file(const char *name, size_t length, const void *data) {
    int rc = 0;

    FILE *file = fopen(name, "wb");
    if (!file) {
        return -errno;
    }

    if (fwrite(data, 1, length, file) != length) {
        rc = -EIO;
    }
    if (fclose(file) != 0) {
        rc = -errno;
    }
    return rc;
}

int main(void)
{
    uint8_t buffer[1024];
    size_t buffer_length;
    cmt_rollup_t rollup;

    assert(write_whole_file("build/gio.bin", sizeof valid_gio_reply_0, valid_gio_reply_0) == 0);
    assert(setenv("CMT_INPUTS", "42:build/gio.bin", 1) == 0);
    assert(cmt_rollup_init(&rollup) == 0);

    char request[] = "gio-request-0";
    char reply[] = "gio-reply-0";

    cmt_gio_t req = {
        .domain = 0xFEFE,
        .id = request,
        .id_length = strlen(request),
    };

    assert(cmt_gio_request(&rollup, &req) == 0);
    assert(read_whole_file("none.gio-0.bin", sizeof buffer, buffer, &buffer_length) == 0);
    assert(req.response_code == 42);
    assert(req.response_data_length == strlen(reply));
    assert(memcmp(req.response_data, reply, req.response_data_length) == 0);

    cmt_rollup_fini(&rollup);
    printf("test_gio_request passed!\n");
    return 0;
}
