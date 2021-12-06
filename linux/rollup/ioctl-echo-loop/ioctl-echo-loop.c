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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include "sys/ioctl.h"

#include <linux/types.h>
#include <linux/cartesi/rollup.h>

#define ROLLUP_DEVICE_NAME "/dev/rollup"

static void show_finish(struct rollup_finish *finish);
static void show_advance(struct rollup_advance_state *advance);
static void show_inspect(struct rollup_inspect_state *inspect);
static void show_voucher(struct rollup_voucher *voucher);
static void show_notice(struct rollup_notice *notice);
static void show_report(struct rollup_report *report);

static void help(const char *progname) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "Where options are: \n"
        "  --vouchers=<n>   replicate input in n vouchers (default: 0)\n"
        "  --notices=<n>    replicate input in n notices (default: 0)\n"
        "  --reports=<n>    replicate input in n reports (default: 1)\n"
        "  --reject=<n>     reject the nth input (default: -1)\n"
        "  --verbose=<n>    display information of structures (default: 0)\n",
        progname);

    exit(1);
}

struct parsed_args {
    unsigned voucher_count;
    unsigned notice_count;
    unsigned report_count;
    unsigned verbose;
    unsigned reject;
};

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

    for (i = 1; i < argc; i++) {
        if (!parse_number(argv[i], "--vouchers=%u%n", &args->voucher_count) &&
            !parse_number(argv[i], "--notices=%u%n", &args->notice_count) &&
            !parse_number(argv[i], "--reports=%u%n", &args->report_count) &&
            !parse_number(argv[i], "--verbose=%u%n", &args->verbose) &&
            !parse_number(argv[i], "--reject=%u%n", &args->reject)) {
            help(progname);
        }
    }
}

static int finish_request(int fd, struct rollup_finish *finish, bool accept) {
    int res = 0;
    memset(finish, 0, sizeof(*finish));
    finish->accept_previous_request = accept;
    res = ioctl(fd, IOCTL_ROLLUP_FINISH, (unsigned long)finish);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_FINISH returned error %d\n", res);
    }
    return res;
}

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

static int write_notices(int fd, unsigned count, struct rollup_bytes *bytes, unsigned verbose) {
    struct rollup_notice n;
    memset(&n, 0, sizeof(n));
    n.payload = *bytes;
    for (unsigned i = 0; i < count; i++) {
        int res = ioctl(fd, IOCTL_ROLLUP_WRITE_NOTICE, (unsigned long) &n);
        if (res != 0) {
            fprintf(stderr, "IOCTL_ROLLUP_WRITE_NOTICE returned error %d\n", res);
            return res;
        }
        if (verbose)
            show_notice(&n);
    }
    return 0;
}

static int write_vouchers(int fd, unsigned count, struct rollup_bytes *bytes, uint8_t *address, unsigned verbose) {
    struct rollup_voucher v;
    memset(&v, 0, sizeof(v));
    v.payload = *bytes;
    memcpy(v.address, address, sizeof(v.address));
    for (unsigned i = 0; i < count; i++) {
        int res = ioctl(fd, IOCTL_ROLLUP_WRITE_VOUCHER, (unsigned long) &v);
        if (res != 0) {
            fprintf(stderr, "IOCTL_ROLLUP_WRITE_VOUCHER returned error %d\n", res);
            return res;
        }
        if (verbose)
            show_voucher(&v);
    }
    return 0;
}

static int write_reports(int fd, unsigned count, struct rollup_bytes *bytes, unsigned verbose) {
    struct rollup_report r;
    memset(&r, 0, sizeof(r));
    r.payload = *bytes;
    if (verbose)
        show_report(&r);
    for (unsigned i = 0; i < count; i++) {
        int res = ioctl(fd, IOCTL_ROLLUP_WRITE_REPORT, (unsigned long) &r);
        if (res != 0) {
            fprintf(stderr, "IOCTL_ROLLUP_WRITE_REPORT returned error %d\n", res);
            return res;
        }
    }
    return 0;
}

static int handle_advance_state_request(int fd, struct parsed_args *args, struct rollup_finish *finish,
    struct rollup_bytes *bytes) {
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
        return res;
    }
    if (args->verbose)
        show_advance(&req);
    if (write_vouchers(fd, args->voucher_count, bytes, req.metadata.msg_sender, args->verbose) != 0) {
        return -1;
    }
    if (write_notices(fd, args->notice_count, bytes, args->verbose) != 0) {
        return -1;
    }
    if (write_reports(fd, args->report_count, bytes, args->verbose) != 0) {
        return -1;
    }
    return 0;
}

