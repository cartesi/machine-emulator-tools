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
#include "libcmt/merkle.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>

#if 0 // NOLINT
static void print(int m, uint8_t md[CMT_KECCAK_LENGTH]) {
    printf("%3d: ", m);
    for (int i = 0, n = CMT_KECCAK_LENGTH; i < n; ++i)
        printf("0x%02x%s", md[i], i + 1 == n ? "\n" : ", ");
}
#endif

void test_merkle_init_and_reset(void) {
    cmt_merkle_t merkle;
    cmt_merkle_init(&merkle);
    assert(cmt_merkle_get_leaf_count(&merkle) == 0);
    for (int i = 0; i < CMT_MERKLE_TREE_HEIGHT; ++i) {
        for (int j = 0; j < CMT_KECCAK_LENGTH; ++j) {
            assert(merkle.state[i][j] == 0);
        }
    }

    uint8_t data[CMT_KECCAK_LENGTH] = {0};
    assert(cmt_merkle_push_back(&merkle, data) == 0);
    assert(cmt_merkle_get_leaf_count(&merkle) == 1);

    cmt_merkle_reset(&merkle);
    assert(cmt_merkle_get_leaf_count(&merkle) == 0);
    for (int i = 0; i < CMT_MERKLE_TREE_HEIGHT; ++i) {
        for (int j = 0; j < CMT_KECCAK_LENGTH; ++j) {
            assert(merkle.state[i][j] == 0);
        }
    }
    printf("test_merkle_init_and_reset passed\n");
}

void test_merkle_push_back_and_get_root(void) {
    cmt_merkle_t merkle;
    cmt_merkle_init(&merkle);
    uint8_t data[CMT_KECCAK_LENGTH] = {0};
    assert(cmt_merkle_push_back(&merkle, data) == 0);
    assert(cmt_merkle_get_leaf_count(&merkle) == 1);

    uint8_t root[CMT_KECCAK_LENGTH];
    cmt_merkle_get_root_hash(&merkle, root);
    uint8_t expected_root[CMT_KECCAK_LENGTH] = {0x27, 0x33, 0xe5, 0x0f, 0x52, 0x6e, 0xc2, 0xfa, 0x19, 0xa2, 0x2b, 0x31,
        0xe8, 0xed, 0x50, 0xf2, 0x3c, 0xd1, 0xfd, 0xf9, 0x4c, 0x91, 0x54, 0xed, 0x3a, 0x76, 0x09, 0xa2, 0xf1, 0xff,
        0x98, 0x1f};
    for (int i = 0; i < CMT_KECCAK_LENGTH; ++i) {
        assert(root[i] == expected_root[i]);
    }
    printf("test_merkle_push_back_and_get_root passed\n");
}

void test_cmt_merkle_push_back_data_and_get_root(void) {
    cmt_merkle_t merkle;
    cmt_merkle_init(&merkle);

    const char *data1 = "Cartesi";
    const char *data2 = "Merkle";
    const char *data3 = "Tree";
    assert(cmt_merkle_push_back_data(&merkle, strlen(data1), data1) == 0);
    assert(cmt_merkle_push_back_data(&merkle, strlen(data2), data2) == 0);
    assert(cmt_merkle_push_back_data(&merkle, strlen(data3), data3) == 0);

    uint8_t root[CMT_KECCAK_LENGTH];
    cmt_merkle_get_root_hash(&merkle, root);
    uint8_t expected_root[CMT_KECCAK_LENGTH] = {0xe8, 0xe0, 0x47, 0x71, 0x14, 0xcb, 0x63, 0x0c, 0x4d, 0x14, 0xee, 0xa2,
        0x49, 0xeb, 0x2c, 0x63, 0xd8, 0x4c, 0x9c, 0x68, 0x5d, 0xdf, 0x35, 0xd1, 0x37, 0x01, 0x9e, 0x65, 0x9a, 0xe2,
        0x04, 0x18};
    for (int i = 0; i < CMT_KECCAK_LENGTH; ++i) {
        assert(root[i] == expected_root[i]);
    }

    printf("test_merkle_push_back_data_and_get_root passed\n");
}

