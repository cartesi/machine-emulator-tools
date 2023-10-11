#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "libcmt/rollup.h"

int main(void)
{
	cmt_rollup_t rollup;

	if (cmt_rollup_init(&rollup))
		return EXIT_FAILURE;
	
	for (;;) {
		int rc;
		cmt_rollup_advance_t advance;
		cmt_rollup_inspect_t inspect;
		cmt_rollup_finish_t finish = { .accept_previous_request = true };
		int type = cmt_rollup_finish(&rollup, &finish);

		switch (type) {
		case CMT_ROLLUP_ADVANCE_STATE:
			printf("advance [%lu]\n", finish.next_request_payload_length);

			rc = cmt_rollup_read_advance_state(&rollup, &advance);
			if (rc) {
				fprintf(stderr, "failed to parse with: %s [%d]\n", strerror(-rc), -rc);
				finish.accept_previous_request = false;
				break;
			}
			cmt_rollup_emit_voucher(&rollup, advance.sender, advance.length, advance.data);
			cmt_rollup_emit_report(&rollup, advance.length, advance.data);
			break;

		case CMT_ROLLUP_INSPECT_STATE:
			printf("inspect [%lu]\n", finish.next_request_payload_length);

			rc = cmt_rollup_read_inspect_state(&rollup, &inspect);
			if (rc) {
				fprintf(stderr, "failed to parse with: %s [%d]\n", strerror(-rc), -rc);
				finish.accept_previous_request = false;
				break;
			}
			cmt_rollup_emit_report(&rollup, inspect.length, inspect.data);
			break;

		default: // <- end of inputs when testing
			cmt_rollup_fini(&rollup);
			return EXIT_SUCCESS;
		}
	}
	return EXIT_SUCCESS;
}
