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
 * @defgroup rollup_driver rollup_driver
 * Low level abstraction of the Cartesi Machine rollup kernel driver.
 *
 * Lets look at some code:
 *
 * @include examples/rollup-driver.c
 *
 * @ingroup driver
 * @{ */
#ifndef ROLLUP_DRIVER_API_H
#define ROLLUP_DRIVER_API_H
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

enum {
	CMT_ROLLUP_ADVANCE_STATE =  0, /**< @ref rollup_driver_accept_and_wait_next_input advance state */
	CMT_ROLLUP_INSPECT_STATE =  1, /**< @ref rollup_driver_accept_and_wait_next_input inspect state */
};

/** contents of @ref cmt_rollup_driver are implementation specific.
 * Define it in @p <rollup.h> */
typedef struct cmt_rollup_driver cmt_rollup_driver_t;

/** Open the rollup device and initialize the driver
 *
 * @param [in] me A uninitialized @ref cmt_rollup_driver state
 * @returns
 * - 0 on success
 * - negative value on error. Any of -ENOBUFS, or errno values from `open` and `ioctl`. */
int cmt_rollup_driver_init(struct cmt_rollup_driver *me);

/** Release the driver resources and close the rollup device
 *
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @note usage of @p me after this call is a BUG and will cause undefined behaviour */
void cmt_rollup_driver_fini(struct cmt_rollup_driver *me);

/** Create a output, with the first @p length bytes of @ref cmt_rollup_driver_t.tx
 *
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @param [in] length size in bytes of the output, contents from @p tx buffer.
 * @return
 * - 0 on success
 * - negative value on error. -ENOBUFS, or errno values from `ioctl` */
int cmt_rollup_driver_write_output(struct cmt_rollup_driver *me, uint64_t length);

/** Create a report, with the first @p length bytes of @ref cmt_rollup_driver.tx
 *
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @param [in] length size in bytes of the output, contents from @p tx buffer.
 * @return
 * - 0 on success
 * - negative value on error. -ENOBUFS, or errno values from `ioctl` */
int cmt_rollup_driver_write_report(struct cmt_rollup_driver *me, uint64_t length);

/** Create a exception, with the first @p length bytes of @ref cmt_rollup_driver tx
 *
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @param [in] length size in bytes of the message, contents from @p tx buffer.
 * @return
 * - 0 on success
 * - negative value on error. -ENOBUFS, or -errno values from `ioctl` */
int cmt_rollup_driver_write_exception(struct cmt_rollup_driver *me, uint64_t length);

/** Accept this input, wait for the next one.
 *
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @param [out] length size in bytes of the newly received input.
 * @return
 * - ROLLUP_ADVANCE_STATE On advancing state
 * - ROLLUP_INSPECT_STATE On inspecting state
 * - negative value on error.
 *
 * @note This call expects the tx buffer to have the merkle root hash of the
 * outputs. It as part of its interface. */
int cmt_rollup_driver_accept_and_wait_next_input(struct cmt_rollup_driver *me, size_t *length);

/** Revert state back to how it was the last time @ref
 * rollup_driver_accept_and_wait_next_input got called and reject this input
 *
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @return
 * - 0 on success
 * - negative value on error. errno values from `ioctl` */
int cmt_rollup_driver_revert(struct cmt_rollup_driver *me);

/** Retrieve the writable memory region @p tx
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @param [out] max size of the region in bytes (optional).
 *
 * @return
 * - pointer to the buffer.
 * @note memory is valid until @ref rollup_driver_fini is called. */
void *cmt_rollup_driver_get_tx(struct cmt_rollup_driver *me, size_t *length);

/** Retrieve the writable memory region @p rx
 * @param [in] me A sucessfuly initialized state by @ref rollup_driver_init
 * @param [out] max size of the region in bytes (optional).
 *
 * @return
 * - pointer to the buffer.
 * @note memory is valid until @ref rollup_driver_fini is called. */
void *cmt_rollup_driver_get_rx(struct cmt_rollup_driver *me, size_t *length);

#endif /* ROLLUP_API_H */
/** @} */
