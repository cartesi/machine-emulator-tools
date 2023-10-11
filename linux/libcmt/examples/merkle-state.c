#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libcmt/merkle.h"
#include "libcmt/buf.h"

static void dump_state(size_t length, uint8_t *p)
{
	cmt_buf_xxd(p    , p + 8     , 32);
	cmt_buf_xxd(p + 8, p + length, 32);
}

int main(void)
{
	int rc;
	cmt_merkle_t M[1], N[1];

	cmt_merkle_init(M);
	size_t state_length = cmt_merkle_state_get_length_in_bytes(M);

	uint8_t *state = malloc(state_length);
	if (!state)
		exit(EXIT_FAILURE);

	if ((rc = cmt_merkle_state_save(M, state_length, state))) {
		printf("%s:%d :: %s [%d]\n", __FILE__, __LINE__, strerror(-rc), -rc);
	}
	dump_state(state_length, state);
	cmt_merkle_fini(M);

	cmt_merkle_init(N);
	if ((rc = cmt_merkle_state_load(N, state_length, state))) {
		printf("%s:%d :: %s [%d]\n", __FILE__, __LINE__, strerror(-rc), -rc);
	}
	free(state);
	cmt_merkle_fini(N);

	return 0;
}
