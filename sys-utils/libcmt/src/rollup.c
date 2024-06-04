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
#include "libcmt/merkle.h"
#include "libcmt/util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

// Voucher(address,uint256,bytes)
#define VOUCHER CMT_ABI_FUNSEL(0x23, 0x7a, 0x81, 0x6f)

// Notice(bytes)
#define NOTICE CMT_ABI_FUNSEL(0xc2, 0x58, 0xd6, 0xe5)

// EvmAdvance(uint256,address,address,uint256,uint256,uint256,uint256,bytes)
#define EVM_ADVANCE CMT_ABI_FUNSEL(0x41, 0x5b, 0xf3, 0x63)

#define DBG(X) debug(X, #X, __FILE__, __LINE__)
static int debug(int rc, const char *expr, const char *file, int line) {
    if (rc == 0) {
        return 0;
    }

    if (cmt_util_debug_enabled()) {
        (void) fprintf(stderr, "%s:%d Error %s on `%s'\n", file, line, expr, strerror(-rc));
    }
    return rc;
}

int cmt_rollup_init(cmt_rollup_t *me) {
    if (!me) {
        return -EINVAL;
    }

    int rc = DBG(cmt_io_init(me->io));
    if (rc) {
        return rc;
    }

    cmt_merkle_init(me->merkle);
    return 0;
}

void cmt_rollup_fini(cmt_rollup_t *me) {
    if (!me) {
        return;
    }

    cmt_io_fini(me->io);
    cmt_merkle_fini(me->merkle);
}

int cmt_rollup_emit_voucher(cmt_rollup_t *me, const cmt_abi_address_t *address,
    const cmt_abi_u256_t *value, const cmt_abi_bytes_t *payload, uint64_t *index) {
    if (!me) {
        return -EINVAL;
    }
    if (!payload || (!payload->data && payload->length)) {
        return -EINVAL;
    }

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    cmt_buf_t of[1];
    cmt_buf_t frame[1];

    // clang-format off
    if (DBG(cmt_abi_put_funsel(wr, VOUCHER))
    ||  DBG(cmt_abi_mark_frame(wr, frame))
    ||  DBG(cmt_abi_put_address(wr, address))
    ||  DBG(cmt_abi_put_uint256(wr, value))
    ||  DBG(cmt_abi_put_bytes_s(wr, of))
    ||  DBG(cmt_abi_put_bytes_d(wr, of, frame, payload))) {
        return -ENOBUFS;
    }
    // clang-format on

    size_t used_space = wr->begin - tx->begin;
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_AUTOMATIC,
        .reason = HTIF_YIELD_AUTOMATIC_REASON_TX_OUTPUT,
        .data = used_space,
    }};
    int rc = DBG(cmt_io_yield(me->io, req));
    if (rc) {
        return rc;
    }

    uint64_t count = cmt_merkle_get_leaf_count(me->merkle);

    rc = cmt_merkle_push_back_data(me->merkle, used_space, tx->begin);
    if (rc) {
        return rc;
    }

    if (index) {
        *index = count;
    }

    return 0;
}

int cmt_rollup_emit_notice(cmt_rollup_t *me, const cmt_abi_bytes_t *payload, uint64_t *index) {
    if (!me) {
        return -EINVAL;
    }
    if (!payload || (!payload->data && payload->length)) {
        return -EINVAL;
    }

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    cmt_buf_t of[1];
    cmt_buf_t frame[1];

    // clang-format off
    if (DBG(cmt_abi_put_funsel(wr, NOTICE))
    ||  DBG(cmt_abi_mark_frame(wr, frame))
    ||  DBG(cmt_abi_put_bytes_s(wr, of))
    ||  DBG(cmt_abi_put_bytes_d(wr, of, frame, payload))) {
        return -ENOBUFS;
    }
    // clang-format on

    size_t used_space = wr->begin - tx->begin;
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_AUTOMATIC,
        .reason = HTIF_YIELD_AUTOMATIC_REASON_TX_OUTPUT,
        .data = used_space,
    }};
    int rc = DBG(cmt_io_yield(me->io, req));
    if (rc) {
        return rc;
    }

    uint64_t count = cmt_merkle_get_leaf_count(me->merkle);

    rc = cmt_merkle_push_back_data(me->merkle, used_space, tx->begin);
    if (rc) {
        return rc;
    }

    if (index) {
        *index = count;
    }

    return 0;
}

int cmt_rollup_emit_report(cmt_rollup_t *me, const cmt_abi_bytes_t *payload) {
    if (!me) {
        return -EINVAL;
    }
    if (!payload || (!payload->data && payload->length)) {
        return -EINVAL;
    }

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    if (cmt_buf_split(tx, payload->length, wr, tx)) {
        return -ENOBUFS;
    }

    if (payload->data) {
        memcpy(wr->begin, payload->data, payload->length);
    }
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_AUTOMATIC,
        .reason = HTIF_YIELD_AUTOMATIC_REASON_TX_REPORT,
        .data = payload->length,
    }};
    return DBG(cmt_io_yield(me->io, req));
}

int cmt_rollup_emit_exception(cmt_rollup_t *me, const cmt_abi_bytes_t *payload) {
    if (!me) {
        return -EINVAL;
    }
    if (!payload || (!payload->data && payload->length)) {
        return -EINVAL;
    }

    cmt_buf_t tx[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t wr[1] = {*tx};
    cmt_buf_t _[1];
    if (cmt_buf_split(tx, payload->length, wr, _)) {
        return -ENOBUFS;
    }

    if (payload->data) {
        memcpy(wr->begin, payload->data, payload->length);
    }
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_TX_EXCEPTION,
        .data = payload->length,
    }};
    return DBG(cmt_io_yield(me->io, req));
}

