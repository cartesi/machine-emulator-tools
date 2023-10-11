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
 * @defgroup yield_driver yield_driver
 * Abstraction of the Cartesi yield driver IOCTL API
 *
 * Lets look at some code:
 * - Define a yield struct to store the state.
 * - retrieve a floating point number from the user.
 * - initialize, yield the value, then finalize the state.
 *
 * @include examples/yield-driver.c
 *
 * when executed:
 *
 * @code
 * $ yield-driver --progress=100
 * Progress: 100.00
 * @endcode
 *
 * @ingroup driver
 * @{ */
#ifndef CMT_YIELD_DRIVER_API_H
#define CMT_YIELD_DRIVER_API_H
#include<stdint.h>

/** contents of @ref rollup_driver_t are implementation specific.
 * Define it in @p <yield.h> */
typedef struct cmt_yield_driver cmt_yield_driver_t;

/** Open the rollup device and initialize the driver
 *
 * @param [in] me A uninitialized @ref cmt_yield_driver state
 * @returns
 * - 0 on success
 * - negative value on error. errno values from `open`. */
int cmt_yield_driver_init(struct cmt_yield_driver *me);

/** Close the rollup device and un-initialize the driver
 *
 * @param [in] me A initialized @ref cmt_yield_driver state (with: @ref cmt_yield_driver_init)
 * @note usage of @p me after this call is a BUG and will cause undefined behaviour */
void cmt_yield_driver_fini(struct cmt_yield_driver *me);

/** Report the application progress to the emulator.
 *
 * @param [in] me A sucessfuly initialized state by @ref yield_driver_init
 * @returns
 * - 0 on success
 * - negative value on error. errno values from `ioctl`. */
int cmt_yield_driver_progress(struct cmt_yield_driver *me, uint32_t progress);

#endif /* CMT_YIELD_DRIVER_API_H */
