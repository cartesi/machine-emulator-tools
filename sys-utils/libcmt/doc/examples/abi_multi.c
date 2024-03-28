#include "libcmt/abi.h"
#include "libcmt/buf.h"

// Echo(bytes)
#define ECHO CMT_ABI_FUNSEL(0x5f, 0x88, 0x6b, 0x86)

static int encode_echo(cmt_buf_t *tx, uint32_t funsel, size_t length, void *data) {
    cmt_buf_t wr = *tx;
    cmt_buf_t of;
    void *params_base = wr.begin + 4; // after funsel

    // static section
    return cmt_abi_put_funsel(&wr, funsel)
    ||     cmt_abi_put_bytes_s(&wr, &of)
    // dynamic section
    ||     cmt_abi_put_bytes_d(&wr, &of, length, data, params_base)
    ;
}

static int decode_echo(cmt_buf_t *rx, uint32_t funsel, size_t *length, void **data) {
    cmt_buf_t rd = *rx;
    cmt_buf_t of;
    void *params_base = wr.begin + 4; // after funsel

    // static section
    return cmt_abi_get_funsel(&wr, funsel)
    ||     cmt_abi_get_bytes_s(&wr, &of)
    // dynamic section
    ||     cmt_abi_get_bytes_d(&wr, &of, length, data, params_base)
    ;
}

int f(cmt_buf_t *wr, cmt_buf_t *rd)
{
	int rc;
	size_t length;
	void *data;

	if (cmt_buf_length(rd) < 4)
		return -ENOBUFS;
	switch (cmt_abi_peek_funsel(rd)) {
	case ECHO:
		rc = decode_echo(rd, CALL, &length, &data);
		if (rc) return rc;

		rc = encode_echo(wr, CALL, length, data);
		if (rc) return rc;

		break;
	}
	return 0;
}
