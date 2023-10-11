#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcmt/merkle.h"
#include "rollup-driver.h"

int main(void)
{
	cmt_merkle_t M[1];
	cmt_rollup_driver_t R[1];
	uint64_t n, rxmax, txmax;

	if (cmt_rollup_driver_init(R) != 0)
		return EXIT_FAILURE;

	const void *rx = cmt_rollup_driver_get_rx(R, &rxmax);
	void       *tx = cmt_rollup_driver_get_tx(R, &txmax);

	for (;;) {
		int type = cmt_rollup_driver_accept_and_wait_next_input(R, &n);
		cmt_merkle_init(M);

		switch (type) {
		case CMT_ROLLUP_ADVANCE_STATE:
			memcpy(tx, rx, n); /* echo */
			cmt_rollup_driver_write_output(R, n);
			cmt_merkle_push_back_data(M, n, tx);

			cmt_rollup_driver_write_report(R, n);

			cmt_merkle_get_root_hash(M, tx);
			printf("advance\n");
			break;
		case CMT_ROLLUP_INSPECT_STATE:
			cmt_rollup_driver_write_report(R, n);
			printf("inspect\n");
			break;
		default:
			cmt_rollup_driver_revert(R);
			break;
		}
	}

	cmt_rollup_driver_fini(R);
	return EXIT_SUCCESS;
}
