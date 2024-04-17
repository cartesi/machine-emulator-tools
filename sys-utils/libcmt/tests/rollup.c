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
#include "data.h"
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
        perror("fclose failed");
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
        rc = -errno;
    }
    if (fclose(file) != 0) {
        perror("fclose failed");
    }
    return rc;
}
void test_rollup_init_and_fini(void) {
    cmt_rollup_t rollup;

    // init twice returns failure, same as when using the kernel driver
    assert(cmt_rollup_init(&rollup) == 0);
    assert(cmt_rollup_init(&rollup) == -EBUSY);
    cmt_rollup_fini(&rollup);

    // and should reset when closed
    assert(cmt_rollup_init(&rollup) == 0);
    cmt_rollup_fini(&rollup);

    // double free is a bug, but try to avoid crashing
    cmt_rollup_fini(&rollup);

    // fail to initialize with NULL
    assert(cmt_rollup_init(NULL) == -EINVAL);
    cmt_rollup_fini(NULL);

    printf("test_rollup_init_and_fini passed!\n");
}

static void check_first_input(cmt_rollup_t *rollup) {
    cmt_rollup_advance_t advance;

    assert(cmt_rollup_read_advance_state(rollup, &advance) == 0);
    assert(advance.chain_id == 1);
    // clang-format off
    uint8_t expected_app_contract[CMT_ADDRESS_LENGTH] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02,
    };
    uint8_t expected_msg_sender[CMT_ADDRESS_LENGTH] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x03,
    };
    char expected_payload[] = "advance-0";
    // clang-format on

    // verify the parsed data
    assert(memcmp(advance.app_contract, expected_app_contract, CMT_ADDRESS_LENGTH) == 0);
    assert(memcmp(advance.msg_sender, expected_msg_sender, CMT_ADDRESS_LENGTH) == 0);
    assert(advance.block_number == 4);
    assert(advance.block_timestamp == 5);
    assert(advance.index == 6);
    assert(advance.payload_length == strlen(expected_payload));
    assert(memcmp(advance.payload, expected_payload, strlen(expected_payload)) == 0);
}

static void check_second_input(cmt_rollup_t *rollup) {
    cmt_rollup_inspect_t inspect;

    char expected_payload[] = "inspect-0";
    assert(cmt_rollup_read_inspect_state(rollup, &inspect) == 0);
    assert(inspect.payload_length == strlen(expected_payload));
    assert(memcmp(inspect.payload, expected_payload, strlen(expected_payload)) == 0);
}

void test_rollup_parse_inputs(void) {
    cmt_rollup_t rollup;
    cmt_rollup_finish_t finish = {.accept_previous_request = true};

    uint8_t data[] = {0};

    // synthesize inputs and feed them to io-mock via CMT_INPUTS env.
    assert(write_whole_file("build/0.bin", sizeof valid_advance_0, valid_advance_0) == 0);
    assert(write_whole_file("build/1.bin", sizeof valid_inspect_0, valid_inspect_0) == 0);
    assert(write_whole_file("build/2.bin", sizeof data, data) == 0);
    assert(truncate("build/2.bin", 3 << 20) == 0);
    assert(setenv("CMT_INPUTS", "0:build/0.bin,1:build/1.bin,0:build/2.bin", 1) == 0);

    assert(cmt_rollup_init(&rollup) == 0);
    assert(cmt_rollup_finish(&rollup, &finish) == 0);
    check_first_input(&rollup);

    assert(cmt_rollup_finish(&rollup, &finish) == 0);
    check_second_input(&rollup);

    assert(cmt_rollup_finish(&rollup, &finish) == -ENODATA);

    cmt_rollup_fini(&rollup);
    printf("test_rollup_parse_inputs passed!\n");
}

void test_rollup_outputs_reports_and_exceptions(void) {
    cmt_rollup_t rollup;
    uint64_t index = 0;

    // test data must fit
    uint8_t buffer[1024];
    size_t read_size = 0;

    // clang-format off
    uint8_t address[CMT_ADDRESS_LENGTH] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01
    };
    // clang-format on

    assert(cmt_rollup_init(&rollup) == 0);

    // voucher
    uint8_t value[] = {0xde, 0xad, 0xbe, 0xef};
    char voucher_data[] = "voucher-0";
    assert(cmt_rollup_emit_voucher(&rollup, sizeof address, address, sizeof value, value, strlen(voucher_data),
               voucher_data, &index) == 0);
    assert(index == 0);
    assert(read_whole_file("none.output-0.bin", sizeof buffer, buffer, &read_size) == 0);
    assert(sizeof valid_voucher_0 == read_size);
    assert(memcmp(valid_voucher_0, buffer, sizeof valid_voucher_0) == 0);

    // notice
    char notice_data[] = "notice-0";
    assert(cmt_rollup_emit_notice(&rollup, strlen(notice_data), notice_data, &index) == 0);
    assert(read_whole_file("none.output-1.bin", sizeof buffer, buffer, &read_size) == 0);
    assert(sizeof valid_notice_0 == read_size);
    assert(memcmp(valid_notice_0, buffer, sizeof valid_notice_0) == 0);
    assert(index == 1);

    // report
    char report_data[] = "report-0";
    assert(cmt_rollup_emit_report(&rollup, strlen(report_data), report_data) == 0);
    assert(read_whole_file("none.report-0.bin", sizeof buffer, buffer, &read_size) == 0);
    assert(sizeof valid_report_0 == read_size);
    assert(memcmp(valid_report_0, buffer, sizeof valid_report_0) == 0);

    // exception
    char exception_data[] = "exception-0";
    assert(cmt_rollup_emit_exception(&rollup, strlen(exception_data), exception_data) == 0);
    assert(read_whole_file("none.exception-0.bin", sizeof buffer, buffer, &read_size) == 0);
    assert(sizeof valid_exception_0 == read_size);
    assert(memcmp(valid_exception_0, buffer, sizeof valid_exception_0) == 0);

    cmt_rollup_fini(&rollup);
    printf("test_rollup_outputs_reports_and_exceptions passed!\n");
}

int main(void) {
    test_rollup_init_and_fini();
    test_rollup_parse_inputs();
    test_rollup_outputs_reports_and_exceptions();
    return 0;
}
