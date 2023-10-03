#include<stdio.h>
#include"abi.h"

int encode_voucher(evm_buf_t wr[1], uint8_t address[20], size_t n, const uint8_t data[n])
{
	evm_buf_t of[1];

	uint32_t voucher = EVM_ABI_FUNSEL(0x01, 0x02, 0x03, 0x04);
	// uint32_t voucher = evm_keccak_funsel("Voucher(address, bytes)");

	// funsel
	return evm_abi_put_funsel(wr, voucher)
	// static section
	||     evm_abi_put_address(wr, address)
	||     evm_abi_put_bytes_s(wr, of)
	// dynamic section
	||     evm_abi_put_bytes_d(wr, of, n, data)
	;
}
