#include <stdio.h>
#include <libcmt/abi.h>
#include <libcmt/keccak.h>

int decode_voucher(cmt_buf_t rd[1], uint8_t address[20], size_t *n, void **data)
{
	cmt_buf_t of[1];

	uint32_t voucher = CMT_ABI_FUNSEL(0x00, 0x00, 0x00, 0x01);
	// uint32_t voucher = cmt_keccak_funsel("Voucher(address, bytes)");

	// funsel
	return cmt_abi_check_funsel(rd, voucher)
	// static section
	||     cmt_abi_get_address(rd, address)
	||     cmt_abi_get_bytes_s(rd, of)
	// dynamic section
	||     cmt_abi_get_bytes_d(rd, of, n, data)
	;
}
