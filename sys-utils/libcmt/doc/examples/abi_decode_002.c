#include "libcmt/abi.h"
#include "libcmt/buf.h"

int decode(cmt_buf_t *rx, uint32_t expected_funsel, cmt_abi_address_t *address, cmt_abi_bytes_t *payload) {
    cmt_buf_t rd = *rx;
    cmt_buf_t frame;
    cmt_buf_t offset;

    /* static section */
    return cmt_abi_check_funsel(&rd, expected_funsel)
    ||     cmt_abi_mark_frame(&rd, &frame)
    ||     cmt_abi_get_address(&rd, address)
    ||     cmt_abi_get_bytes_s(&rd, &offset)
    /* dynamic section */
    ||     cmt_abi_get_bytes_d(&rd, &offset, &payload->length, &payload->data)
    ;
}

