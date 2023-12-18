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

/* The Load/Store file format is as follows:
 *
 * | length | offset | contents   |
 * | ------ | ------ | ---------- |
 * |      8 |      0 | leaf_count |
 * |     32 |      8 | state[0]   |
 * |     32 |     40 | state[1]   |
 * |     32 |    ... | state[...] |
 * |     32 |   2024 | state[63]  |
 */

#include "libcmt/merkle.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

extern const uint8_t cmt_merkle_table[][CMT_KECCAK_LENGTH];

static void hash2(const uint8_t lhs[CMT_KECCAK_LENGTH], const uint8_t rhs[CMT_KECCAK_LENGTH],
    uint8_t out[CMT_KECCAK_LENGTH]) {
    cmt_keccak_t c[1];
    cmt_keccak_init(c);
    cmt_keccak_update(c, CMT_KECCAK_LENGTH, lhs);
    cmt_keccak_update(c, CMT_KECCAK_LENGTH, rhs);
    cmt_keccak_final(c, out);
}

void cmt_merkle_init(cmt_merkle_t *me) {
    me->leaf_count = 0;
    me->zero = cmt_merkle_table;
    memcpy(me->state, me->zero, sizeof(me->state));
}

void cmt_merkle_fini(cmt_merkle_t *me) {
    // no-op
    (void) me;
}

static int read_whole_file(const char *name, size_t max, void *data, size_t *length) {
    int rc = 0;

    FILE *file = fopen(name, "rb");
    if (!file)
        return -errno;

    size_t size = fread(data, 1, max, file);
    if (getc(file) != EOF)
        rc = -ENOBUFS;
    else if (length)
        *length = size;
    fclose(file);
    return rc;
}

int cmt_merkle_load(cmt_merkle_t *me, const char *name) {
    return read_whole_file(name, cmt_merkle_max_length(), me, NULL);
    me->zero = cmt_merkle_table;

    return 0;
}

static int write_whole_file(const char *name, size_t length, const void *data) {
    int rc = 0;

    FILE *file = fopen(name, "wb");
    if (!file)
        return -errno;

    if (fwrite(data, 1, length, file) != length)
        rc = -errno;
    fclose(file);
    return rc;
}

int cmt_merkle_save(cmt_merkle_t *me, const char *name) {
    return write_whole_file(name, cmt_merkle_max_length(), me);
}

size_t cmt_merkle_max_length() {
    return sizeof((cmt_merkle_t *) 0)->leaf_count + sizeof((cmt_merkle_t *) 0)->state;
}

int cmt_merkle_push_back(cmt_merkle_t *me, uint8_t hash[CMT_KECCAK_LENGTH]) {
    if (me->leaf_count == UINT64_MAX)
        return -ENOBUFS;

    unsigned n = (uint64_t) ffsll(++me->leaf_count) - 1u;
    for (unsigned i = 0; i < n; ++i)
        hash2(me->state[i], hash, hash);
    memcpy(me->state[n], hash, CMT_KECCAK_LENGTH);
    return 0;
}

void cmt_merkle_get_root_hash(cmt_merkle_t *me, uint8_t root[CMT_KECCAK_LENGTH]) {
    /* n is bound by CMT_MERKLE_MAX_DEPTH-1u */
    unsigned n = ((uint64_t) ffsll(me->leaf_count) - 1u) & (CMT_MERKLE_MAX_DEPTH - 1u);
    memcpy(root, me->state[n], CMT_KECCAK_LENGTH);

    for (unsigned i = n; i < CMT_MERKLE_MAX_DEPTH; ++i) {
        bool set = me->leaf_count & (UINT64_C(1) << i);
        if (set)
            hash2(me->state[i], root, root);
        else
            hash2(root, me->zero[i], root);
    }
}

int cmt_merkle_push_back_data(cmt_merkle_t *me, size_t n, const void *data) {
    uint8_t hash[CMT_KECCAK_LENGTH];
    cmt_keccak_t c[1];
    cmt_keccak_init(c);
    cmt_keccak_update(c, n, data);
    cmt_keccak_final(c, hash);
    return cmt_merkle_push_back(me, hash);
}
