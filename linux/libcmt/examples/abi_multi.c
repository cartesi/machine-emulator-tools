#include <stdlib.h>
#include <libcmt/abi.h>
#include <libcmt/keccak.h>

/* precomputed funsel (from: cmt_keccal_funsel) */
#define EXAMPLE_000 CMT_ABI_FUNSEL(0,0,0,0)
#define EXAMPLE_001 CMT_ABI_FUNSEL(0,0,0,1)

int encode_000(cmt_buf_t wr[1], uint8_t address[CMT_ADDRESS_LENGTH])
{
	// funsel
	return cmt_abi_put_funsel(wr, EXAMPLE_000)
	// static section
	||     cmt_abi_put_address(wr, address)
	// dynamic section
	// > is empty!
	;
}

int decode_000(cmt_buf_t rd[1], uint8_t address[CMT_ADDRESS_LENGTH])
{
	// funsel
	return cmt_abi_check_funsel(rd, EXAMPLE_000)
	// static section
	||     cmt_abi_get_address(rd, address)
	// dynamic section
	// > is empty!
	;
}

int use_000(cmt_buf_t rd[1], cmt_buf_t wr[1])
{
	int rc;
	uint8_t address[CMT_ADDRESS_LENGTH];

	if ((rc = decode_000(rd, address)))
		return rc;

	// valid!
	// ...
	return encode_000(wr, address);
}

int encode_001(cmt_buf_t wr[1], size_t n, uint8_t *data)
{
	cmt_buf_t of[1];
	void *base = wr->begin + 4; // skip function selector
 
	// funsel
	return cmt_abi_put_funsel(wr, EXAMPLE_001)
	// static section
	||     cmt_abi_put_bytes_s(wr, of)
	// dynamic section
	||     cmt_abi_put_bytes_d(wr, of, n, data, base)
	;
}

int decode_001(cmt_buf_t rd[1], size_t *n, void **data)
{
	cmt_buf_t of[1];

	// funsel
	return cmt_abi_check_funsel(rd, EXAMPLE_001)
	// static section
	||     cmt_abi_get_bytes_s(rd, of)
	// dynamic section
	||     cmt_abi_get_bytes_d(rd, of, n, data)
	;
}

int use_001(cmt_buf_t rd[1], cmt_buf_t wr[1])
{
	int rc;
	size_t   n;
	uint8_t *data;

	if ((rc = decode_001(rd, &n, (void **)&data)))
		return rc;

	// valid!
	// ...
	return encode_001(wr, n, data);
}

int decode(cmt_buf_t rd[1], cmt_buf_t wr[1])
{
	if (cmt_buf_length(rd) < 4)
		return EXIT_FAILURE;

	switch (cmt_abi_peek_funsel(rd)) {
	case EXAMPLE_000: return use_000(rd, wr);
	case EXAMPLE_001: return use_001(rd, wr);
	default:          return EXIT_FAILURE;
	}
}
