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
#include <stdio.h>
#include "yield-driver.h"

int cmt_yield_driver_init(struct cmt_yield_driver *me)
{
	if (!me) return -EINVAL;
	return 0;
}

void cmt_yield_driver_fini(struct cmt_yield_driver *me)
{
	if (!me) return;
}

int cmt_yield_driver_progress(struct cmt_yield_driver *me, uint32_t progress)
{
	if (!me) return -EINVAL;
	fprintf(stderr, "Progress: %u.%02u\n", progress / 10, progress % 10 * 10);
	return 0;
}
