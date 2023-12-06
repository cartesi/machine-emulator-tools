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
 * Takes care of @ref rollup_driver interactions.
 *
 * @include examples/rollup.c
 *
 * @ingroup libcmt
 * @{ */
#ifndef CMT_ROLLUP_H
#define CMT_ROLLUP_H
#include "abi.h"
#include "buf.h"
#include "merkle.h"
#include "rollup-driver.h"

/** Opaque rollup state, initialize with:
 * - @ref cmt_rollup_init */
typedef struct {
	struct cmt_rollup_driver rollup_driver[1];
	cmt_buf_t tx[1],
	          rx[1];
	cmt_merkle_t merkle[1];
} cmt_rollup_t;

/** Public struct with the advance state contents */
typedef struct {
	uint8_t  sender[CMT_ADDRESS_LENGTH]; /**< the address of the input sender */
	uint64_t block_number;               /**< block number of this input */
	uint64_t block_timestamp;            /**< block timestamp of this input UNIX epoch format) */
	uint64_t index;                      /**< input index (in relation to all inputs ever sent to the DApp) */
	size_t   length;                     /**< length in bytes of the data field */
	void    *data;                       /**< advance contents */
} cmt_rollup_advance_t;

/** Public struct with the inspect state contents */
typedef struct {
	size_t length; /**< length in bytes of the data field */
	void  *data;   /**< inspect contents */
} cmt_rollup_inspect_t;

/** Public struct with the finish state contents */
typedef struct {
	bool accept_previous_request;
	int next_request_type;
	size_t next_request_payload_length;
} cmt_rollup_finish_t;

/** Initialize a @ref cmt_rollup_t state.
 *
 * @param [in] me    uninitialized state
 *
 * - 0 success
 * - negative value on error. values from: @ref cmt_rollup_driver_init */
int
cmt_rollup_init(cmt_rollup_t *me);

/** Finalize a @ref cmt_rollup_t statate previously initialized with @ref
 * cmt_rollup_init
 *
 * @param [in] me    initialized state
 *
 * @note use of @p me after this call is undefined behavior. */
void
cmt_rollup_fini(cmt_rollup_t *me);

/** Emit a voucher
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     address destination
 * @param [in]     n       sizeof @p data in bytes
 * @param [in]     data    message contents
 * @return
 * - 0 success
 * - -ENOBUFS no space left in @p me */
int
cmt_rollup_emit_voucher(cmt_rollup_t *me
                       ,uint8_t address[CMT_ADDRESS_LENGTH]
                       ,size_t n
                       ,const void *data);

/** Emit a notice
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     n       sizeof @p data in bytes
 * @param [in]     data    message contents
 * @return
 * - 0 success
 * - -ENOBUFS no space left in @p me */
int
cmt_rollup_emit_notice(cmt_rollup_t *me
                      ,size_t n
                      ,const void *data);

/** Emit a report
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     n       sizeof @p data in bytes
 * @param [in]     data    message contents
 * @return
 * - 0 success
 * - -ENOBUFS no space left in @p me */
int
cmt_rollup_emit_report(cmt_rollup_t *me
                      ,size_t n
                      ,const void *data);

/** Emit a exception
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in]     n       sizeof @p data in bytes
 * @param [in]     data    message contents
 * @return
 * - 0 success
 * - -ENOBUFS no space left in @p me */
int
cmt_rollup_emit_exception(cmt_rollup_t *me
                         ,size_t n
                         ,const void *data);

/** read advance state
 *
 * @return
 * - 0 success
 * - negative value on error. */
int
cmt_rollup_read_advance_state(cmt_rollup_t *me
                             ,cmt_rollup_advance_t *advance);

/** read inspect state
 *
 * @return
 * - 0 success
 * - negative value on error. */
int
cmt_rollup_read_inspect_state(cmt_rollup_t *me
                             ,cmt_rollup_inspect_t *inspect);

/* Finishes processing of current advance or inspect.
 * Waits for and returns the next advance or inspect query when available.
 *
 * @param [in,out] me      initialized cmt_rollup_t instance
 * @param [in,out] finish  initialized cmt_rollup_finish_t instance
 *
 * @return
 * - 0 success
 * - negative value on error. Values from either @ref
 *   cmt_rollup_driver_accept_and_wait_next_input or @ref
 *   cmt_rollup_driver_revert.
 */
int
cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish);

#endif /* CMT_ROLLUP_H */
