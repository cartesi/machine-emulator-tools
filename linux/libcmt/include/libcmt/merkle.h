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
 * @defgroup libcmt_merkle merkle
 * merkle tree of keccak256 hashes
 *
 * @ingroup libcmt
 * @{ */
#ifndef EVM_MERKLE_H
#define EVM_MERKLE_H
#include"keccak.h"

enum {
	CMT_MERKLE_MAX_DEPTH = 64, /**< merkle tree height */
};

/** Opaque Merkle tree state.
 * initialize with: @ref cmt_merkle_init */
typedef struct {
	uint64_t leaf_count;
	uint8_t state[CMT_MERKLE_MAX_DEPTH][CMT_KECCAK_LENGTH];
	const uint8_t (*zero)[CMT_KECCAK_LENGTH];
} cmt_merkle_t;

/** Initialize a @ref cmt_merkle_t tree state.
 *
 * @param [in] me    uninitialized state */
void cmt_merkle_init(cmt_merkle_t *me);

/** Finalize a @ref cmt_merkle_t tree state.
 *
 * @param [in] me    initialized state
 *
 * @note use of @p me after this call is undefined behavior. */
void cmt_merkle_fini(cmt_merkle_t *me);

/** Load the a @ref cmt_merkle_t tree from a memory blob @p data, with length @p length.
 *
 * @param [in] me    uninitialized state
 * @param [in] length size in bytes of @p data
 * @param [in] data array of bytes
 * @return
 * - 0 on success */
int cmt_merkle_state_load(cmt_merkle_t *me, size_t length, const void *data);

/** Save the a @ref cmt_merkle_t tree to a  @p data, with length @p length.
 *
 * @param [in] me     uninitialized state
 * @param [in] length size of @p data in bytes
 * @param [in] data   array of bytes
 * @return
 * - 0 on success */
int cmt_merkle_state_save(cmt_merkle_t *me, size_t length, void *data);

/** Size in bytes required by merkle state save
 *
 * @param [in] me     uninitialized state
 * @param [in] length size of @p data in bytes
 * @param [in] data   array of bytes
 * @return
 * - size of the array required by @ref cmt_merkle_state_save */
size_t cmt_merkle_state_get_length_in_bytes(cmt_merkle_t *me);

/** Append a leaf node
 *
 * @param [in,out] me initialized state
 * @param [in] hash   value of the new leaf
 * @return
 * - 0        success
 * - -ENOBUFS indicates the tree is full */
int cmt_merkle_push_back(cmt_merkle_t *me, uint8_t hash[CMT_KECCAK_LENGTH]);

/** Compute the keccak-256 hash of @p data and append it as a leaf node
 *
 * @param [in,out] me     initialized state
 * @param [in]     length size of @p data in bytes
 * @param [in]     data   array of bytes
 * @return
 * - 0        success
 * - -ENOBUFS indicates that the tree is full */
int cmt_merkle_push_back_data(cmt_merkle_t *me, size_t length, const void *data);

/** Retrieve the root hash of the merkle tree
 *
 * @param [in]  me   initialized state
 * @param [out] root root hash of the merkle tree */
void cmt_merkle_get_root_hash(cmt_merkle_t *me, uint8_t root[CMT_KECCAK_LENGTH]);

#endif /* CMT_MERKLE_H */
