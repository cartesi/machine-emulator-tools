#include "libcmt/abi.h"
#include "libcmt/buf.h"

int decode(cmt_buf_t *rx, uint32_t expected_funsel, cmt_abi_address_t *address, cmt_abi_u256_t *value) {
    cmt_buf_t rd = *rx;
    return cmt_abi_check_funsel(&rd, expected_funsel)
    ||     cmt_abi_get_address(&rd, address)
    ||     cmt_abi_get_uint256(&rd, value)
    ;
}

