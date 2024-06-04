#include "libcmt/abi.h"
#include "libcmt/buf.h"

int decode_address(cmt_buf_t *tx, uint32_t expected_funsel, cmt_abi_address_t *address) {
    cmt_buf_t wr = *tx;
    return cmt_abi_check_funsel(&wr, expected_funsel)
    ||     cmt_abi_get_address(&wr, address);
}
