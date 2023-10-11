#include <libcmt/abi.h>
#include <libcmt/keccak.h>

int encode(cmt_buf_t wr[1], uint8_t address[CMT_ADDRESS_LENGTH])
{
	uint32_t example = cmt_keccak_funsel("Example(address)");

	// funsel
	return cmt_abi_put_funsel(wr, example)
	// static section
	||     cmt_abi_put_address(wr, address)
	// dynamic section
	// > is empty!
	;
}

