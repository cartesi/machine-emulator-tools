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
 * @defgroup libcmt_rollup rollup
 * Rollup abstraction layer
 *
 * Takes care of @ref libcmt_io_driver interactions, @ref libcmt_abi
 * encoding/decoding and @ref libcmt_merkle tree handling.
 *
 * Mocked version has support for simulating I/O via environment variables:
 * @p CMT_INPUTS="0:input.bin,..." and verbose output with @p CMT_DEBUG=yes.
 *
 * Lets look at some code:
 *
 * @include doc/examples/rollup.c
 *
 * @ingroup libcmt
 * @{ */
#ifndef CMT_ROLLUP_H
#define CMT_ROLLUP_H
#include "abi.h"
#include "io.h"
#include "merkle.h"

typedef struct cmt_rollup {
    union cmt_io_driver io[1];
    uint32_t fromhost_data;
    cmt_merkle_t merkle[1];
} cmt_rollup_t;

/** Public struct with the advance state contents */
typedef struct cmt_rollup_advance {
    uint64_t chain_id;                        /**< network */
    cmt_abi_address_t app_contract;           /**< application contract address */
    cmt_abi_address_t msg_sender;             /**< input sender address */
    uint64_t block_number;                    /**< block number of this input */
    uint64_t block_timestamp;                 /**< block timestamp of this input UNIX epoch format) */
    cmt_abi_u256_t prev_randao;               /**< The latest RANDAO mix of the post beacon state of the previous block */
    uint64_t index;                           /**< input index (in relation to all inputs ever sent to the DApp) */
    cmt_abi_bytes_t payload;                  /**< payload for this input */
} cmt_rollup_advance_t;

/** Public struct with the inspect state contents */
typedef struct cmt_rollup_inspect {
    cmt_abi_bytes_t payload; /**< payload for this input */
} cmt_rollup_inspect_t;

/** Public struct with the finish state contents */
typedef struct cmt_rollup_finish {
    bool accept_previous_request;
    int next_request_type;
    uint32_t next_request_payload_length;
} cmt_rollup_finish_t;

/** Public struct with generic io request/response */
typedef struct cmt_gio {
    uint16_t domain;               /**< domain for the gio request */
    uint32_t id_length;            /**< length of id */
    void *id;                      /**< id for the request */
    uint16_t response_code;        /**< response */
    uint32_t response_data_length; /**< length of response data */
    void *response_data;           /**< response data */
} cmt_gio_t;

/** Initialize a @ref cmt_rollup_t state.
 *
 * @param [in] me uninitialized state
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_init(cmt_rollup_t *me);

/** Finalize a @ref cmt_rollup_t state previously initialized with @ref
 * cmt_rollup_init
 *
 * @param [in] me    initialized state
 *
 * @note use of @p me after this call is undefined behavior. */
void cmt_rollup_fini(cmt_rollup_t *me);

/** Emit a voucher
 *
 * Equivalent to the `Voucher(address,uint256,bytes)` solidity call.
 *
 * @param [in,out] me             initialized @ref cmt_rollup_t instance
 * @param [in]     address        destination data
 * @param [in]     value          value data
 * @param [in]     data           message contents
 * @param [out]    index          index of emitted voucher, if successful
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_emit_voucher(cmt_rollup_t *me, const cmt_abi_address_t *address, const cmt_abi_u256_t *value, const cmt_abi_bytes_t *data, uint64_t *index);

/** Emit a delegate call voucher
 *
 * Equivalent to the `DelegateCallVoucher(address,bytes)` solidity call.
 *
 * @param [in,out] me             initialized @ref cmt_rollup_t instance
 * @param [in]     address        destination data
 * @param [in]     data           message contents
 * @param [out]    index          index of emitted voucher, if successful
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_emit_delegate_call_voucher(cmt_rollup_t *me, const cmt_abi_address_t *address, const cmt_abi_bytes_t *data, uint64_t *index);

/** Emit a notice
 *
 * @param [in,out] me          initialized cmt_rollup_t instance
 * @param [in]     data        message contents
 * @param [out]    index       index of emitted notice, if successful
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_emit_notice(cmt_rollup_t *me, const cmt_abi_bytes_t *payload, uint64_t *index);

/** Emit a report
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     n       sizeof @p data in bytes
 * @param [in]     data    message contents
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_emit_report(cmt_rollup_t *me, const cmt_abi_bytes_t *payload);

/** Emit a exception
 * @param [in,out] me          initialized cmt_rollup_t instance
 * @param [in]     data_length data length in bytes
 * @param [in]     data        message contents
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_emit_exception(cmt_rollup_t *me, const cmt_abi_bytes_t *payload);

/** Report progress
 *
 * @param [in,out] me        initialized cmt_rollup_t instance
 * @param [in]     progress  progress value to be set
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_progress(cmt_rollup_t *me, uint32_t progress);

/** Read advance state
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [out]    advance cmt_rollup_advance_t instance (may be uninitialized)
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_read_advance_state(cmt_rollup_t *me, cmt_rollup_advance_t *advance);

/** Read inspect state
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [out]    inspect cmt_rollup_inspect_t instance (may be uninitialized)
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_read_inspect_state(cmt_rollup_t *me, cmt_rollup_inspect_t *inspect);

/** Finish processing of current advance or inspect.
 * Waits for and returns the next advance or inspect query when available.
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in,out] finish  initialized cmt_rollup_finish_t instance
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish);

/** Performs a generic IO request
 *
 * @param [in,out] me  initialized cmt_rollup_t instance
 * @param [in,out] req initialized cmt_gio_t structure
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_gio_request(cmt_rollup_t *me, cmt_gio_t *req);

/** Retrieve the merkle tree and intermediate state from a file @p path
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     file    path to file (parent directories must exist)
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_load_merkle(cmt_rollup_t *me, const char *path);

/** Store the merkle tree and intermediate state to a file @p path
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     file    path to file (parent directories must exist)
 *
 * @return
 * |   |                             |
 * |--:|-----------------------------|
 * |  0| success                     |
 * |< 0| failure with a -errno value | */
int cmt_rollup_save_merkle(cmt_rollup_t *me, const char *path);

/** Resets the merkle tree to pristine conditions
 *
 * @param [in,out] me      initialized cmt_rollup_t instance */
void cmt_rollup_reset_merkle(cmt_rollup_t *me);

#endif /* CMT_ROLLUP_H */
