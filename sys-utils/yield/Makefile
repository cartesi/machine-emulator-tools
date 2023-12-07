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
TOOLCHAIN_SUFFIX ?=

RVCC  = $(TOOLCHAIN_PREFIX)gcc$(TOOLCHAIN_SUFFIX)
RVCXX = $(TOOLCHAIN_PREFIX)g++$(TOOLCHAIN_SUFFIX)
RVCOPY = $(TOOLCHAIN_PREFIX)objcopy$(TOOLCHAIN_SUFFIX)
RVDUMP = $(TOOLCHAIN_PREFIX)objdump$(TOOLCHAIN_SUFFIX)
STRIP = $(TOOLCHAIN_PREFIX)strip
RISCV_CFLAGS :=-march=$(RISCV_ARCH) -mabi=$(RISCV_ABI)

CONTAINER_MAKE := /usr/bin/make
CONTAINER_BASE := /opt/cartesi/tools
KERNEL_HEADERS_PATH := /opt/riscv/usr/include

all: yield

yield.with-toolchain with-toolchain:
	$(MAKE) toolchain-exec CONTAINER_COMMAND="$(CONTAINER_MAKE) $@.toolchain"

extra.ext2.with-toolchain:
	$(MAKE) toolchain-exec CONTAINER_COMMAND="$(CONTAINER_MAKE) $@.toolchain"

yield: yield.c
	$(RVCC) -O2 -o yield yield.c
	$(STRIP) yield

extra.ext2: yield
	mkdir -m 0755 ./extra
	cp ./yield ./extra/yield
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
	@rm -rf yield extra.ext2 extra

.PHONY: toolchain-exec toolchain-env
