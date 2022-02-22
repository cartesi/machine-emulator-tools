/* Copyright 2021 Cartesi Pte. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include "bindings.h"

static int resize_bytes(struct rollup_bytes *bytes, uint64_t size) {
    if (bytes->length < size) {
        uint8_t *new_data = (uint8_t *) realloc(bytes->data, size);
        if (!new_data) {
            return -1;
        }
        bytes->length = size;
        bytes->data = new_data;
    }
    return 0;
}

/* Finishes processing of current advance or inspect.
 * Returns 0 on success, -1 on error */
int rollup_finish_request(int fd, struct rollup_finish *finish, bool accept) {
    int res = 0;
    memset(finish, 0, sizeof(*finish));
    finish->accept_previous_request = accept;
    res = ioctl(fd, IOCTL_ROLLUP_FINISH, (unsigned long)finish);
    return res;
}

/* Obtains arguments to advance state
 * Outputs metadata and payload in structs
 * Returns 0 on success, -1 on error */
int rollup_read_advance_state_request(int fd, struct rollup_finish *finish,
        struct rollup_bytes *bytes, struct rollup_input_metadata *metadata) {
    struct rollup_advance_state req;
    int res = 0;
    if (resize_bytes(bytes, finish->next_request_payload_length) != 0) {
        fprintf(stderr, "Failed growing payload buffer\n");
        return -1;
    }
    memset(&req, 0, sizeof(req));
    req.payload = *bytes;
    res = ioctl(fd, IOCTL_ROLLUP_READ_ADVANCE_STATE, (unsigned long) &req);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_READ_ADVANCE_STATE returned error (%d)\n", res);
    }
    *metadata = req.metadata;
    return res;
}


/* Obtains query of inspect state request
 * Returns 0 on success, -1 on error */
int rollup_read_inspect_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *query) {
    struct rollup_inspect_state req;
    int res = 0;
    if (resize_bytes(query, finish->next_request_payload_length) != 0) {
        fprintf(stderr, "Failed growing payload buffer\n");
        return -1;
    }
    memset(&req, 0, sizeof(req));
    req.payload = *query;
    res = ioctl(fd, IOCTL_ROLLUP_READ_INSPECT_STATE, (unsigned long) &req);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_READ_INSPECT_STATE returned error (%d)\n", res);
        return res;
    }
    return 0;
}

/* Outputs a new voucher.
 * voucher_index is filled with new index from the driver
 * Returns 0 on success, -1 on error */
int rollup_write_vouchers(int fd, uint8_t address[CARTESI_ROLLUP_ADDRESS_SIZE], struct rollup_bytes *bytes, uint64_t* voucher_index) {
    unsigned i;
    struct rollup_voucher v;
    memset(&v, 0, sizeof(v));
    memcpy(v.address, address, CARTESI_ROLLUP_ADDRESS_SIZE);
    v.payload = *bytes;
    int res = ioctl(fd, IOCTL_ROLLUP_WRITE_VOUCHER, (unsigned long) &v);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_WRITE_VOUCHER returned error %d\n", res);
        return res;
    }
    *voucher_index = v.index;
    return 0;
}

/* Outputs a new notice.
 * notice_index is filled with new index from the driver
 * Returns 0 on success, -1 on error */
int rollup_write_notices(int fd, struct rollup_bytes *bytes, uint64_t* notice_index) {
    unsigned i;
    struct rollup_notice n;
    memset(&n, 0, sizeof(n));
    n.payload = *bytes;
    int res = ioctl(fd, IOCTL_ROLLUP_WRITE_NOTICE, (unsigned long) &n);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_WRITE_NOTICE returned error %d\n", res);
        return res;
    }
    *notice_index = n.index;
    return 0;
}


/* Outputs a new report.
 * Returns 0 on success, -1 on error */
int rollup_write_reports(int fd, struct rollup_bytes *bytes) {
    unsigned i;
    struct rollup_report r;
    memset(&r, 0, sizeof(r));
    r.payload = *bytes;
    int res = ioctl(fd, IOCTL_ROLLUP_WRITE_REPORT, (unsigned long) &r);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_WRITE_REPORT returned error %d\n", res);
        return res;
    }

    return 0;
}

