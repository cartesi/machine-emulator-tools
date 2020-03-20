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

#define HTIF_BASE_ADDR 0x40008000
#define HTIF_SIZE 16

#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void help(const char *name) {
    fprintf(stderr, "Usage:\n"
        "  %s <action> [<action> ...]\n"
        "where <action> is:\n"
        "  --help\n"
        "  --no-debug\n"
        "  --debug\n"
        "  --progress-server <fifo>\n"
        "  --halt <exit-code>\n"
        "  --yield <cmd> <data>\n"
        "  --console <cmd> <data>\n", name);
    exit(1);
}

static void write_tohost(volatile uint64_t *base, uint64_t val) {
    base[0] = val;
}

static uint64_t read_tohost(volatile uint64_t *base) {
    return base[0];
}

static void write_fromhost(volatile uint64_t *base, uint64_t val) {
    base[1] = val;
}

static uint64_t read_fromhost(volatile uint64_t *base) {
    return base[1];
}

static uint64_t htif(volatile uint64_t *base,
    uint64_t dev, uint64_t cmd, uint64_t data) {
    write_fromhost(base, 0);
    dev = dev << 56;
    cmd = cmd << 56 >> 8;
    data = data << 16 >> 16;
    write_tohost(base, dev | cmd | data);
    return read_fromhost(base);
}

int main(int argc, char *argv[]) {
    int memfd = open("/dev/mem", O_RDWR);
    volatile uint64_t *base = (volatile uint64_t *) mmap(0, HTIF_SIZE,
        PROT_WRITE | PROT_READ, MAP_SHARED, memfd, HTIF_BASE_ADDR);
    uint64_t dev = 0;
    uint64_t cmd = 0;
    uint64_t data = 1;
    uint64_t ack = 0;
    int end = 0;
    int debug = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--halt")) {
            i++;
            if (i < argc && sscanf(argv[i], "%" SCNu64 "%n", &data, &end) == 1
                && !argv[i][end]) {
                data = (data << 1) + 1;
                dev = 0;
                cmd = 0;
                htif(base, dev, cmd, data);
            } else {
                help(argv[0]);
            }
        } else if (!strcmp(argv[i], "--debug")) {
            debug = 1;
        } else if (!strcmp(argv[i], "--no-debug")) {
            debug = 0;
        } else if (!strcmp(argv[i], "--progress-server")) {
            dev = 2;
            cmd = 0;
            i++;
            if (i >= argc) {
                help(argv[0]);
            }
            while (1) {
                FILE *fp = fopen(argv[i], "r");
                if (!fp) {
                    fprintf(stderr, "Unable to open '%s' for reading\n");
                    exit(1);
                }
                setvbuf(fp, NULL, _IOLBF, 1024);
                while (1) {
                    if (fscanf(fp, "%" SCNu64, &data) == 1) {
                        ack = htif(base, dev, cmd, data);
                    } else {
                        int c = ' ';
                        while (c != EOF && c != '\n')
                            c = fgetc(fp);
                        if (c == EOF) break;
                        else ungetc(c, fp);
                    }
                }
                fclose(fp);
            }
        } else if (!strcmp(argv[i], "--yield")) {
            dev = 2;
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &cmd, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &data, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            ack = htif(base, dev, cmd, data);
            if (debug) {
                printf("ack: %016" PRIx64 "\n", ack);
            }
        } else if (!strcmp(argv[i], "--console")) {
            dev = 1;
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &cmd, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &data, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            htif(base, dev, cmd, data);
            if (debug) {
                printf("ack: %016" PRIx64 "\n", ack);
            }
        } else if (!strcmp(argv[i], "--generic")) {
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &dev, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &cmd, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            i++;
            if (i >= argc ||
                sscanf(argv[i], "%" SCNu64 "%n", &data, &end) != 1 ||
                argv[i][end]) {
                help(argv[0]);
            }
            ack = htif(base, dev, cmd, data);
            if (debug) {
                printf("ack: %016" PRIx64 "\n", ack);
            }
        } else {
            help(argv[0]);
        }
    }
    return 0;
}
