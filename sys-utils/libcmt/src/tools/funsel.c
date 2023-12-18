#include <libcmt/keccak.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s \"<declaration>\"\n", argv[0]);
        exit(1);
    }

    // value encoded as big endian
    uint32_t funsel = cmt_keccak_funsel(argv[1]);
    printf("// %s\n"
           "#define FUNSEL CMT_ABI_FUNSEL(0x%02x, 0x%02x, 0x%02x, 0x%02x)\n",
        argv[1], ((uint8_t *) &funsel)[0], ((uint8_t *) &funsel)[1], ((uint8_t *) &funsel)[2],
        ((uint8_t *) &funsel)[3]);
}
