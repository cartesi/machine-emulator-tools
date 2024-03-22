/* Copyright Cartesi and individual authors (see AUTHORS)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "keccak.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        (void) fprintf(stderr, "usage: %s \"<declaration>\"\n", argv[0]);
        exit(1);
    }

    // value encoded as big endian
    uint32_t funsel = cmt_keccak_funsel(argv[1]);
    printf("// %s\n"
           "#define FUNSEL CMT_ABI_FUNSEL(0x%02x, 0x%02x, 0x%02x, 0x%02x)\n",
        argv[1], ((uint8_t *) &funsel)[0], ((uint8_t *) &funsel)[1], ((uint8_t *) &funsel)[2],
        ((uint8_t *) &funsel)[3]);
}
