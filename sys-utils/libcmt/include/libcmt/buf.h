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
/** @file
 * @defgroup libcmt_buf buf
 * slice of contiguous memory
 *
 * @ingroup libcmt
 * @{ */
#ifndef CMT_BUF_H
#define CMT_BUF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** A slice of contiguous memory from @b begin to @b end, as an open range.
 * Size can be taken with: `end - begin`.
 *
 * `begin == end` indicate an empty buffer */
typedef struct {
    uint8_t *begin; /**< begin of memory region */
    uint8_t *end;   /**< end of memory region */
} cmt_buf_t;

/** Initialize @p me buffer backed by @p data, @p length bytes in size
 *
 * @param [out] me     a uninitialized instance
 * @param [in]  length size in bytes of @b data
 * @param [in]  data   the backing memory to be used.
 *
 * @note @p data memory must outlive @p me.
 * user must copy the contents otherwise */
void cmt_buf_init(cmt_buf_t *me, size_t length, void *data);

/** Split a buffer in two, @b lhs with @b lhs_length bytes and @b rhs with the rest
 *
 * @param [in,out] me         initialized buffer
 * @param [in]     lhs_length bytes in @b lhs
 * @param [out]    lhs        left hand side
 * @param [out]    rhs        right hand side
 *
 * @return
 * - 0 success
 * - negative value on error. -ENOBUFS when length(me) < lhs_length. */
int cmt_buf_split(const cmt_buf_t *me, size_t lhs_length, cmt_buf_t *lhs, cmt_buf_t *rhs);

/** Length in bytes of @p me
 *
 * @param [in] me     initialized buffer
 *
 * @return
 * - size in bytes */
size_t cmt_buf_length(const cmt_buf_t *me);

/** Print the contents of @b me buffer to stdout
 *
 * @param [in] begin          start of memory region
 * @param [in] end            end of memory region
 * @param [in] bytes_per_line bytes per line (must be a power of 2). */
void cmt_buf_xxd(void *begin, void *end, int bytes_per_line);

/** Take the substring @p x from @p xs start to the first @p , (comma).
 * @param [out]    x           substring
 * @param [in,out] xs          string (interator)
 *
 * @note @p x points inside @p xs, make a copy if it outlives @p xs. */
bool cmt_buf_split_by_comma(cmt_buf_t *x, cmt_buf_t *xs);

#endif /* CMT_BUF_H */
/** @} */

