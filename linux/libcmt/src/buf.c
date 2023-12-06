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
#include<stdbool.h>
#include<stdio.h>
#include<errno.h>
#include<libcmt/buf.h>

static inline int is_pow2(int l)
{
	return !(l & (l-1));
}

void cmt_buf_init(cmt_buf_t *me, size_t n, void *data)
{
	if (!me) return;
	me->begin = (uint8_t *)data;
	me->end = (uint8_t *)data + n;
}

int cmt_buf_split(const cmt_buf_t *me, size_t lhs_length, cmt_buf_t *lhs, cmt_buf_t *rhs)
{
	if (!me) return -EINVAL;
	if (!lhs) return -EINVAL;
	if (!rhs) return -EINVAL;

	lhs->begin = me->begin;
	lhs->end = rhs->begin = me->begin + lhs_length;
	rhs->end = me->end;

	return lhs->end > me->end? -ENOBUFS : 0;
}

size_t cmt_buf_length(const cmt_buf_t *me)
{
	if (!me) return 0;
	return me->end - me->begin;
}

static void xxd(const uint8_t *p, const uint8_t *q, size_t mask)
{
	if (q < p) return;

	for (size_t i=0u, n = q - p; i < n; ++i) {
		bool is_line_start = (i & mask) ==  0;
		bool is_line_end = (i & mask) == mask || (i+1 == n);
		char separator   = is_line_end? '\n' : ' ';

		if (is_line_start) printf("%p %4zu: ", (void *)(p + i), i);
		printf("%02x%c", p[i], separator);
	}
}

void cmt_buf_xxd(void *p, void *q, int l)
{
	if (!p) return;
	if (!q) return;
	if (q <= p) return;
	if (!is_pow2(l)) return;

	xxd(p, q, l-1);
}
