# Copyright Cartesi and individual authors (see AUTHORS)
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TOOLCHAIN_PREFIX ?= riscv64-linux-gnu-
PKG_CONFIG_PATH = /usr/riscv64-linux-gnu/lib/pkgconfig
export PKG_CONFIG_PATH

CC      = $(TOOLCHAIN_PREFIX)gcc
STRIP   = $(TOOLCHAIN_PREFIX)strip
CFLAGS  = -Wall -Wextra -pedantic -O2 `pkg-config --cflags libcmt`
LDFLAGS = `pkg-config --libs libcmt`

all: ioctl-echo-loop

ioctl-echo-loop: ioctl-echo-loop.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	$(STRIP) $@

clean:
	@rm -f ioctl-echo-loop