void test_cmt_merkle_push_back(void) {
    cmt_merkle_t merkle;
    cmt_merkle_init(&merkle);

    uint8_t hash1[CMT_KECCAK_LENGTH] = {0};
    assert(cmt_merkle_push_back(&merkle, hash1) == 0);
    assert(cmt_merkle_get_leaf_count(&merkle) == 1);

    uint8_t hash2[CMT_KECCAK_LENGTH] = {1};
    assert(cmt_merkle_push_back(&merkle, hash2) == 0);
    assert(cmt_merkle_get_leaf_count(&merkle) == 2);

    printf("test_merkle_push_back passed\n");
}

void test_cmt_merkle_push_back_data(void) {
    cmt_merkle_t merkle;
    cmt_merkle_init(&merkle);

    const char *data1 = "test data 1";
    assert(cmt_merkle_push_back_data(&merkle, strlen(data1), data1) == 0);
    assert(cmt_merkle_get_leaf_count(&merkle) == 1);

    const char *data2 = "test data 2";
    assert(cmt_merkle_push_back_data(&merkle, strlen(data2), data2) == 0);
    assert(cmt_merkle_get_leaf_count(&merkle) == 2);

    printf("test_merkle_push_back_data passed\n");
}

void test_cmt_merkle_save_load(void) {
    cmt_merkle_t merkle1;
    cmt_merkle_t merkle2;
    char valid[] = "/tmp/tmp.XXXXXX";
    char invalid[] = "/tmp/tmp.XXXXXX/invalid.bin";

    // create a unique file and an invalid file from it.
    (void) !mkstemp(valid);
    memcpy(invalid, valid, strlen(valid));

    // Initialize merkle1 and add some data
    cmt_merkle_init(&merkle1);
    uint8_t data[CMT_KECCAK_LENGTH] = {0};
    for (int i = 0; i < 5; ++i) {
        // Fill data with i
        memset(data, i,
            CMT_KECCAK_LENGTH); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        assert(cmt_merkle_push_back_data(&merkle1, CMT_KECCAK_LENGTH, data) == 0);
    }

    // succeed with normal usage
    assert(cmt_merkle_save(&merkle1, valid) == 0);
    assert(cmt_merkle_load(&merkle2, valid) == 0);
    assert(memcmp(&merkle1, &merkle2, sizeof(merkle1)) == 0);

    // fail to load smaller file
    (void) !truncate(valid, sizeof(merkle1) - 1);
    assert(cmt_merkle_load(&merkle2, valid) != 0);
    (void) !remove(valid);

    // fail to save/load invalid filepath
    assert(cmt_merkle_save(&merkle1, invalid) != 0);
    assert(cmt_merkle_load(&merkle2, invalid) != 0);

    // invalid state
    assert(cmt_merkle_save(NULL, valid) == -EINVAL);
    assert(cmt_merkle_load(NULL, valid) == -EINVAL);

    printf("test_cmt_merkle_save_load passed\n");
}

void test_cmt_merkle_full(void) {
    const uint64_t max_count =
        (CMT_MERKLE_TREE_HEIGHT < 8 * sizeof(uint64_t)) ? (UINT64_C(1) << CMT_MERKLE_TREE_HEIGHT) : UINT64_MAX;
    cmt_merkle_t merkle = {
        .leaf_count = max_count - 1,
    };
    assert(cmt_merkle_get_leaf_count(&merkle) == max_count - 1);

    uint8_t data[] = {0};
    assert(cmt_merkle_push_back_data(&merkle, sizeof(data), data) == 0);
    assert(cmt_merkle_push_back_data(&merkle, sizeof(data), data) == -ENOBUFS);
    assert(cmt_merkle_get_leaf_count(&merkle) == max_count);
}

int main(void) {
    setenv("CMT_DEBUG", "yes", 1);
    test_merkle_init_and_reset();
    test_merkle_push_back_and_get_root();
    test_cmt_merkle_push_back_data_and_get_root();
    test_cmt_merkle_push_back();
    test_cmt_merkle_push_back_data();
    test_cmt_merkle_save_load();
    test_cmt_merkle_full();
    printf("All merkle tests passed!\n");
    return 0;
}
