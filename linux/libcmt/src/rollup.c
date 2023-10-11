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
#include <errno.h>
#include <string.h>
#include <libcmt/rollup.h>

// Voucher(address,bytes)
#define VOUCHER CMT_ABI_FUNSEL(0xef, 0x61, 0x5e, 0x2f)

// Notice(bytes)
#define NOTICE CMT_ABI_FUNSEL(0xc2, 0x58, 0xd6, 0xe5)

// EvmAdvance(address,uint256,uint256,uint256,bytes)
#define EVM_ADVANCE CMT_ABI_FUNSEL(0xd2, 0x0c, 0x60, 0xb4)

// EvmInspect(bytes)
#define EVM_INSPECT CMT_ABI_FUNSEL(0x73, 0xd4, 0x41, 0x43)

int cmt_rollup_init(cmt_rollup_t *me)
{
	if (!me) return -EINVAL;

	int rc = 0;
	if ((rc = cmt_rollup_driver_init(me->rollup_driver)))
		return rc;

	uint64_t rxmax, txmax;
	void *rx = cmt_rollup_driver_get_rx(me->rollup_driver, &rxmax);
	void *tx = cmt_rollup_driver_get_tx(me->rollup_driver, &txmax);

	cmt_buf_init(me->tx, txmax, tx);
	cmt_buf_init(me->rx, rxmax, rx);
	cmt_merkle_init(me->merkle);
	return 0;
}

void cmt_rollup_fini(cmt_rollup_t *me)
{
	if (!me) return;

	cmt_rollup_driver_fini(me->rollup_driver);
	cmt_merkle_fini(me->merkle);
}

int cmt_rollup_emit_voucher(cmt_rollup_t *me, uint8_t address[20], size_t length, const void *data)
{
	if (!me) return -EINVAL;
	if (!data && length) return -EINVAL;

	void *params_base = me->tx->begin + 4; // after funsel
	cmt_buf_t wr[1] = {*me->tx},
		  of[1];

	if (cmt_abi_put_funsel(wr, VOUCHER)
	||  cmt_abi_put_address(wr, address)
	||  cmt_abi_put_bytes_s(wr, of)
	||  cmt_abi_put_bytes_d(wr, of, length, data, params_base))
		return -ENOBUFS;

	size_t m = wr->begin - me->tx->begin;
	cmt_merkle_push_back_data(me->merkle, m, me->tx->begin);
	return cmt_rollup_driver_write_output(me->rollup_driver, m);
}

int cmt_rollup_emit_notice(cmt_rollup_t *me, size_t length, const void *data)
{
	if (!me) return -EINVAL;
	if (!data && length) return -EINVAL;

	void *params_base = me->tx->begin + 4; // after funsel
	cmt_buf_t wr[1] = {*me->tx},
		  of[1];

	if (cmt_abi_put_funsel(wr, NOTICE)
	||     cmt_abi_put_bytes_s(wr, of)
	||     cmt_abi_put_bytes_d(wr, of, length, data, params_base))
		return -ENOBUFS;

	size_t m = wr->begin - me->tx->begin;
	cmt_merkle_push_back_data(me->merkle, m, me->tx->begin);
	return cmt_rollup_driver_write_output(me->rollup_driver, m);
}

int cmt_rollup_emit_report(cmt_rollup_t *me, size_t length, const void *data)
{
	if (!me) return -EINVAL;
	if (!data && length) return -EINVAL;
	if (cmt_buf_length(me->tx) < length)
		return -ENOBUFS;

	memcpy(me->tx->begin, data, length);
	return cmt_rollup_driver_write_report(me->rollup_driver, length);
}

int cmt_rollup_emit_exception(cmt_rollup_t *me, size_t length, const void *data)
{
	if (!me) return -EINVAL;
	if (!data && length) return -EINVAL;
	if (cmt_buf_length(me->tx) < length)
		return -ENOBUFS;

	memcpy(me->tx->begin, data, length);
	return cmt_rollup_driver_write_exception(me->rollup_driver, length);
}

int cmt_rollup_read_advance_state(cmt_rollup_t *me, cmt_rollup_advance_t *advance)
{
	if (!me) return -EINVAL;
	if (!advance) return -EINVAL;

	int rc = 0;
	cmt_buf_t rd[1] = {*me->rx},
	          st[1] = {{me->rx->begin + 4, me->rx->end}}, // EVM offsets are from after funsel
	          of[1];
	
	if (cmt_abi_check_funsel(rd, EVM_ADVANCE)
	|| (rc = cmt_abi_get_address(rd, advance->sender))
	|| (rc = cmt_abi_get_uint(rd, sizeof(advance->block_number), &advance->block_number))
	|| (rc = cmt_abi_get_uint(rd, sizeof(advance->block_timestamp), &advance->block_timestamp))
	|| (rc = cmt_abi_get_uint(rd, sizeof(advance->index), &advance->index))
	|| (rc = cmt_abi_get_bytes_s(rd, of))
	|| (rc = cmt_abi_get_bytes_d(st, of, &advance->length, &advance->data)))
		return rc;
	return 0;
}

int cmt_rollup_read_inspect_state(cmt_rollup_t *me, cmt_rollup_inspect_t *inspect)
{
	if (!me) return -EINVAL;
	if (!inspect) return -EINVAL;

	int rc = 0;
	cmt_buf_t rd[1] = {*me->rx},
	          st[1] = {{me->rx->begin + 4, me->rx->end}}, // EVM offsets are from after funsel
		  of[1];
	
	if (cmt_abi_check_funsel(rd, EVM_INSPECT)
	|| (rc = cmt_abi_get_bytes_s(rd, of))
	|| (rc = cmt_abi_get_bytes_d(st, of, &inspect->length, &inspect->data)))
		return rc;
	return 0;
}

int cmt_rollup_finish(cmt_rollup_t *me, cmt_rollup_finish_t *finish)
{
	if (!me) return -EINVAL;
	if (!finish) return -EINVAL;

	if (!finish->accept_previous_request) {
		cmt_rollup_driver_revert(me->rollup_driver);
		/* revert should not return! */
		return -EBADE;
	}

	cmt_merkle_get_root_hash(me->merkle, me->tx->begin); // send the merkle outputs_root_hash
	return finish->next_request_type = cmt_rollup_driver_accept_and_wait_next_input
		(me->rollup_driver, &finish->next_request_payload_length);
}
