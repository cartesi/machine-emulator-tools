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
#include "libcmt/keccak.h"
#include "libcmt/abi.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_cmt_keccak_init(void) {
    cmt_keccak_t state;
    cmt_keccak_init(&state);
    printf("Test cmt_keccak_init: Passed\n");
}

void test_cmt_keccak_hash_operations(void) {
    const char *input = "The quick brown fox jumps over the lazy dog";
    uint8_t result[CMT_KECCAK_LENGTH] = {0};
    uint8_t expected[CMT_KECCAK_LENGTH] = {0x4d, 0x74, 0x1b, 0x6f, 0x1e, 0xb2, 0x9c, 0xb2, 0xa9, 0xb9, 0x91, 0x1c, 0x82,
        0xf5, 0x6f, 0xa8, 0xd7, 0x3b, 0x04, 0x95, 0x9d, 0x3d, 0x9d, 0x22, 0x28, 0x95, 0xdf, 0x6c, 0x0b, 0x28, 0xaa,
        0x15};

    cmt_keccak_t state;
    cmt_keccak_init(&state);
    cmt_keccak_update(&state, strlen(input), input);
    cmt_keccak_final(&state, result);

    // Compare result with expected
    assert(memcmp(result, expected, CMT_KECCAK_LENGTH) == 0);
    printf("Test cmt_keccak_update and cmt_keccak_final: Passed\n");

    cmt_keccak_data(strlen(input), input, result);

    // Compare result with expected
    assert(memcmp(result, expected, CMT_KECCAK_LENGTH) == 0);
    printf("Test cmt_keccak_data: Passed\n");
}

void test_cmt_keccak_funsel(void) {
    const char s[] = "baz(uint32,bool)";
    assert(cmt_keccak_funsel(s) == CMT_ABI_FUNSEL(0xcd, 0xcd, 0x77, 0xc0));
    printf("Test cmt_keccak_funsel: Passed\n");
}

int main(void) {
    test_cmt_keccak_init();
    test_cmt_keccak_hash_operations();
    test_cmt_keccak_funsel();
    printf("All keccak tests passed!\n");
    return 0;
}