static int handle_inspect_state_request(int fd, struct parsed_args *args, struct rollup_finish *finish,
    struct rollup_bytes *bytes) {
    struct rollup_inspect_state req;
    int res = 0;
    if (resize_bytes(bytes, finish->next_request_payload_length) != 0) {
        fprintf(stderr, "Failed growing payload buffer\n");
        return -1;
    }
    memset(&req, 0, sizeof(req));
    req.payload = *bytes;
    if (args->verbose)
        show_inspect(&req);
    res = ioctl(fd, IOCTL_ROLLUP_READ_INSPECT_STATE, (unsigned long) &req);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_READ_INSPECT_STATE returned error (%d)\n", res);
        return res;
    }
    if (write_reports(fd, args->report_count, bytes, args->verbose) != 0) {
        return -1;
    }
    return 0;
}

static int handle_request(int fd, struct parsed_args *args, struct rollup_finish *finish,
    struct rollup_bytes *bytes) {
    switch (finish->next_request_type) {
        case CARTESI_ROLLUP_ADVANCE_STATE:
            return handle_advance_state_request(fd, args, finish, bytes);
        case CARTESI_ROLLUP_INSPECT_STATE:
            return handle_inspect_state_request(fd, args, finish, bytes);
        default:
            /* unknown request type */
            fprintf(stderr, "Unknown request type %d\n", finish->next_request_type);
            return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    struct parsed_args args;
    struct rollup_bytes bytes;
    int fd;

    parse_args(argc, argv, &args);

    fd = open(ROLLUP_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening device " ROLLUP_DEVICE_NAME "\n");
        return fd;
    }

    fprintf(stderr, "Echoing as %d voucher copies, %d notice copies, and %d report copies\n",
        args.voucher_count, args.notice_count, args.report_count);

    memset(&bytes, 0, sizeof(bytes));

    /* Loop accepting previous request, waiting for next, and handling it */
    for (int i=0 ;; ++i) {
        struct rollup_finish finish;
        if (finish_request(fd, &finish, !(i == args.reject)) != 0) {
            break;
        } else if (args.verbose) {
            show_finish(&finish);
        }
        if (handle_request(fd, &args, &finish, &bytes) != 0) {
            break;
        }
    }

    close(fd);

    fprintf(stderr, "Exiting...\n");

    return 0;
}

static void print_address(uint8_t *address)
{
    for (int i=0; i<CARTESI_ROLLUP_ADDRESS_SIZE; i += 4) {
        for (int j=0; j<4; ++j) {
            printf("%02x", address[i+j]);
        }
        printf("%s", i == CARTESI_ROLLUP_ADDRESS_SIZE-4? "\n" : " ");
    }
}

static void show_finish(struct rollup_finish *finish)
{
    const char *type = "unknown";
    switch (finish->next_request_type) {
    case CARTESI_ROLLUP_ADVANCE_STATE:
        type = "advance";
        break;
    case CARTESI_ROLLUP_INSPECT_STATE:
        type = "inspect";
        break;
    }
    printf(
       "finish:\n"
       "\taccept: %s\n"
       "\ttype:   %s\n"
       "\tlength: %d\n"
       , finish->accept_previous_request? "yes" : "no"
       , type
       , finish->next_request_payload_length);
}

static void show_advance(struct rollup_advance_state *advance)
{
    printf(
        "advance:\n"
        "\tmsg_sender: ");
    print_address(advance->metadata.msg_sender);
    printf(
        "\tblock_number: %lu\n"
        "\ttime_stamp: %lu\n"
        "\tepoch_index: %lu\n"
        "\tinput_index: %lu\n"
        , advance->metadata.block_number
        , advance->metadata.time_stamp
        , advance->metadata.epoch_index
        , advance->metadata.input_index);
}

static void show_inspect(struct rollup_inspect_state *inspect) {
    printf("inspect:\n"
           "\tlength: %lu\n",
        inspect->payload.length);
}

static void show_voucher(struct rollup_voucher *voucher) {
    printf("voucher:\n"
           "\tindex: %lu\n"
           "\tlength: %lu\n"
           "\taddress: ",
        voucher->index, voucher->payload.length);
    print_address(voucher->address);
}

static void show_notice(struct rollup_notice *notice) {
    printf("notice:\n"
           "\tindex: %lu\n"
           "\tlength: %lu\n",
        notice->index, notice->payload.length);
}
static void show_report(struct rollup_report *report)
{
    printf(
        "report:\n"
        "\tlength: %lu\n"
        , report->payload.length);
}
