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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_whole_file(const char *name, size_t max, void *data, size_t *length);
static int write_whole_file(const char *name, size_t length, const void *data);

int cmt_io_init(cmt_io_driver_t *_me) {
    if (!_me)
        return -EINVAL;
    cmt_io_driver_mock_t *me = &_me->mock;

    size_t tx_length = 2 * 1024 * 1024; // 2MB
    size_t rx_length = 2 * 1024 * 1024; // 2MB
    cmt_buf_init(me->tx, tx_length, malloc(tx_length));
    cmt_buf_init(me->rx, rx_length, malloc(rx_length));

    if (!me->tx->begin || !me->rx->begin) {
        free(me->tx->begin);
        free(me->rx->begin);
        return -ENOMEM;
    }

    char *inputs = getenv("CMT_INPUTS");
    if (inputs)
        cmt_buf_init(&me->inputs_left, strlen(inputs), inputs);
    else
        cmt_buf_init(&me->inputs_left, 0, "");

    // in case the user writes something before loading any input
    strcpy(me->input_filename, "none");
    strcpy(me->input_fileext, ".bin");

    me->input_seq = 0;
    me->output_seq = 0;
    me->report_seq = 0;
    me->exception_seq = 0;

    return 0;
}

void cmt_io_fini(cmt_io_driver_t *_me) {
    if (!_me)
        return;
    cmt_io_driver_mock_t *me = &_me->mock;

    free(me->tx->begin);
    free(me->rx->begin);
}

cmt_buf_t cmt_io_get_tx(cmt_io_driver_t *me) {
    cmt_buf_t empty = {NULL, NULL};
    if (!me)
        return empty;
    return *me->ioctl.tx;
}

cmt_buf_t cmt_io_get_rx(cmt_io_driver_t *me) {
    cmt_buf_t empty = {NULL, NULL};
    if (!me)
        return empty;
    return *me->ioctl.rx;
}

static int load_next_input(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    cmt_buf_t current_input;
    char filepath[128] = {0};
    if (!cmt_buf_split_by_comma(&current_input, &me->inputs_left) ||
        sscanf((char *) current_input.begin, "%d:%127[^,]", &me->input_type, filepath) != 2)
        return -EINVAL;

    size_t file_length = 0;
    int rc = read_whole_file(filepath, cmt_buf_length(me->rx), me->rx->begin, &file_length);
    if (rc) {
        fprintf(stderr, "failed to load \"%s\". %s\n", filepath, strerror(-rc));
        return rc;
    }
    sscanf(filepath, " %127[^.]%15s", me->input_filename, me->input_fileext);

    assert(file_length <= INT32_MAX);
    rr->reason = me->input_type;
    rr->data = file_length;

    me->output_seq = 0;
    me->report_seq = 0;
    me->exception_seq = 0;

    if (getenv("CMT_DEBUG")) {
        fprintf(stderr, "processing filename: \"%s\" (%lu), type: %d\n", filepath, file_length, me->input_type);
    }
    return 0;
}

static int store_output(cmt_io_driver_mock_t *me, const char *filepath, struct cmt_io_yield *rr) {
    if (rr->data > cmt_buf_length(me->tx))
        return -ENOBUFS;

    // char filepath[128 + 1 + 8 + 16];
    // snprintf(filepath, sizeof filepath, "%s.%s%d%s", me->input_filename, ns, *seq, me->input_fileext);

    int rc = write_whole_file(filepath, rr->data, me->tx->begin);
    if (rc) {
        fprintf(stderr, "failed to store \"%s\". %s\n", filepath, strerror(-rc));
        return rc;
    }
    if (getenv("CMT_DEBUG")) {
        fprintf(stderr, "wrote filename: \"%s\" (%u)\n", filepath, rr->data);
    }

    // seq[0] += 1;
    return 0;
}

static int store_next_output(cmt_io_driver_mock_t *me, char *ns, int *seq, struct cmt_io_yield *rr) {
    char filepath[128 + 32 + 8 + 16];
    snprintf(filepath, sizeof filepath, "%s.%s%d%s", me->input_filename, ns, *seq++, me->input_fileext);
    return store_output(me, filepath, rr);
}

static int mock_progress(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    (void) me;
    if (rr->cmd != HTIF_YIELD_CMD_AUTOMATIC) {
        fprintf(stderr, "Expected cmd to be AUTOMATIC\n");
        return -EINVAL;
    }
    fprintf(stderr, "Progress: %6.2f\n", (double) rr->data / 10);
    return 0;
}

