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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/types.h>
#include <linux/cartesi/dehash.h>

#define DEHASH_DEVICE_NAME "/dev/dehash"
#define HASH_STRING_MAX_LENGTH ((CARTESI_HASH_SIZE * 2 ) + 2)
#define BLOCK_MAX_SIZE (1024*1024)

struct options {
    char *hash_str;
    uint64_t max_size;
    uint8_t info_only;
    uint8_t csv;
};

static void print_help(const char *progname) {
    fprintf(stderr,
        "Usage: %s [-i] [-s=n] <hash>\n"
        "Where: \n"
        "  -i           Print device settings. Hash and size parameters will be\n"
        "               ignored. No block content will be written to stdout.\n\n"
        "  -c           Print device settings as CSV. Only works with '-i'.\n"
        "               Order: Dev Addr, Dev Length, Target Addr, Target Length\n\n"
        "  -s=n         Set max block size equal n bytes. Blocks larger than n\n"
        "               will result in \"Not found\". When ommited, size will\n"
        "               be inferred from block metadata.\n\n"
        "  <hash>       Hexadecimal string starting with \"0x\" followed by\n"
        "               up to 64 characters. (E.g.: 0xabcdef0123456789)\n"
        "\n",
        progname);
}

static int execute_query(struct dehash_query* pquery) {
    int fd = open(DEHASH_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening device " DEHASH_DEVICE_NAME "\n");
        return fd;
    }

    int res = ioctl(fd, IOCTL_DEHASH_QUERY, pquery);
    if (res != 0)
        fprintf(stderr, "Device returned with error %s\n", strerror(errno));

    close(fd);
    return res;
}

static int get_info(struct dehash_info *info) {
    int fd = open(DEHASH_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening device " DEHASH_DEVICE_NAME "\n");
        return fd;
    }

    int res = ioctl(fd, IOCTL_DEHASH_INFO, info);
    if (res != 0)
        fprintf(stderr, "Device returned with error %s\n", strerror(errno));

    close(fd);
    return res;
}

static int parse_hash(unsigned char *hash, uint64_t *hash_length, const char *str) {
    size_t len = strnlen(str, HASH_STRING_MAX_LENGTH + 1);
    if (len <= 2 || len > HASH_STRING_MAX_LENGTH || len & 1)
        return -EINVAL;

    if (str[0] != '0' || (str[1] != 'x' && str[1] != 'X'))
        return -EINVAL;

    const char *pstr = str + 2;
    unsigned char byte = 0, end = 0;
    size_t i = 0, max = (len - 2) >> 1;
    while (*pstr && i < max) {
        if (sscanf(pstr, "%2hhx%hhn", &byte, &end) != 1 || end != 2)
            return -EINVAL;
        hash[i++] = byte;
        pstr += 2;
    }
    *hash_length = max;
    return 0;
}

static int write_result(struct dehash_query *pquery) {
    if (pquery->data_length == CARTESI_HASH_NOT_FOUND)
        return -ENOENT;
    size_t bytes = fwrite(pquery->data, pquery->data_length, 1, stdout);
    if (bytes < pquery->data_length)
        return -EIO;
    return 0;
}

static void print_info(struct dehash_info *info, uint8_t csv) {
    if (csv) {
        printf("0x%" PRIX64 ",%" PRIu64 ",0x%" PRIX64 ",%" PRIu64 "\n",
                info->device_address, info->device_length,
                info->target_address, info->target_length);
    } else {
        printf("Dehash device address: 0x%" PRIX64 "\n", info->device_address);
        printf("Dehash device length: %" PRIu64 "\n", info->device_length);
        printf("Dehash device TARGET address: 0x%" PRIX64 "\n", info->target_address);
        printf("Dehash device TARGET length: %" PRIu64 "\n", info->target_length);
    }
}

static int parse_options(int argc, char *argv[], struct options *opts) {
    if (argc < 2 || argc > 3)
        return -EINVAL;

    char mul[2];
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-')
            break;
        if (argv[i][1] == 'i') {
            opts->info_only = 1;
            continue;
        }
        if (argv[i][1] == 'c') {
            opts->csv = 1;
            continue;
        }
        if (argv[i][1] == 's') {
            if (sscanf(&argv[i][3], "%" SCNu64 "%[km]", &opts->max_size, mul) < 1)
                return -1;
            if (mul[0] == 'k')
                opts->max_size *= 1024;
            else if (mul[0] == 'm')
                opts->max_size *= 1024*1024;
            continue;
        }
        return -EINVAL;
    }

    opts->hash_str = argv[argc - 1];
    return 0;
}

int main(int argc, char *argv[]) {
    struct options opts = { .max_size = CARTESI_HASH_NOT_FOUND, .info_only = 0, .csv = 0 };
    struct dehash_query query;
    struct dehash_info info;
    const char* progname = argv[0];
    int ret;

    if ((ret = parse_options(argc, argv, &opts))) {
        print_help(progname);
        return ret;
    }

    if (opts.info_only || opts.max_size == CARTESI_HASH_NOT_FOUND) {
        if ((ret = get_info(&info)))
            return ret;

        /* if limit was not set, use info to set it */
        if (opts.max_size == CARTESI_HASH_NOT_FOUND)
            opts.max_size = info.target_length;
    }

    if (opts.info_only) {
        print_info(&info, opts.csv);
        return 0;
    } else {
        if ((ret = parse_hash(query.hash, &query.hash_length, opts.hash_str)))
            return ret;

        query.data_length = opts.max_size;
        query.data = (unsigned char *)calloc(opts.max_size, sizeof(unsigned char));
        if (!query.data)
            return -ENOMEM;

        if ((ret = execute_query(&query)))
            goto cleanup;

        ret = write_result(&query);
cleanup:
        free(query.data);
        return ret;
    }

}
