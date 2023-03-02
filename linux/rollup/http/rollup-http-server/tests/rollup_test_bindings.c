/* Copyright 2022 Cartesi Pte. Ltd.
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

#include "rollup_test_bindings.h"

// Every uneven request is advance request,
// every even request is inspect request
static int request_test_counter = 0;
static int voucher_index_counter = 0;
static int notice_index_counter = 0;
static int report_index_counter = 0;
static int exception_index_counter = 0;

static const char test_advance_request_str[] = "test advance request";
static const size_t test_advance_request_str_size = sizeof(test_advance_request_str)-1;
static const char test_inspect_request_str[] = "test inspect request";
static const size_t test_inspect_request_str_size = sizeof(test_inspect_request_str)-1;

static const struct rollup_advance_state test_advance_request = {
    .metadata = {
        .msg_sender = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                       0x11, 0x11, 0x11, 0x11},
        .block_number = 0,
        .timestamp = 0,
        .epoch_index = 0,
        .input_index = 0
    },
    .payload = {
        .data = test_advance_request_str,
        .length = test_advance_request_str_size
    }
};

static const struct rollup_inspect_state test_inspect_request = {
    .payload = {
        .data = test_inspect_request_str,
        .length = test_inspect_request_str_size
    }
};

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
    request_test_counter += 1;
    if (request_test_counter % 2 == 0) {
        //test inspect request
        finish->next_request_type = CARTESI_ROLLUP_INSPECT_STATE;
        finish->next_request_payload_length = test_inspect_request.payload.length;
    } else {
        // test advance request
        finish->next_request_type = CARTESI_ROLLUP_ADVANCE_STATE;
        finish->next_request_payload_length = test_advance_request.payload.length;
    }

    return res;
}

/* Returns test advance state rollup request
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
    // test advance request
    *metadata = test_advance_request.metadata;
    *bytes = test_advance_request.payload;
    return res;
}

/* Returns test inspect state rollup request
 * Returns 0 on success, -1 on error */
int rollup_read_inspect_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *query) {
    struct rollup_inspect_state req;
    int res = 0;
    if (resize_bytes(query, finish->next_request_payload_length) != 0) {
        fprintf(stderr, "Failed growing payload buffer\n");
        return -1;
    }
    memset(&req, 0, sizeof(req));
    *query = test_inspect_request.payload;
    return 0;
}

/* Outputs a new voucher to a file test_voucher_xx.txt in a text file
 * voucher_index is filled with new index from the driver
 * Returns 0 on success, -1 on error */
int rollup_write_voucher(int fd, uint8_t destination[CARTESI_ROLLUP_ADDRESS_SIZE], struct rollup_bytes *bytes,
                          uint64_t *voucher_index) {
    char filename[32] = {0};
    char destination_c[CARTESI_ROLLUP_ADDRESS_SIZE + 1] = {0};
    voucher_index_counter = voucher_index_counter + 1;
    sprintf(filename, "test_voucher_%d.txt", voucher_index_counter);
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }
    memcpy(destination_c, destination, CARTESI_ROLLUP_ADDRESS_SIZE);
    fprintf(f, "index: %d, payload_size: %d, payload: ", voucher_index_counter, bytes->length);
    for (int i = 0; i < bytes->length; i++) {
        fputc(bytes->data[i], f);
    }
    fclose(f);
    *voucher_index = voucher_index_counter;
    return 0;
}

/* Outputs a new notice to a file test_notice_xx.txt
 * notice_index is filled with new index from the driver
 * Returns 0 on success, -1 on error */
int rollup_write_notice(int fd, struct rollup_bytes *bytes, uint64_t *notice_index) {
    char filename[32] = {0};
    notice_index_counter = notice_index_counter + 1;
    sprintf(filename, "test_notice_%d.txt", notice_index_counter);
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }
    fprintf(f, "index: %d, payload_size: %d, payload: ", notice_index_counter, bytes->length);
    for (int i = 0; i < bytes->length; i++) {
        fputc(bytes->data[i], f);
    }
    fclose(f);
    *notice_index = notice_index_counter;
    return 0;
}


/* Outputs a new report to a file test_report_xx.txt
 * Returns 0 on success, -1 on error */
int rollup_write_report(int fd, struct rollup_bytes *bytes) {
    char filename[32] = {0};
    report_index_counter = report_index_counter + 1;
    sprintf(filename, "test_report_%d.txt", report_index_counter);
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }
    fprintf(f, "index: %d, payload_size: %d, payload: ", report_index_counter, bytes->length);
    for (int i = 0; i < bytes->length; i++) {
        fputc(bytes->data[i], f);
    }
    fclose(f);
    return 0;
}

/* Outputs a dapp exception  to a file test_exception_xx.txt
 * Returns 0 on success, -1 on error */
int rollup_throw_exception(int fd, struct rollup_bytes *bytes) {
    char filename[32] = {0};
    exception_index_counter = exception_index_counter + 1;
    sprintf(filename, "test_exception_%d.txt", exception_index_counter);
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        return -1;
    }
    fprintf(f, "index: %d, payload_size: %d, payload: ", exception_index_counter, bytes->length);
    for (int i = 0; i < bytes->length; i++) {
        fputc(bytes->data[i], f);
    }
    fclose(f);
    return 0;
}

