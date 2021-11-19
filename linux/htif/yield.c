/* Copyright 2019-2020 Cartesi Pte. Ltd.
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
#include <linux/cartesi/yield.h>

#define YIELD_DEVICE_NAME "/dev/yield"

static void help(const char *progname) {
    fprintf(stderr,
        "Usage: %s [--ack] <mode> <reason> [<data>]\n"
        "Where: \n"
        "  <mode>       \"manual\" or \"automatic\"\n"
        "  <reason>     \"progress\", \"rx-accepted\", \"rx-rejected\",\n"
        "               \"tx-voucher\", \"tx-notice\", or \"tx-report\"\n"
        "  <data>       32-bit unsigned integer (decimal, default 0)\n"
        "  --ack        writes the acknowledge code to stdout\n",
        progname);

    exit(1);
}

int send_request(struct yield_request* request) {
    int fd, res;

    fd = open(YIELD_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening device " YIELD_DEVICE_NAME "\n");
        return fd;
    }

    res = ioctl(fd, IOCTL_YIELD, (unsigned long)request);

    close(fd);
    if (res != 0) {
        fprintf(stderr, "Device returned error %d\n", res);
        return res;
    }
    return 0;
}

int get_mode(const char *s, uint64_t *mode) {
    if (strcmp(s, "manual") == 0) {
        *mode = HTIF_YIELD_MANUAL;
        return 1;
    } else if (strcmp(s, "automatic") == 0) {
        mode = HTIF_YIELD_AUTOMATIC;
        return 1;
    }
    return 0;
}

int get_reason(const char *s, uint64_t *reason) {
    if (strcmp(s, "progress") == 0) {
        *reason = HTIF_YIELD_REASON_PROGRESS;
        return 1;
    } else if (strcmp(s, "tx-report") == 0) {
        *reason = HTIF_YIELD_REASON_TX_REPORT;
        return 1;
    } else if (strcmp(s, "tx-notice") == 0) {
        *reason = HTIF_YIELD_REASON_TX_NOTICE;
        return 1;
    } else if (strcmp(s, "tx-voucher") == 0) {
        *reason = HTIF_YIELD_REASON_TX_VOUCHER;
        return 1;
    } else if (strcmp(s, "rx-accepted") == 0) {
        *reason = HTIF_YIELD_REASON_RX_ACCEPTED;
        return 1;
    } else if (strcmp(s, "rx-rejected") == 0) {
        *reason = HTIF_YIELD_REASON_RX_REJECTED;
        return 1;
    }
    return 0;
}

int get_data(const char *s, uint64_t *data) {
    int end = 0;
    if (sscanf(s, "%" SCNu64 "%n", data, &end) != 1 ||
        s[end] != 0 ||
        *data > UINT32_MAX) {
        return 0;
    }
    return 1;
}

struct parsed_args {
    int output_ack;
    uint64_t mode;
    uint64_t reason;
    uint64_t data;
};

void parse_args(int argc, char *argv[], struct parsed_args *p) {
    int i = 0;
    const char *progname = argv[0];

    memset(p, 0, sizeof(*p));

    i = 1;
    if (i >= argc) {
        fprintf(stderr, "Too few arguments.\n\n");
        help(progname);
    }

    if (!strcmp(argv[i], "--ack")) {
        p->output_ack = 1;
        i++;
    }

    if (i >= argc) {
        help(progname);
    }

    if (!get_mode(argv[i], &p->mode)) {
        fprintf(stderr, "Invalid <mode> argument.\n\n");
        help(progname);
    }

    i++;
    if (i >= argc) {
        help(progname);
    }

    if (!get_reason(argv[i], &p->reason)) {
        fprintf(stderr, "Invalid <reason> argument.\n\n");
        help(progname);
    }

    i++;
    if (i < argc) {
        if (get_data(argv[i], &p->data)) {
            i++;
        } else {
            fprintf(stderr, "Invalid <data> argument.\n\n");
            help(progname);
        }
    }

    if (i != argc) {
        fprintf(stderr, "Too many arguments.\n\n");
        help(progname);
    }
}

int main(int argc, char *argv[]) {
    struct yield_request request;
    int res = 0;
    struct parsed_args args;

    parse_args(argc, argv, &args);

    request.tohost = (uint64_t) HTIF_DEVICE_YIELD << 56 |
                     (args.mode << 56 >> 8) |
                     (args.reason << 48 >> 16) |
                     (args.data << 32 >> 32);
    request.fromhost = 0;
    if ((res = send_request(&request)))
        return res;

    if (args.output_ack)
        fprintf(stdout, "0x%" PRIx64 "\n", request.fromhost);

    return 0;
}
