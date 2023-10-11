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
#include <stdio.h>
#include <string.h>
#include <libcmt/merkle.h>

static void print(int m, uint8_t md[CMT_KECCAK_LENGTH])
{
	printf("%3d: ", m);
	for (int i=0, n=CMT_KECCAK_LENGTH; i<n; ++i)
		printf("%02x%s", md[i], i+1 == n ? "\n" : " ");
}

static void pristine_zero()
{
	uint8_t md[CMT_KECCAK_LENGTH];
	cmt_merkle_t M[1];
	cmt_merkle_init(M);

	for (uint64_t i=0; i<64; ++i) {
		cmt_merkle_get_root_hash(M, md);
		print(i, md);
		cmt_merkle_push_back_data(M, 0, NULL);
	}
}

int main()
{
	pristine_zero();
	return 0;
}
