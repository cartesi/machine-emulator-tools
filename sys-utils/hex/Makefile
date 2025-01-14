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

UNAME:=$(shell uname)

TOOLCHAIN_IMAGE ?= cartesi/toolchain
TOOLCHAIN_TAG ?= 0.16.0
RISCV_ARCH ?= rv64gc
RISCV_ABI ?= lp64d

TOOLCHAIN_PREFIX ?= riscv64-cartesi-linux-gnu-

RVCC  = $(TOOLCHAIN_PREFIX)gcc
RVCXX = $(TOOLCHAIN_PREFIX)g++
RVCOPY = $(TOOLCHAIN_PREFIX)objcopy
RVDUMP = $(TOOLCHAIN_PREFIX)objdump
RISCV_CFLAGS :=-march=$(RISCV_ARCH) -mabi=$(RISCV_ABI)

CONTAINER_MAKE := /usr/bin/make
CONTAINER_BASE := /opt/cartesi/tools
KERNEL_HEADERS_PATH := /opt/riscv/usr/include

all: hex

hex.with-toolchain with-toolchain:
	$(MAKE) toolchain-exec CONTAINER_COMMAND="$(CONTAINER_MAKE) $@.toolchain"

extra.ext2.with-toolchain:
	$(MAKE) toolchain-exec CONTAINER_COMMAND="$(CONTAINER_MAKE) $@.toolchain"

hex: hex.cpp
	$(RVCXX) -O2 -o hex hex.cpp

extra.ext2: hex
	mkdir -m 0755 ./extra
	cp ./hex ./extra/hex
	xgenext2fs -i 512 -b 8192 -d extra $(basename $@)
	rm -rf ./extra

toolchain-exec:
	@docker run --hostname $@ --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-v `pwd`:$(CONTAINER_BASE) \
		-w $(CONTAINER_BASE) \
		$(TOOLCHAIN_IMAGE):$(TOOLCHAIN_TAG) $(CONTAINER_COMMAND)

toolchain-env:
	@docker run --hostname toolchain-env -it --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-v `pwd`:$(CONTAINER_BASE) \
		-w $(CONTAINER_BASE) \
		$(TOOLCHAIN_IMAGE):$(TOOLCHAIN_TAG)

clean:
	@rm -rf hex extra.ext2 extra

.PHONY: toolchain-exec toolchain-env
