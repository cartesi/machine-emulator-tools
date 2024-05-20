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

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "libcmt/rollup.h"

static void help(const char *progname) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "Where options are: \n"
        "  --vouchers=<n>    replicate input in n vouchers (default: 0)\n"
        "  --notices=<n>     replicate input in n notices (default: 0)\n"
        "  --reports=<n>     replicate input in n reports (default: 1)\n"
        "  --reject=<n>      reject the nth input (default: -1)\n"
        "  --reject-inspects reject all inspects\n"
        "  --exception=<n>   cause an exception on the nth input (default: -1)\n"
        "  --verbose=<n>     display information of structures (default: 0)\n",
        progname);

    exit(1);
}

struct parsed_args {
    unsigned voucher_count;
    unsigned notice_count;
    unsigned report_count;
    unsigned verbose;
    unsigned reject;
    bool reject_inspects;
    unsigned exception;
};

static int parse_token(const char *s, const char *fmt, bool *yes) {
    return *yes |= strcmp(fmt, s) == 0;
}

static int parse_number(const char *s, const char *fmt, unsigned *number) {
    int end = 0;
    if (sscanf(s, fmt, number, &end) != 1 || s[end] != 0) {
        return 0;
    }
    return 1;
}

static void parse_args(int argc, char *argv[], struct parsed_args *args) {
    int i = 0;
    const char *progname = argv[0];

    memset(args, 0, sizeof(*args));
    args->report_count = 1;
    args->reject = -1;
    args->reject_inspects = 0;
    args->exception = -1;

    for (i = 1; i < argc; i++) {
        if (!parse_number(argv[i], "--vouchers=%u%n", &args->voucher_count) &&
            !parse_number(argv[i], "--notices=%u%n", &args->notice_count) &&
            !parse_number(argv[i], "--reports=%u%n", &args->report_count) &&
            !parse_number(argv[i], "--verbose=%u%n", &args->verbose) &&
            !parse_token(argv[i], "--reject-inspects", &args->reject_inspects) &&
            !parse_number(argv[i], "--exception=%u%n", &args->exception) &&
            !parse_number(argv[i], "--reject=%u%n", &args->reject)) {
            help(progname);
        }
    }
}

static int finish_request(cmt_rollup_t *me, cmt_rollup_finish_t *finish, bool accept) {
    finish->accept_previous_request = accept;
    return cmt_rollup_finish(me, finish);
}

static int write_notices(cmt_rollup_t *me, unsigned count, cmt_abi_bytes_t *payload) {
    for (unsigned i = 0; i < count; i++) {
        int rc = cmt_rollup_emit_notice(me, payload, NULL);
        if (rc)
            return rc;
    }
    return 0;
}

static int write_vouchers(cmt_rollup_t *me, unsigned count, cmt_abi_address_t *destination, cmt_abi_bytes_t *payload) {
    cmt_abi_u256_t value = {{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef,
    }};
    for (unsigned i = 0; i < count; i++) {
        int rc = cmt_rollup_emit_voucher(me, destination, &value, payload, NULL);
        if (rc)
            return rc;
    }
    return 0;
}

static int write_reports(cmt_rollup_t *me, unsigned count, cmt_abi_bytes_t *payload) {
    for (unsigned i = 0; i < count; i++) {
        int rc = cmt_rollup_emit_report(me, payload);
        if (rc)
            return rc;
    }
    return 0;
}

static int handle_advance_state_request(cmt_rollup_t *me, struct parsed_args *args, uint64_t *index) {
    cmt_rollup_advance_t advance;
    int rc = cmt_rollup_read_advance_state(me, &advance);
    if (rc)
        return rc;
    *index = advance.index;
    fprintf(stderr, "advance with index %d\n", (int) advance.index);
    if (write_vouchers(me, args->voucher_count, &advance.msg_sender, &advance.payload) != 0) {
        return -1;
    }
    if (write_notices(me, args->notice_count, &advance.payload) != 0) {
        return -1;
    }
    if (write_reports(me, args->report_count, &advance.payload) != 0) {
        return -1;
    }
    return 0;
}

static int handle_inspect_state_request(cmt_rollup_t *me, struct parsed_args *args) {
    cmt_rollup_inspect_t inspect;
    int rc = cmt_rollup_read_inspect_state(me, &inspect);
    if (rc)
        return rc;

    if (write_reports(me, args->report_count, &inspect.payload) != 0) {
        return -1;
    }
    return 0;
}

static int handle_request(cmt_rollup_t *me, struct parsed_args *args, cmt_rollup_finish_t *finish, uint64_t *index) {
    switch (finish->next_request_type) {
        case HTIF_YIELD_REASON_ADVANCE:
            return handle_advance_state_request(me, args, index);
        case HTIF_YIELD_REASON_INSPECT:
            return handle_inspect_state_request(me, args);
        default:
            /* unknown request type */
            fprintf(stderr, "Unknown request type %d\n", finish->next_request_type);
            return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    cmt_rollup_t rollup;
    uint64_t advance_index = 0;

    if (cmt_rollup_init(&rollup))
        return EXIT_FAILURE;

    struct parsed_args args;
    parse_args(argc, argv, &args);

    fprintf(stderr, "Echoing as %d voucher copies, %d notice copies, and %d report copies\n", args.voucher_count,
        args.notice_count, args.report_count);

    /* Accept the initial request */
    cmt_rollup_finish_t finish;
    if (finish_request(&rollup, &finish, true) != 0) {
        exit(1);
    }

    /* handle a request, then wait for next */
    for (;;) {
        bool reject_advance, reject_inspect, throw_exception;
        if (handle_request(&rollup, &args, &finish, &advance_index) != 0) {
            break;
        }
        reject_advance = (finish.next_request_type == HTIF_YIELD_REASON_ADVANCE) && (args.reject == advance_index);
        reject_inspect = (finish.next_request_type == HTIF_YIELD_REASON_INSPECT) && args.reject_inspects;
        throw_exception = (finish.next_request_type == HTIF_YIELD_REASON_ADVANCE) && (args.exception == advance_index);
        if (throw_exception) {
            char message[] = "exception";
            const cmt_abi_bytes_t payload = {
                .data = message,
                .length = sizeof message - 1,
            };
            cmt_rollup_emit_exception(&rollup, &payload);
        }
        if (finish_request(&rollup, &finish, !(reject_advance || reject_inspect)) != 0) {
            break;
        }
    }

    cmt_rollup_fini(&rollup);
    fprintf(stderr, "Exiting...\n");
    return 0;
}
