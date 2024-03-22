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
/** @file
 * @defgroup libcmt_keccak keccak
 * Keccak 256 digest
 *
 * Data can be inserted in pieces:
 * @code
 * ...
 * uint8_t h[CMT_KECCAK_LENGTH];
 * cmt_keccak_t st[1];
 *
 * cmt_keccak_init(st);
 * cmt_keccak_update(st, 1, "h");
 * cmt_keccak_update(st, 1, "e");
 * cmt_keccak_update(st, 1, "l");
 * cmt_keccak_update(st, 1, "l");
 * cmt_keccak_update(st, 1, "o");
 * cmt_keccak_final(st, h);
 * ...
 * @endcode
 *
 * all at once:
 * @code
 * ...
 * const char data[] = "hello";
 * uint8_t h[CMT_KECCAK_LENGTH];
 * cmt_keccak_data(h, sizeof(data)-1, data);
 * ...
 * @endcode
 *
 * or with a specialized call to generate @ref funsel data:
 * @code
 * ...
 * uint32_t funsel = cmt_keccak_funsel("FunctionCall(address,bytes)");
 * ...
 * @endcode
 *
 * Code based on https://github.com/mjosaarinen/tiny_sha3 with the sha3 to
 * keccak change as indicated by this comment:
 * https://github.com/mjosaarinen/tiny_sha3#updated-07-aug-15
 *
 * @ingroup libcmtcmt
 * @{ */
#ifndef CMT_KECCAK_H
#define CMT_KECCAK_H
#include <stddef.h>
#include <stdint.h>

enum {
    CMT_KECCAK_LENGTH = 32, /**< Bytes in the hash digest */
};

/** Opaque internal keccak state */
typedef union cmt_keccak_state {
    uint8_t b[200];
    uint64_t q[25];
} cmt_keccak_state_t;

/** Opaque Keccak state, used to do hash computations, initialize with:
 * - @ref cmt_keccak_init
 * - @ref CMT_KECCAK_INIT
 * - @ref CMT_KECCAK_DECL */
typedef struct cmt_keccak {
    cmt_keccak_state_t st;
    int pt, rsiz;
} cmt_keccak_t;

/** Initialize a @ref cmt_keccak_t hasher state.
 *
 * @param [out] state uninitialized @ref cmt_keccak_t */
void cmt_keccak_init(cmt_keccak_t *state);

/** Hash @b n bytes of @b data
 *
 * @param [in,out] state  initialize the hasher state
 * @param [in]     length bytes in @b data to process
 * @param [in]     data   data to hash */
void cmt_keccak_update(cmt_keccak_t *state, size_t n, const void *data);

/** Finalize the hash calculation from @b state and store it in @b md
 *
 * @param [in]  state initialize the hasher state (with all data already added to it)
 * @param [out] md    32bytes to store the computed hash */
void cmt_keccak_final(cmt_keccak_t *state, void *md);

/** Hash all @b n bytes of @b data at once
 *
 * @param [in]  length bytes in @b data to process
 * @param [in]  data   data to hash
 * @param [out] md     32bytes to store the computed hash
 * @return pointer to @b md
 *
 * Equivalent to:
 * @code
 * cmt_keccak_t st = CMT_KECCAK_INIT(&st);
 * cmt_keccak_update(&st, n, data);
 * cmt_keccak_final(&st, md);
 * return md;
 * @endcode */
uint8_t *cmt_keccak_data(size_t length, const void *data, uint8_t md[CMT_KECCAK_LENGTH]);

/** Compute the function selector from the solidity declaration @p decl
 *
 * @param [in]  decl   solidity call declaration, without variable names
 * @param [out] funsel function selector as described by @ref funsel
 * @return A @p funsel value as if defined by @ref CMT_ABI_FUNSEL
 *
 * Example usage:
 * @code
 * ...
 * uint32_t funsel = cmt_keccak_funsel("FunctionCall(address,bytes)");
 * ...
 * @endcode
 */
uint32_t cmt_keccak_funsel(const char *decl);
#endif /* CMT_KECCAK_H */
/** $@} */

