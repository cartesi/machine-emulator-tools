#include<stdio.h>
#include"abi.h"

int decode_voucher(evm_buf_t rd[1], uint8_t address[20], size_t *n, uint8_t **data)
{
	evm_buf_t of[1];

	uint32_t voucher = EVM_ABI_FUNSEL(0x00, 0x00, 0x00, 0x01);
	// uint32_t voucher = evm_keccak_funsel("Voucher(address, bytes)");

	// funsel
	return evm_abi_check_funsel(rd, voucher)
	// static section
	||     evm_abi_get_address(rd, address)
	||     evm_abi_get_bytes_s(rd, of)
	// dynamic section
	||     evm_abi_get_bytes_d(rd, of, n, data)
	;
}