static int cmt_rollup_get_rx(cmt_rollup_t *me, cmt_buf_t *lhs) {
    cmt_buf_t rx[1] = {cmt_io_get_rx(me->io)};
    cmt_buf_t _[1];
    return cmt_buf_split(rx, me->fromhost_data, lhs, _);
}

int cmt_rollup_read_advance_state(cmt_rollup_t *me, cmt_rollup_advance_t *advance) {
    if (!me) {
        return -EINVAL;
    }
    if (!advance) {
        return -EINVAL;
    }

    cmt_buf_t rd[1];
    if (cmt_rollup_get_rx(me, rd)) {
        return -ENOBUFS;
    }
    cmt_buf_t frame[1];
    cmt_buf_t of[1];

    // clang-format off
    if (DBG(cmt_abi_check_funsel(rd, EVM_ADVANCE))
    ||  DBG(cmt_abi_mark_frame(rd, frame))
    ||  DBG(cmt_abi_get_uint(rd, sizeof(advance->chain_id), &advance->chain_id))
    ||  DBG(cmt_abi_get_address(rd, &advance->app_contract))
    ||  DBG(cmt_abi_get_address(rd, &advance->msg_sender))
    ||  DBG(cmt_abi_get_uint(rd, sizeof(advance->block_number), &advance->block_number))
    ||  DBG(cmt_abi_get_uint(rd, sizeof(advance->block_timestamp), &advance->block_timestamp))
    ||  DBG(cmt_abi_get_uint256(rd, &advance->prev_randao))
    ||  DBG(cmt_abi_get_uint(rd, sizeof(advance->index), &advance->index))
    ||  DBG(cmt_abi_get_bytes_s(rd, of))
    ||  DBG(cmt_abi_get_bytes_d(frame, of, &advance->payload.length, &advance->payload.data))) {
        return -ENOBUFS;
    }
    // clang-format on

    return 0;
}

int cmt_rollup_read_inspect_state(cmt_rollup_t *me, cmt_rollup_inspect_t *inspect) {
    if (!me) {
        return -EINVAL;
    }
    if (!inspect) {
        return -EINVAL;
    }

    cmt_buf_t rd[1];
    if (DBG(cmt_rollup_get_rx(me, rd))) {
        return -ENOBUFS;
    }

    inspect->payload.length = cmt_buf_length(rd);
    inspect->payload.data = rd->begin;
    return 0;
}

static int accepted(union cmt_io_driver *io, uint32_t *n) {
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_RX_ACCEPTED,
        .data = *n,
    }};
    int rc = DBG(cmt_io_yield(io, req));
    if (rc) {
        return rc;
    }

    *n = req->data;
    return req->reason;
}

static int revert(union cmt_io_driver *io) {
    struct cmt_io_yield req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = HTIF_YIELD_MANUAL_REASON_RX_REJECTED,
        .data = 0,
    }};
    return DBG(cmt_io_yield(io, req));
}

int cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish) {
    if (!me) {
        return -EINVAL;
    }
    if (!finish) {
        return -EINVAL;
    }

    if (!finish->accept_previous_request) {
        return revert(me->io); /* revert should not return! */
    }

    cmt_merkle_get_root_hash(me->merkle, cmt_io_get_tx(me->io).begin);
    me->fromhost_data = CMT_ABI_U256_LENGTH;
    int reason = accepted(me->io, &me->fromhost_data);
    if (reason < 0) {
        return reason;
    }
    finish->next_request_type = reason;
    finish->next_request_payload_length = me->fromhost_data;
    cmt_merkle_init(me->merkle);
    return 0;
}

int cmt_gio_request(cmt_rollup_t *me, cmt_gio_t *req) {
    if (!me) {
        return -EINVAL;
    }
    if (!req) {
        return -EINVAL;
    }

    /* Accept empty id */
    if (!req->id && req->id_length) {
        return -EINVAL;
    }

    cmt_buf_t wr[1] = {cmt_io_get_tx(me->io)};
    cmt_buf_t _[1];
    if (cmt_buf_split(wr, req->id_length, wr, _)) {
        return -ENOBUFS;
    }
    if (req->id) {
        memcpy(wr->begin, req->id, req->id_length);
    }

    struct cmt_io_yield rr[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_MANUAL,
        .reason = req->domain,
        .data = req->id_length,
    }};

    int rc = DBG(cmt_io_yield(me->io, rr));
    if (rc != 0) {
        return rc;
    }
    me->fromhost_data = rr->data;

    cmt_buf_t rd[1];
    if (cmt_rollup_get_rx(me, rd)) {
        return -ENOBUFS;
    }

    req->response_data = rd->begin;
    req->response_code = rr->reason;
    req->response_data_length = rr->data;
    return 0;
}

int cmt_rollup_progress(cmt_rollup_t *me, uint32_t progress) {
    cmt_io_yield_t req[1] = {{
        .dev = HTIF_DEVICE_YIELD,
        .cmd = HTIF_YIELD_CMD_AUTOMATIC,
        .reason = HTIF_YIELD_AUTOMATIC_REASON_PROGRESS,
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

void cmt_rollup_reset_merkle(cmt_rollup_t *me) {
    cmt_merkle_reset(me->merkle);
}
