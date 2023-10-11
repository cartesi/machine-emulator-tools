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
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/cartesi/yield.h>
#include "yield-driver.h"

int cmt_yield_driver_init(struct cmt_yield_driver *me)
{
	if (!me) return -EINVAL;
	me->fd = open("/dev/yield", O_RDWR);
	if (me->fd < 0)
		return -errno;
	return 0;
}

void cmt_yield_driver_fini(struct cmt_yield_driver *me)
{
	if (!me) return;
	close(me->fd);

	me->fd = -1;
}

int cmt_yield_driver_progress(struct cmt_yield_driver *me, uint32_t progress)
{
	if (!me) return -EINVAL;
	struct yield_request req = {
		.dev    = (uint64_t)HTIF_DEVICE_YIELD,
		.cmd    = (uint64_t)HTIF_YIELD_AUTOMATIC,
		.reason = (uint64_t)HTIF_YIELD_REASON_PROGRESS,
		.data   = progress,
	};
	if (ioctl(me->fd, IOCTL_YIELD, (unsigned long)&req))
		return -errno; 
	return 0;
}
