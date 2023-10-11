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
#ifndef CMT_ROLLUP_DRIVER_H
#define CMT_ROLLUP_DRIVER_H
#include<stddef.h>
#include<stdint.h>

struct cmt_rollup_driver {
	int input_seq;  // input sequence number
	int output_seq; // output sequence number
	int report_seq; // report sequence number
	struct {
		void *data;
		size_t len;
	} tx[1], rx[1];
};
#include "libcmt/rollup-driver-api.h"
#endif /* CMT_ROLLUP_DRIVER_H */
