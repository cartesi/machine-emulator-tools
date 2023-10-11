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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "rollup-driver.h"

int cmt_rollup_driver_init(struct cmt_rollup_driver *me)
{
	if (!me) return -EINVAL;
	me->input_seq =-1;
	me->output_seq = 0;
	me->report_seq = 0;
	me->tx->len = 2 * 1024 * 1024; // 2MB
	me->rx->len = 2 * 1024 * 1024; // 2MB
	me->tx->data = malloc(me->tx->len);
	me->rx->data = malloc(me->rx->len);

	if (!me->tx->data || !me->rx->data) {
		free(me->tx->data);
		free(me->rx->data);
		return -ENOMEM;
	}
	return 0;
}

void cmt_rollup_driver_fini(struct cmt_rollup_driver *me)
{
	if (!me) return;
	free(me->tx->data);
	free(me->rx->data);
}

static int cmt_rollup_driver_mock_write(struct cmt_rollup_driver *me, size_t n, const char *filename)
{
	int rc = 0;
	if (me->tx->len < n)
		return -ENOBUFS;

	FILE *file = fopen(filename, "wb");
	if (!file)
		return -errno;

	if (fwrite(me->tx->data, 1, n, file) != n)
		rc = -EIO;

	if (fclose(file))
		return -errno;

	return rc;
}

int cmt_rollup_driver_write_output(struct cmt_rollup_driver *me, size_t n)
{
	char filename[1024];
	if (!me) return -EINVAL;
	snprintf(filename, sizeof(filename), "input-%u-output-%u.bin",
		 me->input_seq, ++me->output_seq - 1);
	return cmt_rollup_driver_mock_write(me, n, filename);
}

int cmt_rollup_driver_write_report(struct cmt_rollup_driver *me, size_t n)
{
	char filename[1024];
	if (!me) return -EINVAL;
	snprintf(filename, sizeof(filename), "input-%u-report-%u.bin",
		 me->input_seq, ++me->report_seq - 1);
	return cmt_rollup_driver_mock_write(me, n, filename);
}

int cmt_rollup_driver_write_exception(cmt_rollup_driver_t *me, uint64_t n)
{
	if (!me) return -EINVAL;
	if (me->tx->len < n)
		return -ENOBUFS;

	fprintf(stderr, "rollup exception with payload: \"%.*s\"\n",
		(int)me->tx->len, (char *)me->tx->data);
	exit(1);
	return 0;
}

static int read_file_contents(FILE *file, size_t max, void *data, size_t *n)
{
	long pos;
	if (fseek(file, 0, SEEK_END))
		return -errno;
	if ((pos = ftell(file)) < 0)
		return -errno;
	if (fseek(file, 0, SEEK_SET))
		return -errno;
	*n = pos;
	if (*n > max)
		return -ENOBUFS;
	if (fread(data, 1, *n, file) != *n)
		return -ENODATA;
	return 0;
}

int cmt_rollup_driver_accept_and_wait_next_input(struct cmt_rollup_driver *me, size_t *n)
{
	char b[1024];
	FILE *file = NULL;

	if (!me) return -EINVAL;
	if (me->input_seq != -1) { /* don't write outputs_root_hash of the first accept */
		snprintf(b, sizeof(b), "input-%u-outputs-root-hash.bin", me->input_seq);

		file = fopen(b, "wb");
		if (!file)
			return -errno;
		fwrite(me->tx->data, 1, 32, file);
		fclose(file);
	}

	/* read next input from a file */
	snprintf(b, sizeof(b), "input-%u.bin", ++me->input_seq);
	if ((file = fopen(b, "rb"))) {
		int rc = read_file_contents(file, me->rx->len, me->rx->data, n);
		fclose(file);
		if (rc == 0)
			return CMT_ROLLUP_ADVANCE_STATE;
	}
	return -errno;
}

int cmt_rollup_driver_revert(struct cmt_rollup_driver *me)
{
	if (!me) return -EINVAL;
	fprintf(stderr, "%s:%d revert\n", __FILE__, __LINE__);
	exit(0); // should revert, but this implementation can't deal with it...
}

void *cmt_rollup_driver_get_tx(struct cmt_rollup_driver *me, size_t *max)
{
	if (max)
		*max = me->tx->len;
	return me->tx->data;
}
void *cmt_rollup_driver_get_rx(struct cmt_rollup_driver *me, size_t *max)
{
	if (max)
		*max = me->rx->len;
	return me->rx->data;
}
