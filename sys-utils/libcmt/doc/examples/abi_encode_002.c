#include "libcmt/abi.h"
#include "libcmt/buf.h"

int encode_bytes(cmt_buf_t *tx, uint32_t funsel, cmt_abi_bytes_t *payload0, cmt_abi_bytes_t *payload1) {
    cmt_buf_t wr = *tx;
    cmt_buf_t of[2];
    cmt_buf_t frame;

    // static section
    return cmt_abi_put_funsel(&wr, funsel)
    ||     cmt_abi_mark_frame(&wr, &frame)
    ||     cmt_abi_put_bytes_s(&wr, &of[0])
    ||     cmt_abi_put_bytes_s(&wr, &of[1])
    // dynamic section
    ||     cmt_abi_put_bytes_d(&wr, &frame, &of[0], payload0)
    ||     cmt_abi_put_bytes_d(&wr, &frame, &of[1], payload1)
    ;
}

