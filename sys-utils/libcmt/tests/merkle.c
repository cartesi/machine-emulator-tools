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
    uint8_t expected_root[CMT_KECCAK_LENGTH] = {0x0a, 0x16, 0x29, 0x46, 0xe5, 0x61, 0x58, 0xba, 0xc0, 0x67, 0x3e, 0x6d,
        0xd3, 0xbd, 0xfd, 0xc1, 0xe4, 0xa0, 0xe7, 0x74, 0x4a, 0x12, 0x0f, 0xdb, 0x64, 0x00, 0x50, 0xc8, 0xd7, 0xab,
        0xe1, 0xc6};
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
    uint8_t expected_root[CMT_KECCAK_LENGTH] = {0x45, 0xa6, 0xfd, 0x68, 0x45, 0xc1, 0x7f, 0xe0, 0x21, 0x7f, 0xf0, 0xfd,
        0x9b, 0x2c, 0x02, 0x1f, 0x2f, 0x2e, 0xe3, 0x9e, 0xe1, 0x99, 0x7d, 0xf3, 0x1f, 0x2d, 0x4c, 0x81, 0x7d, 0x3d,
        0x1b, 0xf7};
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
