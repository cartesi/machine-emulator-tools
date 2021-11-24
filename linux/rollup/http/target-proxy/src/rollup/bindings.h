/* Copyright 2021 Cartesi Pte. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */


// C interfaces that are converted to rust using bindgen https://github.com/rust-lang/rust-bindgen
// bindgen command to generate Rust file from rollup C interface:
// bindgen ./bindings.h -o ./bindings.rs --whitelist-var '^IOCTL.*' --whitelist-var '^CARTESI.*' --whitelist-type "^rollup_.*"\
// --whitelist-function '^rollup.*' -- -I/opt/riscv/riscv64-cartesi-linux-gnu/riscv64-cartesi-linux-gnu/sysroot/usr/include/linux/cartesi

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/cartesi/rollup.h>


int rollup_finish_request(int fd, struct rollup_finish *finish);
int rollup_read_advance_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *bytes, struct rollup_input_metadata *metadata);
int rollup_read_inspect_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *bytes);
int rollup_write_vouchers(int fd, unsigned count, uint8_t address[CARTESI_ROLLUP_ADDRESS_SIZE], struct rollup_bytes *bytes);
int rollup_write_notices(int fd, unsigned count, struct rollup_bytes *bytes);
int rollup_write_reports(int fd, unsigned count, struct rollup_bytes *bytes);

