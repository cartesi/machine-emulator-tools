#include "libcmt/abi.h"
#include "libcmt/buf.h"

int encode_address(cmt_buf_t *tx, uint32_t funsel, cmt_abi_address_t *address) {
    cmt_buf_t wr = *tx;
    /* static section */
    return cmt_abi_put_funsel(&wr, funsel)
    ||     cmt_abi_put_address(&wr, address);
}