static int mock_rx_accepted(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    if (rr->cmd != HTIF_YIELD_CMD_MANUAL) {
        fprintf(stderr, "Expected cmd to be MANUAL\n");
        return -EINVAL;
    }
    if (me->input_seq++) { // skip the first
        char filepath[128 + 32 + 8 + 16];
        snprintf(filepath, sizeof filepath, "%s.outputs_root_hash%s", me->input_filename, me->input_fileext);
        int rc = store_output(me, filepath, rr);
        if (rc)
            return rc;
    }
    if (load_next_input(me, rr))
        return -ENODATA;
    return 0;
}

static int mock_rx_rejected(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    if (rr->cmd != HTIF_YIELD_CMD_MANUAL) {
        fprintf(stderr, "Expected cmd to be MANUAL\n");
        return -EINVAL;
    }
    fprintf(stderr, "%s:%d no revert for the mock implementation\n", __FILE__, __LINE__);
    if (load_next_input(me, rr))
        return -1;
    return 0;
}

static int mock_tx_output(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    if (rr->cmd != HTIF_YIELD_CMD_AUTOMATIC) {
        fprintf(stderr, "Expected cmd to be AUTOMATIC\n");
        return -EINVAL;
    }
    return store_next_output(me, "output-", &me->output_seq, rr);
}

static int mock_tx_report(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    if (rr->cmd != HTIF_YIELD_CMD_AUTOMATIC) {
        fprintf(stderr, "Expected cmd to be AUTOMATIC\n");
        return -EINVAL;
    }
    return store_next_output(me, "report-", &me->report_seq, rr);
}

static int mock_tx_exception(cmt_io_driver_mock_t *me, struct cmt_io_yield *rr) {
    if (rr->cmd != HTIF_YIELD_CMD_MANUAL) {
        fprintf(stderr, "Expected cmd to be MANUAL\n");
        return -EINVAL;
    }
    return store_next_output(me, "exception-", &me->exception_seq, rr);
}

/* These behaviours are defined by the cartesi-machine emulator */
int cmt_io_yield(cmt_io_driver_t *_me, struct cmt_io_yield *rr) {
    if (!_me)
        return -EINVAL;
    cmt_io_driver_mock_t *me = &_me->mock;

    if (getenv("CMT_DEBUG")) {
        fprintf(stderr,
            "yield {\n"
            "\t.dev = %d,\n"
            "\t.cmd = %d,\n"
            "\t.reason = %d,\n"
            "\t.data = %d,\n"
            "};\n",
            rr->dev, rr->cmd, rr->reason, rr->data);
    }

    if (rr->cmd == HTIF_YIELD_CMD_MANUAL) {
        switch (rr->reason) {
            case HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED:
                return mock_rx_accepted(me, rr);
            case HTIF_YIELD_MANUAL_REASON_RX_REJECTED:
                return mock_rx_rejected(me, rr);
            case HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION:
                return mock_tx_exception(me, rr);
            default:
                return -EINVAL;
        }
    } else if (rr->cmd == HTIF_YIELD_CMD_AUTOMATIC) {
        switch (rr->reason) {
            case HTIF_YIELD_AUTOMATIC_REASON_PROGRESS:
                return mock_progress(me, rr);
            case HTIF_YIELD_AUTOMATIC_REASON_TX_OUTPUT:
                return mock_tx_output(me, rr);
            case HTIF_YIELD_AUTOMATIC_REASON_TX_REPORT:
                return mock_tx_report(me, rr);
            default:
                return -EINVAL;
        }
    } else {
        return -EINVAL;
    }

    return 0;
}

static int read_whole_file(const char *name, size_t max, void *data, size_t *length) {
    int rc = 0;

    FILE *file = fopen(name, "rb");
    if (!file)
        return -errno;

    *length = fread(data, 1, max, file);
    if (!feof(file))
        rc = -ENOBUFS;
    fclose(file);
    return rc;
}

static int write_whole_file(const char *name, size_t length, const void *data) {
    int rc = 0;

    FILE *file = fopen(name, "wb");
    if (!file)
        return -errno;

    if (fwrite(data, 1, length, file) != length)
        rc = -errno;
    fclose(file);
    return rc;
}
