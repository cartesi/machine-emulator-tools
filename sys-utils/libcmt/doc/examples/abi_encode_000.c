#include "libcmt/abi.h"
#include "libcmt/buf.h"

int encode_address(cmt_buf_t *tx, uint32_t funsel, uint8_t address[CMT_ADDRESS_LENGTH]) {
    cmt_buf_t wr = *tx;
    return cmt_abi_put_funsel(&wr, funsel)
    ||     cmt_abi_put_address(&wr, address);
}
