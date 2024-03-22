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
#include "abi.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

/** Declare a cmt_buf_t with stack backed memory.
 * @param [in] N - size in bytes
 * @note don't port */
#define CMT_BUF_DECL(S, L)                                                                                             \
    cmt_buf_t S[1] = {{                                                                                                \
        .begin = (uint8_t[L]){0},                                                                                      \
        .end = (S)->begin + L,                                                                                         \
    }}

/** Declare a cmt_buf_t with parameters backed memory.
 * @param [in] L - size in bytes
 * @note don't port */
#define CMT_BUF_DECL3(S, L, P)                                                                                         \
    cmt_buf_t S[1] = {{                                                                                                \
        .begin = P,                                                                                                    \
        .end = (S)->begin + L,                                                                                         \
    }}

static void encode_u8() {
    uint8_t x = 0x01;
    uint8_t en[CMT_WORD_LENGTH];
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
    };
    cmt_abi_encode_uint(sizeof(x), (void *) &x, en);
    assert(memcmp(en, be, sizeof(be)) == 0);
}

static void encode_u16() {
    uint16_t x = UINT16_C(0x0123);
    uint8_t en[CMT_WORD_LENGTH];
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
    };
    cmt_abi_encode_uint(sizeof(x), (void *) &x, en);
    assert(memcmp(en, be, sizeof(be)) == 0);
}

static void encode_u32() {
    uint32_t x = UINT32_C(0x01234567);
    uint8_t en[CMT_WORD_LENGTH];
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
    };
    cmt_abi_encode_uint(sizeof(x), (void *) &x, en);
    assert(memcmp(en, be, sizeof(be)) == 0);
}

static void encode_u64() {
    uint64_t x = UINT64_C(0x0123456789abcdef);
    uint8_t en[CMT_WORD_LENGTH];
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
    };
    cmt_abi_encode_uint(sizeof(x), (void *) &x, en);
    assert(memcmp(en, be, sizeof(be)) == 0);
}

static void encode_u256() {
    uint8_t x[CMT_WORD_LENGTH] = {
        0x1f,
        0x1e,
        0x1d,
        0x1c,
        0x1b,
        0x1a,
        0x19,
        0x18,
        0x17,
        0x16,
        0x15,
        0x14,
        0x13,
        0x12,
        0x11,
        0x10,
        0x0f,
        0x0e,
        0x0d,
        0x0c,
        0x0b,
        0x0a,
        0x09,
        0x08,
        0x07,
        0x06,
        0x05,
        0x04,
        0x03,
        0x02,
        0x01,
        0x00,
    };
    uint8_t en[CMT_WORD_LENGTH];
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0x0a,
        0x0b,
        0x0c,
        0x0d,
        0x0e,
        0x0f,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1a,
        0x1b,
        0x1c,
        0x1d,
        0x1e,
        0x1f,
    };
    cmt_abi_encode_uint(sizeof(x), (void *) &x, en);
    assert(memcmp(en, be, sizeof(be)) == 0);
}

static void encode_edom() {
    uint8_t x[CMT_WORD_LENGTH + 1] = {
        0x00,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0x0a,
        0x0b,
        0x0c,
        0x0d,
        0x0e,
        0x0f,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1a,
        0x1b,
        0x1c,
        0x1d,
        0x1e,
        0x1f,
        0x20,
    };
    uint8_t en[CMT_WORD_LENGTH];
    assert(cmt_abi_encode_uint_nr(sizeof(x), x, en) == EDOM);
    assert(cmt_abi_encode_uint_nn(sizeof(x), x, en) == EDOM);
}

static void decode_u8() {
    uint8_t x, ex = 0x01;
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
    };
    assert(cmt_abi_decode_uint(be, sizeof(x), (void *) &x) == 0);
    assert(x == ex);
}

static void decode_u16() {
    uint16_t x, ex = UINT16_C(0x0123);
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
    };
    assert(cmt_abi_decode_uint(be, sizeof(x), (void *) &x) == 0);
    assert(x == ex);
}

static void decode_u32() {
    uint32_t x, ex = UINT32_C(0x01234567);
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
    };
    assert(cmt_abi_decode_uint(be, sizeof(x), (void *) &x) == 0);
    assert(x == ex);
}

static void decode_u64() {
    uint64_t x, ex = UINT64_C(0x0123456789abcdef);
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
    };
    assert(cmt_abi_decode_uint(be, sizeof(x), (void *) &x) == 0);
    assert(x == ex);
}

static void decode_u256() {
    uint8_t x[CMT_WORD_LENGTH],
        ex[CMT_WORD_LENGTH] = {
            0x1f,
            0x1e,
            0x1d,
            0x1c,
            0x1b,
            0x1a,
            0x19,
            0x18,
            0x17,
            0x16,
            0x15,
            0x14,
            0x13,
            0x12,
            0x11,
            0x10,
            0x0f,
            0x0e,
            0x0d,
            0x0c,
            0x0b,
            0x0a,
            0x09,
            0x08,
            0x07,
            0x06,
            0x05,
            0x04,
            0x03,
            0x02,
            0x01,
            0x00,
        };
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0x0a,
        0x0b,
        0x0c,
        0x0d,
        0x0e,
        0x0f,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1a,
        0x1b,
        0x1c,
        0x1d,
        0x1e,
        0x1f,
    };
    assert(cmt_abi_decode_uint(be, sizeof(x), x) == 0);
    assert(memcmp(x, ex, sizeof(ex)) == 0);
}

