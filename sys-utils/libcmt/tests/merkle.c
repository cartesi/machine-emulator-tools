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
#include "merkle.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#if 0
static void print(int m, uint8_t md[CMT_KECCAK_LENGTH])
{
	printf("%3d: ", m);
	for (int i=0, n=CMT_KECCAK_LENGTH; i<n; ++i)
		printf("%02x%s", md[i], i+1 == n ? "\n" : " ");
}
#endif

static int pristine_zero() {
    uint8_t md[CMT_KECCAK_LENGTH];
    cmt_merkle_t M[1];
    cmt_merkle_init(M);

    const uint8_t expected[] = {
        0xb9,
        0x92,
        0xa5,
        0x00,
        0x58,
        0xa2,
        0x81,
        0x2b,
        0x0f,
        0xc4,
        0xfe,
        0x1b,
        0xbb,
        0xfb,
        0x3d,
        0x8f,
        0xfd,
        0x47,
        0x6f,
        0xb8,
        0x93,
        0x91,
        0x40,
        0x82,
        0x12,
        0xe0,
        0x0a,
        0x70,
        0x19,
        0xe1,
        0x0e,
        0xff,
    };

    for (uint64_t i = 0; i < 64; ++i) {
        cmt_merkle_get_root_hash(M, md);
        if (memcmp(md, expected, sizeof expected) != 0)
            return -1;

        cmt_merkle_push_back_data(M, 0, NULL);
    }
    return 0;
}

int main() {
    assert(pristine_zero() == 0);
    return 0;
}
