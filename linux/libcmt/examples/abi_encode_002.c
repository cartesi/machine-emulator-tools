#include <libcmt/abi.h>
#include <libcmt/keccak.h>

int encode(cmt_buf_t wr[1]
	  ,uint8_t address[CMT_ADDRESS_LENGTH]
	  ,size_t n0, uint8_t *data0
	  ,size_t n1, uint8_t *data1)
{
	cmt_buf_t of[2][1];
	void *base = wr->begin + 4; // skip function selector
	uint32_t example = cmt_keccak_funsel("Example(address,bytes,bytes)");

	// funsel
	return cmt_abi_put_funsel(wr, example)
	// static section
	||     cmt_abi_put_address(wr, address)
	||     cmt_abi_put_bytes_s(wr, of[0])
	||     cmt_abi_put_bytes_s(wr, of[1])
	// dynamic section
	||     cmt_abi_put_bytes_d(wr, of[0], n0, data0, base)
	||     cmt_abi_put_bytes_d(wr, of[1], n1, data1, base)
	;
}


