#include <libcmt/abi.h>
#include <libcmt/keccak.h>

int decode(cmt_buf_t rd[1], size_t *n, void **data)
{
	cmt_buf_t of[1];

	uint32_t expected = cmt_keccak_funsel("Example(bytes)");

	// funsel
	return cmt_abi_check_funsel(rd, expected)
	// static section
	||     cmt_abi_get_bytes_s(rd, of)
	// dynamic section
	||     cmt_abi_get_bytes_d(rd, of, n, data)
	;
}

void use(cmt_buf_t rd[1])
{
	size_t n;
	void  *data;

	if (decode(rd, &n, &data))
		return;

	// valid!
	// ...
}