/* encoded value can't be represented in a uint64_t. */
static void decode_edom() {
    uint64_t x;
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xAA,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
    };
    assert(cmt_abi_decode_uint(be, sizeof(x), (void *) &x) == -EDOM);
}

static void put_funsel() {
    uint8_t data[] = {0xcd, 0xcd, 0x77, 0xc0};
    uint32_t funsel = CMT_ABI_FUNSEL(data[0], data[1], data[2], data[3]);
    CMT_BUF_DECL(b, 64);
    cmt_buf_t it[1] = {*b};

    assert(cmt_abi_put_funsel(it, funsel) == 0);
    assert(memcmp(b->begin, data, 4) == 0);
}

static void put_uint() {
    uint64_t x = UINT64_C(0x0123456789abcdef);
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
    };
    CMT_BUF_DECL(b, 64);
    cmt_buf_t it[1] = {*b};

    assert(cmt_abi_put_uint(it, sizeof(x), &x) == 0);
    assert(memcmp(b->begin, be, sizeof(be)) == 0);
}

static void put_address() {
    uint8_t x[20] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0x01, 0x23, 0x45, 0x67};
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
        0x01,
        0x23,
        0x45,
        0x67,
    };
    CMT_BUF_DECL(b, 64);
    cmt_buf_t it[1] = {*b};

    assert(cmt_abi_put_address(it, x) == 0);
    assert(memcmp(b->begin, be, sizeof(be)) == 0);
}

static void put_bytes() {
    uint64_t x = UINT64_C(0x0123456789abcdef);
    uint8_t be[] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x20,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x08,
        0xef,
        0xcd,
        0xab,
        0x89,
        0x67,
        0x45,
        0x23,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };
    CMT_BUF_DECL(b, 128);
    cmt_buf_t it[1] = {*b}, of[1];

    assert(cmt_abi_put_bytes_s(it, of) == 0);
    assert(cmt_abi_put_bytes_d(it, of, sizeof(x), &x, b->begin) == 0);
    assert(memcmp(b->begin, be, sizeof(be)) == 0);
}

static void get_funsel() {
    CMT_BUF_DECL(b, 64);
    cmt_buf_t wr[1] = {*b}, rd[1] = {*b};
    uint32_t funsel = CMT_ABI_FUNSEL(1, 2, 3, 4);

    assert(cmt_abi_put_funsel(wr, funsel) == 0);
    assert(cmt_abi_peek_funsel(rd) == funsel);
    assert(cmt_abi_check_funsel(rd, funsel) == 0);
}

static void get_uint() {
    uint64_t x, ex = UINT64_C(0x0123456789abcdef);
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
    };
    CMT_BUF_DECL3(b, sizeof(be), be);
    cmt_buf_t rd[1] = {*b};

    assert(cmt_abi_get_uint(rd, sizeof(x), &x) == 0);
    assert(x == ex);
}

static void get_address() {
    uint8_t x[20];
    uint8_t ex[20] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0x01, 0x23, 0x45, 0x67};
    uint8_t be[CMT_WORD_LENGTH] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
        0x01,
        0x23,
        0x45,
        0x67,
        0x89,
        0xab,
        0xcd,
        0xef,
        0x01,
        0x23,
        0x45,
        0x67,
    };
    CMT_BUF_DECL3(b, sizeof(be), be);
    cmt_buf_t it[1] = {*b};

    assert(cmt_abi_get_address(it, x) == 0);
    assert(memcmp(x, ex, sizeof(ex)) == 0);
}

static void get_bytes() {
    uint64_t ex = UINT64_C(0x0123456789abcdef);
    uint8_t be[] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x20,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x08,
        0xef,
        0xcd,
        0xab,
        0x89,
        0x67,
        0x45,
        0x23,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };
    CMT_BUF_DECL3(b, sizeof(be), be);
    cmt_buf_t it[1] = {*b}, of[1], bytes[1];

    assert(cmt_abi_get_bytes_s(it, of) == 0);
    assert(cmt_abi_peek_bytes_d(b, of, bytes) == 0);
    assert(memcmp(bytes->begin, &ex, sizeof(ex)) == 0);
}

int main() {
    encode_u8();
    encode_u16();
    encode_u32();
    encode_u64();
    encode_u256();
    encode_edom();

    decode_u8();
    decode_u16();
    decode_u32();
    decode_u64();
    decode_u256();
    decode_edom();

    put_funsel();
    put_uint();
    put_address();
    put_bytes();

    get_funsel();
    get_uint();
    get_address();
    get_bytes();

    return 0;
}
