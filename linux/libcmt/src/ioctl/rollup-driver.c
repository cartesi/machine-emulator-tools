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
#include <unistd.h>
#include <stdint.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <rollup-driver.h>
#include <linux/cartesi/rollup.h>

int cmt_rollup_driver_init(struct cmt_rollup_driver *me)
{
	int rc = 0;

	if (!me) return -EINVAL;
	me->fd = open("/dev/rollup", O_RDWR);
	if (me->fd < 0)
		return -errno;

	struct rollup_setup setup;
	if (ioctl(me->fd, IOCTL_ROLLUP_SETUP, &setup)) {
		rc = -errno;
		goto setup_failed;
	}

	me->tx->length = setup.tx.length;
	me->tx->data = mmap((void *)setup.tx.data, setup.tx.length,
	                    PROT_READ | PROT_WRITE,
	                    MAP_SHARED | MAP_SHARED, me->fd, 0);
	if (me->tx->data == MAP_FAILED) {
		rc = -errno;
		goto tx_map_failed;
	}

	me->rx->length = setup.rx.length;
	me->rx->data = mmap((void *)setup.rx.data, setup.rx.length,
	                    PROT_READ,
	                    MAP_SHARED | MAP_SHARED, me->fd, 0);
	if (me->rx->data == MAP_FAILED) {
		rc = -errno;
		goto rx_map_failed;
	}
	return 0;

rx_map_failed:
	munmap(me->tx->data, me->tx->length);
tx_map_failed:
setup_failed:
	if (close(me->fd))
		return -errno;
	return rc;
}

void cmt_rollup_driver_fini(struct cmt_rollup_driver *me)
{
	if (!me) return;
	munmap(me->tx->data, me->tx->length);
	munmap(me->rx->data, me->rx->length);
	close(me->fd);

	me->tx->data = NULL;
	me->tx->length = 0;
	me->rx->data = NULL;
	me->rx->length = 0;
	me->fd = -1;
}

int cmt_rollup_driver_write_output(struct cmt_rollup_driver *me, uint64_t n)
{
	if (!me) return -EINVAL;
	if (ioctl(me->fd, IOCTL_ROLLUP_WRITE_OUTPUT, (unsigned long) &n))
		return -errno;
	return 0;
}

int cmt_rollup_driver_write_report(struct cmt_rollup_driver *me, uint64_t n)
{
	if (!me) return -EINVAL;
	if (ioctl(me->fd, IOCTL_ROLLUP_WRITE_REPORT, (unsigned long) &n))
		return -errno;
	return 0;
}

int cmt_rollup_driver_write_exception(struct cmt_rollup_driver *me, uint64_t n)
{
	if (!me) return -EINVAL;
	if (ioctl(me->fd, IOCTL_ROLLUP_WRITE_EXCEPTION, (unsigned long) &n))
		return -errno;
	return 0;
}

int cmt_rollup_driver_accept_and_wait_next_input(struct cmt_rollup_driver *me, uint64_t *n)
{
	if (!me) return -EINVAL;
	if (!n) return -EINVAL;
	switch (ioctl(me->fd, IOCTL_ROLLUP_ACCEPT_AND_WAIT_NEXT_INPUT, (unsigned long) n)) {
	case ROLLUP_ADVANCE_STATE: return CMT_ROLLUP_ADVANCE_STATE;
	case ROLLUP_INSPECT_STATE: return CMT_ROLLUP_INSPECT_STATE;
	default:                   return -errno;
	}
}

int cmt_rollup_driver_revert(struct cmt_rollup_driver *me)
{
	if (!me) return -EINVAL;
	if (ioctl(me->fd, IOCTL_ROLLUP_REJECT_INPUT))
		return -errno;
	return 0;
}

void *cmt_rollup_driver_get_tx(struct cmt_rollup_driver *me, size_t *max)
{
	if (max)
		*max = me->tx->length;
	return me->tx->data;
}

void *cmt_rollup_driver_get_rx(struct cmt_rollup_driver *me, size_t *max)
{
	if (max)
		*max = me->rx->length;
	return me->rx->data;
}
