#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool cmt_util_debug_enabled(void) {
    static bool checked = false;
    static bool enabled = false;

    if (!checked) {
        enabled = getenv("CMT_DEBUG") != NULL;
        checked = true;
    }

    return enabled;
}

int cmt_util_read_whole_file(const char *name, size_t max, void *data, size_t *length) {
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

int cmt_util_write_whole_file(const char *name, size_t length, const void *data) {
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

