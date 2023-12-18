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
#include "libcmt/rollup.h"
#include "libcmt/abi.h"
#include "libcmt/keccak.h"
#include "libcmt/merkle.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Voucher(address,bytes)
#define VOUCHER CMT_ABI_FUNSEL(0xef, 0x61, 0x5e, 0x2f)

// Notice(bytes)
#define NOTICE CMT_ABI_FUNSEL(0xc2, 0x58, 0xd6, 0xe5)

// EvmAdvance(address,uint256,uint256,uint256,bytes)
#define EVM_ADVANCE CMT_ABI_FUNSEL(0xd2, 0x0c, 0x60, 0xb4)

// EvmInspect(bytes)
#define EVM_INSPECT CMT_ABI_FUNSEL(0x73, 0xd4, 0x41, 0x43)

#define DBG(X) debug(X, #X, __FILE__, __LINE__)
static int debug(int rc, const char *expr, const char *file, int line) {
    static bool checked = false, enabled = false;

    if (rc == 0)
        return 0;

    if (!checked) {
        enabled = getenv("CMT_DEBUG") != NULL;
        checked = true;
    }
    if (enabled)
        fprintf(stderr, "%s:%d Error %s on `%s'\n", file, line, expr, strerror(-rc));
    return rc;
}

int cmt_rollup_init(cmt_rollup_t *me) {
    if (!me)
        return -EINVAL;

    int rc = DBG(cmt_io_init(me->io));
    if (rc)
        return rc;

    cmt_merkle_init(me->merkle);
    return 0;
}

void cmt_rollup_fini(cmt_rollup_t *me) {
    if (!me)
        return;

    cmt_io_fini(me->io);
    cmt_merkle_fini(me->merkle);
}

int cmt_rollup_emit_voucher(cmt_rollup_t *me, uint8_t address[20], size_t length, const void *data) {
    if (!me)
        return -EINVAL;
    if (!data && length)
        return -EINVAL;

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    cmt_buf_t of[1];
    void *params_base = tx->begin + 4; // after funsel

    if (DBG(cmt_abi_put_funsel(wr, VOUCHER)) || DBG(cmt_abi_put_address(wr, address)) ||
        DBG(cmt_abi_put_bytes_s(wr, of)) || DBG(cmt_abi_put_bytes_d(wr, of, length, data, params_base)))
        return -ENOBUFS;

    size_t m = wr->begin - tx->begin;
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_AUTOMATIC,
        .reason = CMT_IO_REASON_TX_OUTPUT,
        .data = m,
    }};
    int rc = DBG(cmt_io_yield(me->io, req));
    if (rc)
        return rc;
    return cmt_merkle_push_back_data(me->merkle, m, tx->begin);
}

int cmt_rollup_emit_notice(cmt_rollup_t *me, size_t length, const void *data) {
    if (!me)
        return -EINVAL;
    if (!data && length)
        return -EINVAL;

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    cmt_buf_t of[1];
    void *params_base = tx->begin + 4; // after funsel

    if (DBG(cmt_abi_put_funsel(wr, NOTICE)) || DBG(cmt_abi_put_bytes_s(wr, of)) ||
        DBG(cmt_abi_put_bytes_d(wr, of, length, data, params_base)))
        return -ENOBUFS;

    size_t m = wr->begin - tx->begin;
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_AUTOMATIC,
        .reason = CMT_IO_REASON_TX_OUTPUT,
        .data = m,
    }};
    int rc = DBG(cmt_io_yield(me->io, req));
    if (rc)
        return rc;
    return cmt_merkle_push_back_data(me->merkle, m, tx->begin);
}

int cmt_rollup_emit_report(cmt_rollup_t *me, size_t length, const void *data) {
    if (!me)
        return -EINVAL;
    if (!data && length)
        return -EINVAL;

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    if (cmt_buf_split(tx, length, wr, tx))
        return -ENOBUFS;

    memcpy(wr->begin, data, length);
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_AUTOMATIC,
        .reason = CMT_IO_REASON_TX_REPORT,
        .data = length,
    }};
    return DBG(cmt_io_yield(me->io, req));
}

