#include<stdlib.h>
#include "abi.h"

#define VOUCHER EVM_ABI_FUNSEL(0x00, 0x00, 0x00, 0x01)
#define NOTICE  EVM_ABI_FUNSEL(0x00, 0x00, 0x00, 0x02)

int encode_voucher(evm_buf_t wr[1], uint8_t address[20], size_t n, const void *data);
int send_voucher(evm_buf_t tx[1]);

int decode_voucher(evm_buf_t rd[1], uint8_t address[20], size_t *n, uint8_t **data);
int recv_multiple(evm_buf_t rx[1]);

int encode_voucher(evm_buf_t wr[1], uint8_t address[20], size_t n, const void *data)
{
	evm_buf_t of[1];

	return evm_abi_put_funsel(wr, VOUCHER)
	||     evm_abi_put_address(wr, address)
	||     evm_abi_put_bytes_s(wr, of)
	||     evm_abi_put_bytes_d(wr, of, n, data)
	;
}

int send_voucher(evm_buf_t tx[1])
{
	uint8_t address[20] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	};
	char message[] = "hello world\n";

	evm_buf_t it[1] = { *tx };
	if (encode_voucher(it, address, sizeof(message)-1, message))
		return EXIT_FAILURE;

	//size_t len = it->p - tx->p;
	//merkle_put_data(len, tx->p);
	//ioctl(ROLLUP_IOCTL_EMIT_OUTPUT, len, tx->p);
	return EXIT_SUCCESS;

	//return evm_rollup_emit(rollup, it->p - tx->p);
}

int decode_voucher(evm_buf_t rd[1], uint8_t address[20], size_t *n, uint8_t **data)
{
	evm_buf_t of[1];

	return evm_abi_check_funsel(rd, VOUCHER)
	||     evm_abi_get_address(rd, address)
	||     evm_abi_get_bytes_s(rd, of)
	||     evm_abi_get_bytes_d(rd, of, n, data)
	;
}

int recv_multiple(evm_buf_t rx[1])
{
	evm_buf_t it[1] = { *rx };

	if (evm_buf_length(it) < 4)
		return EXIT_FAILURE;

	switch (evm_abi_peek_funsel(it)) {
	case VOUCHER:;
		uint8_t  address[20];
		size_t   n;
		uint8_t *data;
		if (decode_voucher(it, address, &n, &data) == 0) {
			/* use contents */
		}
		break;
	case NOTICE:
		/* falltrough */
	default: return EXIT_FAILURE;
	}
}

int main()
{
	evm_buf_t tx[1], rx[1];
	send_voucher(tx);
	recv_multiple(rx);

	return 0;
}
