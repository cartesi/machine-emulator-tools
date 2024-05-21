#include "libcmt/abi.h"
#include "libcmt/buf.h"

int encode_bytes(cmt_buf_t *tx, uint32_t funsel,
                 size_t bytes0_length, void *bytes0_data,
                 size_t bytes1_length, void *bytes1_data) {
    cmt_buf_t wr = *tx;
    cmt_buf_t of[2];
    cmt_buf_t frame;

    // static section
    return cmt_abi_put_funsel(&wr, funsel)
    ||     cmt_abi_mark_frame(&wr, &frame)
    ||     cmt_abi_put_bytes_s(&wr, &of[0])
    ||     cmt_abi_put_bytes_s(&wr, &of[1])
    // dynamic section
    ||     cmt_abi_put_bytes_d(&wr, &frame, &of[0], &(cmt_abi_bytes_t){bytes0_length, bytes0_data})
    ||     cmt_abi_put_bytes_d(&wr, &frame, &of[1], &(cmt_abi_bytes_t){bytes1_length, bytes1_data})
    ;
}

