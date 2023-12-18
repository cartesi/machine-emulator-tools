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
#include <assert.h>
#include <errno.h>
#include <libcmt/abi.h>
#include <libcmt/keccak.h>
#include <stdio.h>
#include <string.h>

static void inits() {
    uint8_t md[3][CMT_KECCAK_LENGTH];
    uint8_t data[] = {
        0x00,
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0x0a,
        0x0b,
        0x0c,
        0x0d,
        0x0e,
        0x0f,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1a,
        0x1b,
        0x1c,
        0x1d,
        0x1e,
        0x1f,
    };

    // from init
    cmt_keccak_t t[1];
    cmt_keccak_init(t);
    cmt_keccak_update(t, sizeof(data), data);
    cmt_keccak_final(t, md[0]);

    // from data
    cmt_keccak_data(sizeof(data), data, md[1]);
    assert(memcmp(md[0], md[1], CMT_KECCAK_LENGTH) == 0);
}

static void funsel() {
    const char s[] = "baz(uint32,bool)";
    assert(cmt_keccak_funsel(s) == CMT_ABI_FUNSEL(0xcd, 0xcd, 0x77, 0xc0));
}

int main() {
    funsel();
    inits();
    return 0;
}
