# Copyright 2019 Cartesi Pte. Ltd.
#
# This file is part of the machine-emulator. The machine-emulator is free
# software: you can redistribute it and/or modify it under the terms of the GNU
# Lesser General Public License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# The machine-emulator is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with the machine-emulator. If not, see http://www.gnu.org/licenses/.
#

UNAME:=$(shell uname)

TOOLCHAIN_TAG ?= 0.2.0

RVCC  = riscv64-unknown-linux-gnu-gcc
RVCXX = riscv64-unknown-linux-gnu-g++
RVCOPY = riscv64-unknown-linux-gnu-objcopy
RVDUMP = riscv64-unknown-linux-gnu-objdump
RISCV_CFLAGS :=-march=rv64ima -mabi=lp64

CONTAINER_MAKE := /usr/bin/make
CONTAINER_BASE := /opt/cartesi/tools

extra/htif: htif.c
	$(MAKE) toolchain-exec CONTAINER_COMMAND="$(CONTAINER_MAKE) $@.toolchain"

extra.ext2: extra/htif extra/yield-test
	$(MAKE) toolchain-exec CONTAINER_COMMAND="$(CONTAINER_MAKE) $@.toolchain"

extra/htif.toolchain:
	$(RVCC) -O2 -o extra/htif htif.c

extra.ext2.toolchain:
	genext2fs -i 512 -b 8192 -d extra $(basename $@)

toolchain-exec:
	@docker run --hostname $@ --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-v `pwd`:$(CONTAINER_BASE) \
		-w $(CONTAINER_BASE) \
		cartesi/toolchain:$(TOOLCHAIN_TAG) $(CONTAINER_COMMAND)

toolchain-env:
	@docker run --hostname toolchain-env -it --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-v `pwd`:$(CONTAINER_BASE) \
		-w $(CONTAINER_BASE) \
		cartesi/toolchain:$(TOOLCHAIN_TAG)

clean:
	\rm -rf extra/htif extra.ext2

.PHONY: toolchain-exec toolchain-env
