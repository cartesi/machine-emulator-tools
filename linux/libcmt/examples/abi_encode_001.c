#include <libcmt/abi.h>
#include <libcmt/keccak.h>

int encode(cmt_buf_t wr[1], size_t n, uint8_t *data)
{
	cmt_buf_t of[1];
	uint32_t example = cmt_keccak_funsel("Example(bytes)");
	void *base = wr->begin + 4; // skip function selector

	// funsel
	return cmt_abi_put_funsel(wr, example)
	// static section
	||     cmt_abi_put_bytes_s(wr, of)
	// dynamic section
	||     cmt_abi_put_bytes_d(wr, of, n, data, base)
	;
}

