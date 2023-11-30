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

/* C interfaces that are converted to rust using bindgen (https://github.com/rust-lang/rust-bindgen)
 * command to generate Rust file from rollup C interface. In case that C rollup bindings are updated,
 * `bindings.rs` file needs to be regenerated using following procedure:
 * Add toolchain `riscv64-cartesi-linux-gnu` to the shell execution path
 * $ cd linux/rollup/http/http-dispatcher/src/rollup
 * $ bindgen ./bindings.h -o ./bindings.rs --whitelist-var '^IOCTL.*' --whitelist-var '^CARTESI.*' --whitelist-type
 "^rollup_.*" --whitelist-function '^rollup.*'\
     -- --sysroot=/opt/riscv/riscv64-cartesi-linux-gnu/riscv64-cartesi-linux-gnu/sysroot
 --target=riscv64-cartesi-linux-gnu -march=rv64gc -mabi=lp64d
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <linux/cartesi/rollup.h>
#include <linux/types.h>

int rollup_finish_request(int fd, struct rollup_finish *finish, bool accept);
int rollup_read_advance_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *bytes,
    struct rollup_input_metadata *metadata);
int rollup_read_inspect_state_request(int fd, struct rollup_finish *finish, struct rollup_bytes *query);
int rollup_write_voucher(int fd, uint8_t destination[CARTESI_ROLLUP_ADDRESS_SIZE], struct rollup_bytes *bytes,
    uint64_t *voucher_index);
int rollup_write_notice(int fd, struct rollup_bytes *bytes, uint64_t *notice_index);
int rollup_write_report(int fd, struct rollup_bytes *bytes);
int rollup_throw_exception(int fd, struct rollup_bytes *bytes);
