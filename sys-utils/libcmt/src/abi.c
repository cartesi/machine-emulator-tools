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
#include "libcmt/abi.h"

#include <errno.h>
#include <string.h>

static uintptr_t align_forward(uintptr_t p, size_t a) {
    return (p + (a - 1)) & ~(a - 1);
}

uint32_t cmt_abi_funsel(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return CMT_ABI_FUNSEL(a, b, c, d);
}

int cmt_abi_mark_frame(const cmt_buf_t *me, cmt_buf_t *frame) {
    if (!me || !frame) {
        return -EINVAL;
    }
    *frame = *me;
    return 0;
}

int cmt_abi_put_funsel(cmt_buf_t *me, uint32_t funsel) {
    cmt_buf_t x[1];
    int rc = cmt_buf_split(me, sizeof(funsel), x, me);
    if (rc) {
        return rc;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(x->begin, &funsel, sizeof(funsel));
    return 0;
}

int cmt_abi_encode_uint_nr(size_t n, const uint8_t *data, uint8_t out[CMT_ABI_U256_LENGTH]) {
    if (n > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    for (size_t i = 0; i < n; ++i) {
        out[CMT_ABI_U256_LENGTH - 1 - i] = data[i];
    }
    for (size_t i = n; i < CMT_ABI_U256_LENGTH; ++i) {
        out[CMT_ABI_U256_LENGTH - 1 - i] = 0;
    }
    return 0;
}

int cmt_abi_encode_uint_nn(size_t n, const uint8_t *data, uint8_t out[CMT_ABI_U256_LENGTH]) {
    if (n > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    for (size_t i = 0; i < CMT_ABI_U256_LENGTH - n; ++i) {
        out[i] = 0;
    }
    for (size_t i = CMT_ABI_U256_LENGTH - n; i < CMT_ABI_U256_LENGTH; ++i) {
        out[i] = data[i - CMT_ABI_U256_LENGTH + n];
    }
    return 0;
}

int cmt_abi_encode_uint(size_t n, const void *data, uint8_t out[CMT_ABI_U256_LENGTH]) {
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    return cmt_abi_encode_uint_nn(n, data, out);
#else
    return cmt_abi_encode_uint_nr(n, data, out);
#endif
}

int cmt_abi_decode_uint_nr(const uint8_t data[CMT_ABI_U256_LENGTH], size_t n, uint8_t *out) {
    if (n > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    for (size_t i = 0; i < CMT_ABI_U256_LENGTH - n; ++i) {
        if (data[i]) {
            return -EDOM;
        }
    }
    for (size_t i = CMT_ABI_U256_LENGTH - n; i < CMT_ABI_U256_LENGTH; ++i) {
        out[CMT_ABI_U256_LENGTH - 1 - i] = data[i];
    }
    return 0;
}

int cmt_abi_decode_uint_nn(const uint8_t data[CMT_ABI_U256_LENGTH], size_t n, uint8_t *out) {
    if (n > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    for (size_t i = 0; i < CMT_ABI_U256_LENGTH - n; ++i) {
        if (data[i]) {
            return -EDOM;
        }
    }
    for (size_t i = CMT_ABI_U256_LENGTH - n; i < CMT_ABI_U256_LENGTH; ++i) {
        out[i - CMT_ABI_U256_LENGTH + n] = data[i];
    }
    return 0;
}

int cmt_abi_decode_uint(const uint8_t data[CMT_ABI_U256_LENGTH], size_t n, uint8_t *out) {
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    return cmt_abi_decode_uint_nn(data, n, out);
#else
    return cmt_abi_decode_uint_nr(data, n, out);
#endif
}

int cmt_abi_put_uint(cmt_buf_t *me, size_t data_length, const void *data) {
    cmt_buf_t x[1];
    if (data_length > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    if (cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me)) {
        return -ENOBUFS;
    }
    return cmt_abi_encode_uint(data_length, data, x->begin);
}

int cmt_abi_put_uint_be(cmt_buf_t *me, size_t data_length, const void *data) {
    cmt_buf_t x[1];
    if (data_length > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    if (cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me)) {
        return -ENOBUFS;
    }
    return cmt_abi_encode_uint_nn(data_length, data, x->begin);
}
int cmt_abi_put_uint256(cmt_buf_t *me, const cmt_abi_u256_t *value) {
    cmt_buf_t x[1];
    if (cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me)) {
        return -ENOBUFS;
    }
    return cmt_abi_encode_uint_nn(sizeof(*value), value->data, x->begin);
}

int cmt_abi_put_bool(cmt_buf_t *me, bool value) {
    uint8_t boolean = !!value;
    return cmt_abi_put_uint(me, sizeof(boolean), &boolean);
}

int cmt_abi_put_address(cmt_buf_t *me, const cmt_abi_address_t *address) {
    cmt_buf_t x[1];
    if (cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me)) {
        return -ENOBUFS;
    }
    return cmt_abi_encode_uint_nn(sizeof(*address), address->data, x->begin);
}

int cmt_abi_put_bytes_s(cmt_buf_t *me, cmt_buf_t *offset) {
    return cmt_buf_split(me, CMT_ABI_U256_LENGTH, offset, me);
}

int cmt_abi_reserve_bytes_d(cmt_buf_t *me, cmt_buf_t *of, size_t n, cmt_buf_t *out, const void *start) {
    int rc = 0;
    cmt_buf_t tmp[1];
    cmt_buf_t sz[1];
    size_t n32 = align_forward(n, CMT_ABI_U256_LENGTH);

    rc = cmt_buf_split(me, CMT_ABI_U256_LENGTH, sz, tmp);
    if (rc) {
        return rc;
    }
    rc = cmt_buf_split(tmp, n32, out, tmp);
    if (rc) {
        return rc;
    }

    size_t offset = sz->begin - (uint8_t *) start;
    rc = cmt_abi_encode_uint(sizeof(offset), &offset, of->begin);
    if (rc) {
        return rc;
    }
    rc = cmt_abi_encode_uint(sizeof(n), &n, sz->begin);
    if (rc) {
        return rc;
    }

    *me = *tmp; // commit the buffer changes
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memset(out->begin + n, 0, n32 - n); // zero out the padding
    return 0;
}

int cmt_abi_put_bytes_d(cmt_buf_t *me, cmt_buf_t *offset, const cmt_buf_t *frame, const cmt_abi_bytes_t *payload) {
    cmt_buf_t res[1];
    int rc = cmt_abi_reserve_bytes_d(me, offset, payload->length, res, frame->begin);
    if (rc) {
        return rc;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(res->begin, payload->data, payload->length);
    return 0;
}

uint32_t cmt_abi_peek_funsel(cmt_buf_t *me) {
    if (cmt_buf_length(me) < 4) {
        return 0;
    }
    return CMT_ABI_FUNSEL(me->begin[0], me->begin[1], me->begin[2], me->begin[3]);
}

int cmt_abi_check_funsel(cmt_buf_t *me, uint32_t expected) {
    if (cmt_buf_length(me) < 4) {
        return -ENOBUFS;
    }

    if (cmt_abi_peek_funsel(me) != expected) {
        return -EBADMSG;
    }

    me->begin += 4;
    return 0;
}

int cmt_abi_get_uint(cmt_buf_t *me, size_t n, void *data) {
    cmt_buf_t x[1];

    if (n > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    int rc = cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me);
    if (rc) {
        return rc;
    }

    return cmt_abi_decode_uint(x->begin, n, data);
}

int cmt_abi_get_uint_be(cmt_buf_t *me, size_t n, void *data) {
    cmt_buf_t x[1];

    if (n > CMT_ABI_U256_LENGTH) {
        return -EDOM;
    }
    int rc = cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me);
    if (rc) {
        return rc;
    }

    return cmt_abi_decode_uint_nn(x->begin, n, data);
}

int cmt_abi_get_bool(cmt_buf_t *me, bool *value) {
    bool boolean = 0;
    int rc = cmt_abi_get_uint(me, sizeof(boolean), &boolean);
    if (rc) {
        return rc;
    }
    *value = boolean;
    return 0;
}

int cmt_abi_get_address(cmt_buf_t *me, cmt_abi_address_t *address) {
    cmt_buf_t x[1];

    int rc = cmt_buf_split(me, CMT_ABI_U256_LENGTH, x, me);
    if (rc) {
        return rc;
    }
    return cmt_abi_decode_uint_nn(x->begin, sizeof(*address), address->data);
}

int cmt_abi_get_bytes_s(cmt_buf_t *me, cmt_buf_t of[1]) {
    return cmt_buf_split(me, CMT_ABI_U256_LENGTH, of, me);
}

int cmt_abi_peek_bytes_d(const cmt_buf_t *start, cmt_buf_t of[1], cmt_buf_t *bytes) {
    int rc = 0;
    uint64_t offset = 0;
    uint64_t size = 0;
    rc = cmt_abi_get_uint(of, sizeof(offset), &offset);
    if (rc) {
        return rc;
    }

    /* from the beginning, after funsel */
    cmt_buf_t it[1] = {{start->begin + offset, start->end}};
    rc = cmt_abi_get_uint(it, sizeof(size), &size);
    if (rc) {
        return rc;
    }
    return cmt_buf_split(it, size, bytes, it);
}

int cmt_abi_get_bytes_d(const cmt_buf_t *start, cmt_buf_t of[1], size_t *n, void **data) {
    cmt_buf_t bytes[1];
    int rc = cmt_abi_peek_bytes_d(start, of, bytes);
    if (rc) {
        return rc;
    }
    *n = cmt_buf_length(bytes);
    *data = bytes->begin;
    return 0;
}
