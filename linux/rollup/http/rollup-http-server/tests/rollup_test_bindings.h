// Copyright (C) 2022 Cartesi Pte. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use
// this file except in compliance with the License. You may obtain a copy of the
// License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef _UAPI_LINUX_CARTESI_ROLLUP_H
#define _UAPI_LINUX_CARTESI_ROLLUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <stdbool.h>


#define CARTESI_ROLLUP_ADVANCE_STATE 0
#define CARTESI_ROLLUP_INSPECT_STATE 1

#define CARTESI_ROLLUP_ADDRESS_SIZE 20

struct rollup_bytes {
    unsigned char *data;
    uint64_t length;
};

struct rollup_input_metadata {
    uint8_t msg_sender[CARTESI_ROLLUP_ADDRESS_SIZE];
    uint64_t block_number;
    uint64_t timestamp;
    uint64_t epoch_index;
    uint64_t input_index;
};

struct rollup_advance_state {
    struct rollup_input_metadata metadata;
    struct rollup_bytes payload;
};

struct rollup_inspect_state {
    struct rollup_bytes payload;
};

struct rollup_finish {
    /* True if previous request should be accepted */
    /* False if previous request should be rejected */
    bool accept_previous_request;

    int next_request_type; /* either CARTESI_ROLLUP_ADVANCE or CARTESI_ROLLUP_INSPECT */
    int next_request_payload_length;
};

struct rollup_voucher {
    uint8_t destination[CARTESI_ROLLUP_ADDRESS_SIZE];
    struct rollup_bytes payload;
    uint64_t index;
};

struct rollup_notice {
    struct rollup_bytes payload;
    uint64_t index;
};

struct rollup_report {
    struct rollup_bytes payload;
};

struct rollup_exception {
    struct rollup_bytes payload;
};

/* Finishes processing of current advance or inspect.
 * Returns only when next advance input or inspect query is ready.
 * How:
 *   Yields manual with rx-accepted if accept is true and yields manual with rx-rejected if accept is false.
 *   Once yield returns, checks the data field in fromhost to decide if next request is advance or inspect.
 *   Returns type and payload length of next request in struct
 * Returns 0 */
#define IOCTL_ROLLUP_FINISH  _IOWR(0xd3, 0, struct rollup_finish)

/* Obtains arguments to advance state
 * How:
 *   Reads from input metadat memory range and convert data.
 *   Reads from rx buffer and copy to payload
 * Returns 0 */
#define IOCTL_ROLLUP_READ_ADVANCE_STATE _IOWR(0xd3, 0, struct rollup_advance_state)

/* Obtains arguments to inspect state
 * How:
 *   Reads from rx buffer and copy to payload
 * Returns 0 */
#define IOCTL_ROLLUP_READ_INSPECT_STATE _IOWR(0xd3, 0, struct rollup_inspect_state)

/* Outputs a new voucher.
 * How: Computes the Keccak-256 hash of address+payload and then, atomically:
 *  - Copies the (address+be32(0x40)+be32(payload_length)+payload) to the tx buffer
 *  - Copies the hash to the next available slot in the voucher-hashes memory range
 *  - Yields automatic with tx-voucher
 *  - Fills in the index field with the corresponding slot from voucher-hashes
 * Returns 0 */
#define IOCTL_ROLLUP_WRITE_VOUCHER _IOWR(0xd3, 1, struct rollup_voucher)

/* Outputs a new notice.
 * How: Computes the Keccak-256 hash of payload and then, atomically:
 *  - Copies the (be32(0x20)+be32(payload_length)+payload) to the tx buffer
 *  - Copies the hash to the next available slot in the notice-hashes memory range
 *  - Yields automatic with tx-notice
 *  - Fills in the index field with the corresponding slot from notice-hashes
 * Returns 0 */
#define IOCTL_ROLLUP_WRITE_NOTICE  _IOWR(0xd3, 2, struct rollup_notice)

/* Outputs a new report.
 *  - Copies the (be32(0x20)+be32(payload_length)+payload) to the tx buffer
 *  - Yields automatic with tx-report
 * Returns 0 */
#define IOCTL_ROLLUP_WRITE_REPORT  _IOWR(0xd3, 3, struct rollup_report)

/* Throws an exeption.
 *  - Copies the (be32(0x20)+be32(payload_length)+payload) to the tx buffer
 *  - Yields manual with tx-exception
 * Returns 0 */
#define IOCTL_ROLLUP_THROW_EXCEPTION  _IOWR(0xd3, 4, struct rollup_exception)
#endif


int rollup_finish_request(int fd, struct rollup_finish *finish, bool accept);
int rollup_read_advance_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *bytes, struct rollup_input_metadata *metadata);
int rollup_read_inspect_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *query);
int rollup_write_voucher(int fd, uint8_t address[CARTESI_ROLLUP_ADDRESS_SIZE], struct rollup_bytes *bytes, uint64_t* voucher_index);
int rollup_write_notice(int fd, struct rollup_bytes *bytes, uint64_t* notice_index);
int rollup_write_report(int fd, struct rollup_bytes *bytes);
int rollup_throw_exception(int fd, struct rollup_bytes *bytes);
