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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/cartesi/rollup.h>

#define ROLLUP_DEVICE_NAME "/dev/rollup"

static void show_finish(struct rollup_finish *finish);
static void show_advance(struct rollup_advance_state *advance);
static void show_inspect(struct rollup_inspect_state *inspect);
static void show_voucher(struct rollup_voucher *voucher);
static void show_notice(struct rollup_notice *notice);
static void show_report(struct rollup_report *report);
static void show_exception(struct rollup_exception *exception);

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

static int finish_request(int fd, struct rollup_finish *finish, bool accept) {
    int res = 0;
    memset(finish, 0, sizeof(*finish));
    finish->accept_previous_request = accept;
    res = ioctl(fd, IOCTL_ROLLUP_FINISH, (unsigned long) finish);
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

static int write_vouchers(int fd, unsigned count, struct rollup_bytes *bytes, uint8_t *destination, unsigned verbose) {
    struct rollup_voucher v;
    memset(&v, 0, sizeof(v));
    v.payload = *bytes;
    memcpy(v.destination, destination, sizeof(v.destination));
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

static int write_exception(int fd, struct rollup_bytes *bytes, unsigned verbose) {
    struct rollup_exception e;
    memset(&e, 0, sizeof(e));
    e.payload = *bytes;
    if (verbose)
        show_exception(&e);
    int res = ioctl(fd, IOCTL_ROLLUP_THROW_EXCEPTION, (unsigned long) &e);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_THROW_EXCEPTION returned error %d\n", res);
        return res;
    }
    return 0;
}

static int handle_advance_state_request(int fd, struct parsed_args *args, struct rollup_finish *finish, struct rollup_bytes *bytes, struct rollup_input_metadata *metadata) {
    struct rollup_advance_state req;
    int res = 0;
    if (resize_bytes(bytes, finish->next_request_payload_length) != 0) {
        fprintf(stderr, "Failed growing payload buffer\n");
        return -1;
    }
    memset(&req, 0, sizeof(req));
    req.payload.data = bytes->data;
    req.payload.length = finish->next_request_payload_length;
    res = ioctl(fd, IOCTL_ROLLUP_READ_ADVANCE_STATE, (unsigned long) &req);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_READ_ADVANCE_STATE returned error (%d)\n", res);
        return res;
    }
    *metadata = req.metadata;
    if (args->verbose)
        show_advance(&req);
    if (write_vouchers(fd, args->voucher_count, &req.payload, req.metadata.msg_sender, args->verbose) != 0) {
        return -1;
    }
    if (write_notices(fd, args->notice_count, &req.payload, args->verbose) != 0) {
        return -1;
    }
    if (write_reports(fd, args->report_count, &req.payload, args->verbose) != 0) {
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
    req.payload.data = bytes->data;
    req.payload.length = finish->next_request_payload_length;
    if (args->verbose)
        show_inspect(&req);
    res = ioctl(fd, IOCTL_ROLLUP_READ_INSPECT_STATE, (unsigned long) &req);
    if (res != 0) {
        fprintf(stderr, "IOCTL_ROLLUP_READ_INSPECT_STATE returned error (%d)\n", res);
        return res;
    }
    if (write_reports(fd, args->report_count, &req.payload, args->verbose) != 0) {
        return -1;
    }
    return 0;
}

static int handle_request(int fd, struct parsed_args *args, struct rollup_finish *finish, struct rollup_bytes *bytes, struct rollup_input_metadata *metadata) {
    switch (finish->next_request_type) {
        case CARTESI_ROLLUP_ADVANCE_STATE:
            return handle_advance_state_request(fd, args, finish, bytes, metadata);
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
    struct rollup_input_metadata metadata;
    struct rollup_finish finish;
    struct parsed_args args;
    struct rollup_bytes bytes;
    int fd;

    memset(&metadata, 0, sizeof(metadata));
    parse_args(argc, argv, &args);

    fd = open(ROLLUP_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening device " ROLLUP_DEVICE_NAME "\n");
        return fd;
    }

    fprintf(stderr, "Echoing as %d voucher copies, %d notice copies, and %d report copies\n", args.voucher_count,
        args.notice_count, args.report_count);

    memset(&bytes, 0, sizeof(bytes));

    /* Accept the initial request */
    if (finish_request(fd, &finish, true) != 0) {
        exit(1);
    } else if (args.verbose) {
        show_finish(&finish);
    }

    /* handle a request, then wait for next */
    for (;;) {
        bool reject_advance, reject_inspect, throw_exception;
        if (handle_request(fd, &args, &finish, &bytes, &metadata) != 0) {
            break;
        }
        reject_advance =
            (finish.next_request_type == CARTESI_ROLLUP_ADVANCE_STATE) &&
            (args.reject == metadata.input_index);
        reject_inspect =
            (finish.next_request_type == CARTESI_ROLLUP_INSPECT_STATE) && args.reject_inspects;
        throw_exception = (finish.next_request_type == CARTESI_ROLLUP_ADVANCE_STATE) &&
            (args.exception == metadata.input_index);
        if (throw_exception) {
            write_exception(fd, &bytes, args.verbose);
        }
        if (finish_request(fd, &finish, !(reject_advance || reject_inspect)) != 0) {
            break;
        } else if (args.verbose) {
            show_finish(&finish);
        }
    }

    close(fd);

    fprintf(stderr, "Exiting...\n");

    return 0;
}

static void print_address(uint8_t *address) {
    for (int i = 0; i < CARTESI_ROLLUP_ADDRESS_SIZE; i += 4) {
        for (int j = 0; j < 4; ++j) {
            printf("%02x", address[i + j]);
        }
        printf("%s", i == CARTESI_ROLLUP_ADDRESS_SIZE - 4 ? "\n" : " ");
    }
}

static void show_finish(struct rollup_finish *finish) {
    const char *type = "unknown";
    switch (finish->next_request_type) {
        case CARTESI_ROLLUP_ADVANCE_STATE:
            type = "advance";
            break;
        case CARTESI_ROLLUP_INSPECT_STATE:
            type = "inspect";
            break;
    }
    printf("finish:\n"
           "\taccept: %s\n"
           "\ttype:   %s\n"
           "\tlength: %d\n",
        finish->accept_previous_request ? "yes" : "no", type, finish->next_request_payload_length);
}

static void show_advance(struct rollup_advance_state *advance) {
    printf("advance:\n"
           "\tmsg_sender: ");
    print_address(advance->metadata.msg_sender);
    printf("\tblock_number: %lu\n"
           "\ttimestamp: %lu\n"
           "\tepoch_index: %lu\n"
           "\tinput_index: %lu\n",
        advance->metadata.block_number, advance->metadata.timestamp, advance->metadata.epoch_index,
        advance->metadata.input_index);
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
           "\tdestination: ",
        voucher->index, voucher->payload.length);
    print_address(voucher->destination);
}

static void show_notice(struct rollup_notice *notice) {
    printf("notice:\n"
           "\tindex: %lu\n"
           "\tlength: %lu\n",
        notice->index, notice->payload.length);
}

static void show_report(struct rollup_report *report) {
    printf("report:\n"
           "\tlength: %lu\n",
        report->payload.length);
}

static void show_exception(struct rollup_exception *exception) {
    printf("exception:\n"
           "\tlength: %lu\n",
        exception->payload.length);
}
