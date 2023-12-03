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
        "Usage: %s <mode> <reason> [<data>]\n"
        "Where: \n"
        "  <mode>       \"manual\" or \"automatic\"\n"
        "  <reason>     \"progress\", \"rx-accepted\", \"rx-rejected\",\n"
        "               \"tx-voucher\", \"tx-notice\", \"tx-exception\" or\n"
        "               \"tx-report\"\n"
        "  <data>       32-bit unsigned integer (decimal, default 0)\n",
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

struct name_value {
    const char *name;
    uint64_t   value;
};
static int find_value(const struct name_value *pairs, size_t n, const char *s, uint64_t *value)
{
    for (size_t i=0; i<n; ++i) {
        if (strcmp(s, pairs[i].name) == 0) {
            *value = pairs[i].value;
            return 1;
        }
    }
    return 0;
}

int get_mode(const char *s, uint64_t *mode) {
    struct name_value modes[] = {
        {"manual",    HTIF_YIELD_MANUAL},
        {"automatic", HTIF_YIELD_AUTOMATIC},
    };
    return find_value(modes, sizeof(modes)/sizeof(*modes), s, mode);
}

int get_reason(const char *s, uint64_t *reason) {
    struct name_value reasons[] = {
        {"progress",     HTIF_YIELD_REASON_PROGRESS},
        {"rx-accepted",  HTIF_YIELD_REASON_RX_ACCEPTED},
        {"rx-rejected",  HTIF_YIELD_REASON_RX_REJECTED},
        {"tx-exception", HTIF_YIELD_REASON_TX_EXCEPTION},
        {"tx-notice",    HTIF_YIELD_REASON_TX_NOTICE},
        {"tx-report",    HTIF_YIELD_REASON_TX_REPORT},
        {"tx-voucher",   HTIF_YIELD_REASON_TX_VOUCHER},
    };
    return find_value(reasons, sizeof(reasons)/sizeof(*reasons), s, reason);
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

    request.dev = HTIF_DEVICE_YIELD;
    request.cmd = args.mode;
    request.reason = args.reason;
    request.data = args.data;
    if ((res = send_request(&request)))
        return res;

    return 0;
}