int cmt_rollup_emit_exception(cmt_rollup_t *me, size_t length, const void *data) {
    if (!me)
        return -EINVAL;
    if (!data && length)
        return -EINVAL;

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    if (cmt_buf_split(tx, length, wr, tx))
        return -ENOBUFS;

    memcpy(tx->begin, data, length);
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_MANUAL,
        .reason = CMT_IO_REASON_TX_EXCEPTION,
        .data = length,
    }};
    return DBG(cmt_io_yield(me->io, req));
}

int cmt_rollup_read_advance_state(cmt_rollup_t *me, cmt_rollup_advance_t *advance) {
    if (!me)
        return -EINVAL;
    if (!advance)
        return -EINVAL;

    cmt_buf_t rd[1] = {cmt_io_get_rx(me->io)};
    cmt_buf_t st[1] = {{rd->begin + 4, rd->end}}; // EVM offsets are from after funsel
    cmt_buf_t of[1];

    size_t length = 0;
    if (DBG(cmt_abi_check_funsel(rd, EVM_ADVANCE)) || DBG(cmt_abi_get_address(rd, advance->sender)) ||
        DBG(cmt_abi_get_uint(rd, sizeof(advance->block_number), &advance->block_number)) ||
        DBG(cmt_abi_get_uint(rd, sizeof(advance->block_timestamp), &advance->block_timestamp)) ||
        DBG(cmt_abi_get_uint(rd, sizeof(advance->index), &advance->index)) || DBG(cmt_abi_get_bytes_s(rd, of)) ||
        DBG(cmt_abi_get_bytes_d(st, of, &length, &advance->data)))
        return -ENOBUFS;
    advance->length = length;
    return 0;
}

int cmt_rollup_read_inspect_state(cmt_rollup_t *me, cmt_rollup_inspect_t *inspect) {
    if (!me)
        return -EINVAL;
    if (!inspect)
        return -EINVAL;

    cmt_buf_t rd[1] = {cmt_io_get_rx(me->io)};
    cmt_buf_t st[1] = {{rd->begin + 4, rd->end}}; // EVM offsets are from after funsel
    cmt_buf_t of[1];

    size_t length = 0;
    if (DBG(cmt_abi_check_funsel(rd, EVM_INSPECT)) || DBG(cmt_abi_get_bytes_s(rd, of)) ||
        DBG(cmt_abi_get_bytes_d(st, of, &length, &inspect->data)))
        return -ENOBUFS;
    inspect->length = length;
    return 0;
}

static int accepted(union cmt_io_driver *io, uint32_t *n) {
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_MANUAL,
        .reason = CMT_IO_REASON_RX_ACCEPTED,
        .data = *n,
    }};
    int rc = DBG(cmt_io_yield(io, req));
    if (rc)
        return rc;

    *n = req->data;
    return req->reason;
}

static int revert(union cmt_io_driver *io) {
    struct cmt_io_yield req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_MANUAL,
        .reason = CMT_IO_REASON_RX_REJECTED,
        .data = 0,
    }};
    return DBG(cmt_io_yield(io, req));
}

int cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish) {
    if (!me)
        return -EINVAL;
    if (!finish)
        return -EINVAL;

    if (!finish->accept_previous_request) {
        revert(me->io);
        /* revert should not return! */
        return -EBADE;
    }

    cmt_merkle_get_root_hash(me->merkle, cmt_io_get_tx(me->io).begin);
    finish->next_request_payload_length = CMT_WORD_LENGTH;
    finish->next_request_type = accepted(me->io, &finish->next_request_payload_length);
    cmt_merkle_init(me->merkle);
    return finish->next_request_type;
}

int cmt_rollup_progress(cmt_rollup_t *me, uint32_t progress) {
    cmt_io_yield_t req[1] = {{
        .dev = CMT_IO_DEV,
        .cmd = CMT_IO_CMD_AUTOMATIC,
        .reason = CMT_IO_REASON_PROGRESS,
        .data = progress,
    }};
    return DBG(cmt_io_yield(me->io, req));
}

int cmt_rollup_load_merkle(cmt_rollup_t *me, const char *path) {
    return DBG(cmt_merkle_load(me->merkle, path));
}

int cmt_rollup_save_merkle(cmt_rollup_t *me, const char *path) {
    return DBG(cmt_merkle_save(me->merkle, path));
}
