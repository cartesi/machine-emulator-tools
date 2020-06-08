/* Copyright 2019 Cartesi Pte. Ltd.
 *
 * This file is part of the machine-emulator. The machine-emulator is free
 * software: you can redistribute it and/or modify it under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * The machine-emulator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the machine-emulator. If not, see http://www.gnu.org/licenses/.
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
        "Usage: %s [--ack] <command> <data>\n"
        "Where: \n"
        "  <command>    \"progress\" or \"rollup\"\n"
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

int main(int argc, char *argv[]) {
    struct yield_request request;
    int i, ack_output, res;
    uint64_t cmd, data;
    const char* progname = argv[0];
    
    i = 1;
    if (i >= argc)
        help(progname);

    ack_output = 0;
    if (!strcmp(argv[i], "--ack")) {
        ack_output = 1;
        i++;
    }

    if (i >= argc)
        help(progname);

    if (!strcmp(argv[i], "progress"))
        cmd = HTIF_YIELD_PROGRESS;
    else if (!strcmp(argv[i], "rollup")) 
        cmd = HTIF_YIELD_ROLLUP;
    else
        help(progname);
    

    i++;
    if (i >= argc) 
        help(progname);

    if (sscanf(argv[i], "%" SCNu64, &data) != 1)
        help(progname);

    i++;
    if (i != argc) 
        help(progname);

    request.tohost = (cmd << 48) | (data << 16 >> 16);
    request.fromhost = 0;
    if ((res = send_request(&request)))
        return res;

    if (ack_output)
        fprintf(stdout, "0x%" PRIx64 "\n", request.fromhost);

    return 0;
}
