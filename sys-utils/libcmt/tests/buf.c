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
#include <stdio.h>
#include <errno.h>
#include "buf.h"

void split_in_bounds_must_succeed()
{
    uint8_t _[8];
    cmt_buf_t b;
    cmt_buf_init(&b, sizeof _, _);
    assert(cmt_buf_length(&b) == 8);

    { // everything to lhs
        cmt_buf_t lhs;
        cmt_buf_t rhs;
        assert(cmt_buf_split(&b, 8, &lhs, &rhs) == 0);
        assert(cmt_buf_length(&lhs) == 8);
        assert(cmt_buf_length(&rhs) == 0);
    }

    { // everything to rhs
        cmt_buf_t lhs;
        cmt_buf_t rhs;
        assert(cmt_buf_split(&b, 0, &lhs, &rhs) == 0);
        assert(cmt_buf_length(&lhs) == 0);
        assert(cmt_buf_length(&rhs) == 8);
    }

    { // handle alias (lhs)
        cmt_buf_t tmp = b;
        cmt_buf_t rhs;
        assert(cmt_buf_split(&tmp, 8, &tmp, &rhs) == 0);
        assert(cmt_buf_length(&tmp) == 8);
        assert(cmt_buf_length(&rhs) == 0);

        assert(tmp.begin == b.begin);
    }

    { // handle alias (rhs)
        cmt_buf_t tmp = b;
        cmt_buf_t lhs;
        assert(cmt_buf_split(&tmp, 8, &lhs, &tmp) == 0);
        assert(cmt_buf_length(&lhs) == 8);
        assert(cmt_buf_length(&tmp) == 0);

        assert(tmp.begin == b.end);
    }
    printf("test_buf_split_in_bounds_must_succeed passed\n");
}

void split_out_of_bounds_must_fail(void)
{
    uint8_t _[8];
    cmt_buf_t b;
    cmt_buf_t lhs;
    cmt_buf_t rhs;
    cmt_buf_init(&b, sizeof _, _);

    assert(cmt_buf_split(&b, 9, &lhs, &rhs) == -ENOBUFS);
    assert(cmt_buf_split(&b, SIZE_MAX, &lhs, &rhs) == -ENOBUFS);
    printf("test_buf_split_out_of_bounds_must_fail passed\n");

}

int main(void)
{
    split_in_bounds_must_succeed();
    split_out_of_bounds_must_fail();
    return 0;
}

