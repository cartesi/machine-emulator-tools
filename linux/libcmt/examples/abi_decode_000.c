#include <libcmt/abi.h>
#include <libcmt/keccak.h>

int decode(cmt_buf_t rd[1], uint8_t address[CMT_ADDRESS_LENGTH])
{
	uint32_t expected = cmt_keccak_funsel("Example(address)");

	// funsel
	return cmt_abi_check_funsel(rd, expected)
	// static section
	||     cmt_abi_get_address(rd, address)
	// dynamic section
	// > is empty!
	;
}

void use(cmt_buf_t rd[1])
{
	uint8_t address[CMT_ADDRESS_LENGTH];

	if (decode(rd, address))
		return;

	// valid!
	// ...
}
