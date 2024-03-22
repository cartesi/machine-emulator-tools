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
 * @defgroup libcmt_io_driver Cartesi Machine Input/Output Driver
 * Low level abstraction of the Cartesi Machine kernel driver.
 *
 * Interaction is as follows:
 * - Open the kernel device and mmap the shared memory ranges (tx and rx) with:
 *   @ref cmt_io_init.
 * - Retrieve the memory regions from the library with: @ref cmt_io_get_tx and
 *   @ref cmt_io_get_rx.
 * - Do requests (next input, emit output ...) by yielding the control back to
 *   the emulator with: @ref cmt_io_yield.
 *
 * Lets look at some code:
 *
 * @include examples/io.c
 *
 * There are two implementations provided: `ioctl` that interacts with the
 * cartesi-machine linux driver and `mock` that reads and writes the host
 * filesystem, used for testing.
 *
 * @section Environment variables
 *
 * this module exposes two environment variables: @p CMT_DEBUG and @p CMT_INPUTS.
 *
 * @p CMT_DEBUG prints runtime information during application execution.
 *
 * ```
 * CMT_DEBUG=yes ./application
 * ```
 *
 * @p CMT_INPUTS feeds inputs to the mock (ignored for ioctl).
 *
 * ```
 * CMT_INPUTS=0:advance.bin,1:inspect.bin ./application
 * ```
 *
 * @ingroup libcmt
 * @{ */
#ifndef CMT_IO_H
#define CMT_IO_H
#include "buf.h"
#include <stddef.h>
#include <stdint.h>

/** Device */
enum {
    HTIF_DEVICE_YIELD = 2, /**< Yield device */
};

/** Commands */
enum {
    HTIF_YIELD_CMD_AUTOMATIC = 0, /**< Automatic yield */
    HTIF_YIELD_CMD_MANUAL = 1,    /**< Manual yield */
};

/** Automatic reasons */
enum {
    HTIF_YIELD_AUTOMATIC_REASON_PROGRESS = 1,     /**< Progress */
    HTIF_YIELD_AUTOMATIC_REASON_TX_OUTPUT = 2,    /**< emit an output */
    HTIF_YIELD_AUTOMATIC_REASON_TX_REPORT = 4,    /**< emit a report */
};

/** Manual reasons */
enum {
    HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED = 1,     /**< Accept and load next input */
    HTIF_YIELD_MANUAL_REASON_RX_REJECTED = 2,     /**< Reject and revert */
    HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION = 4,    /**< emit a exception and halt execution */
};

/** Reply reason when requesting @ref HTIF_YIELD_REASON_RX_ACCEPTED or HTIF_YIELD_REASON_RX_REJECTED */
enum {
    HTIF_YIELD_REASON_ADVANCE = 0, /**< Input is advance */
    HTIF_YIELD_REASON_INSPECT = 1, /**< Input is inspect */
};

typedef struct {
    cmt_buf_t tx[1];
    cmt_buf_t rx[1];
    uint32_t rx_max_length;
    uint32_t rx_fromhost_length;
    int fd;
} cmt_io_driver_ioctl_t;

typedef struct {
    cmt_buf_t tx[1];
    cmt_buf_t rx[1];
    cmt_buf_t inputs_left;

    int input_type;
    char input_filename[128];
    char input_fileext[16];

    int input_seq;
    int output_seq;
    int report_seq;
    int exception_seq;
} cmt_io_driver_mock_t;

/** Implementation specific cmio state. */
typedef union cmt_io_driver {
    cmt_io_driver_ioctl_t ioctl;
    cmt_io_driver_mock_t mock;
} cmt_io_driver_t;

/** yield struct cmt_io_yield */
typedef struct cmt_io_yield {
    uint8_t dev;
    uint8_t cmd;
    uint16_t reason;
    uint32_t data;
} cmt_io_yield_t;

/** Open the io device and initialize the driver. Release its resources with @ref cmt_io_fini.
 *
 * @param [in] me A uninitialized @ref cmt_io_driver state
 * @returns
 * - 0 on success
 * - negative errno code on error */
int cmt_io_init(cmt_io_driver_t *me);

/** Release the driver resources and close the io device.
 *
 * @param [in] me A sucessfuly initialized state by @ref cmt_io_init
 * @note usage of @p me after this call is a BUG and will cause undefined behaviour */
void cmt_io_fini(cmt_io_driver_t *me);

/** Retrieve the transmit buffer @p tx
 *
 * @param [in] me A sucessfuly initialized state by @ref cmt_io_init
 * @return
 * - writable memory region (check @ref cmt_buf_t)
 * @note memory is valid until @ref cmt_io_fini is called. */
cmt_buf_t cmt_io_get_tx(cmt_io_driver_t *me);

/** Retrieve the receive buffer @p rx
 *
 * @param [in] me A sucessfuly initialized state by @ref cmt_io_init
 * @return
 * - readable memory region (check @ref cmt_buf_t)
 * @note memory is valid until @ref cmt_io_fini is called. */
cmt_buf_t cmt_io_get_rx(cmt_io_driver_t *me);

/** Perform the yield encoded in @p rr.
 *
 * @param [in] me A sucessfuly initialized state by @ref cmt_io_init
 * @param [in,out] rr Request and Reply
 * @return
 * - 0 on success
 * - negative errno code on error */
int cmt_io_yield(cmt_io_driver_t *me, cmt_io_yield_t *rr);

#endif /* CMT_IO_H */

