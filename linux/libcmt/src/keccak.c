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
#include <string.h>
#include <libcmt/abi.h>
#include <libcmt/keccak.h>

#define ROTL64(x, y) \
	(((x) << (y)) | ((x) >> (64 - (y))))

static void
keccakf(uint64_t st[25])
{
	const uint64_t keccakf_rndc[24] = {
		0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
		0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
		0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
		0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
		0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
		0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
		0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
		0x8000000000008080, 0x0000000080000001, 0x8000000080008008
	};
	const int keccakf_rotc[24] = {
		1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
		27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44,
	};
	const int keccakf_piln[24] = {
		10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
		15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1,
	};


#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	for (int i = 0; i < 25; i++) {
		st[i] = __builtin_bswap64((uint64_t *)(st[i]));
	}
#endif

	for (int r = 0; r < 24; r++) {
		uint64_t t, bc[5];

		// Theta
		for (int i = 0; i < 5; i++)
			bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

		for (int i = 0; i < 5; i++) {
			t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
			for (int j = 0; j < 25; j += 5)
				st[j + i] ^= t;
		}

		// Rho Pi
		t = st[1];
		for (int i = 0; i < 24; i++) {
			int j = keccakf_piln[i];
			bc[0] = st[j];
			st[j] = ROTL64(t, keccakf_rotc[i]);
			t = bc[0];
		}

		//  Chi
		for (int j = 0; j < 25; j += 5) {
			for (int i = 0; i < 5; i++)
				bc[i] = st[j + i];
			for (int i = 0; i < 5; i++)
				st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
		}

		//  Iota
		st[0] ^= keccakf_rndc[r];
	}

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	for (int i = 0; i < 25; i++) {
		st[i] = __builtin_bswap64(*(uint64_t *)(&st[i]));
	}
#endif
}

void cmt_keccak_init(cmt_keccak_t *c)
{
	*c = (cmt_keccak_t)CMT_KECCAK_INIT(c);
}

void cmt_keccak_update(cmt_keccak_t *c, size_t n, const void *data)
{
	int j = c->pt;
	for (size_t i = 0; i < n; i++) {
		c->st.b[j++] ^= ((const uint8_t *) data)[i];
		if (j >= c->rsiz) {
			keccakf(c->st.q);
			j = 0;
		}
	}
	c->pt = j;
}

void cmt_keccak_final(cmt_keccak_t *c, void *md)
{
	c->st.b[c->pt] ^= 0x01;
	c->st.b[c->rsiz - 1] ^= 0x80;
	keccakf(c->st.q);

	for (int i = 0; i < CMT_KECCAK_LENGTH; i++) {
		((uint8_t *) md)[i] = c->st.b[i];
	}
}

uint8_t *cmt_keccak_data(size_t n, const void *data, uint8_t md[CMT_KECCAK_LENGTH])
{
	CMT_KECCAK_DECL(c);
	cmt_keccak_update(c, n, data);
	cmt_keccak_final(c, md);
	return md;
}

uint32_t cmt_keccak_funsel(const char *decl)
{
	uint8_t md[32];
	cmt_keccak_data(strlen(decl), decl, md);
	return CMT_ABI_FUNSEL(md[0], md[1], md[2], md[3]);
}

