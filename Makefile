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

MAJOR := 0
MINOR := 15
PATCH := 0
LABEL :=
VERSION := $(MAJOR).$(MINOR).$(PATCH)$(LABEL)

TOOLS_DEB    := machine-emulator-tools-v$(VERSION).deb
TOOLS_IMAGE  := cartesi/machine-emulator-tools:$(VERSION)
TOOLS_ROOTFS := rootfs-tools-v$(VERSION).ext2

IMAGE_KERNEL_VERSION ?= v0.20.0
LINUX_VERSION ?= 6.5.13-ctsi-1
LINUX_HEADERS_URLPATH := https://github.com/cartesi/image-kernel/releases/download/${IMAGE_KERNEL_VERSION}/linux-libc-dev-riscv64-cross-${LINUX_VERSION}-${IMAGE_KERNEL_VERSION}.deb

all: fs

build: control package.json
	@docker buildx build --load \
		--build-arg TOOLS_DEB=$(TOOLS_DEB) \
		--build-arg IMAGE_KERNEL_VERSION=$(IMAGE_KERNEL_VERSION) \
		--build-arg LINUX_VERSION=$(LINUX_VERSION) \
		--build-arg LINUX_HEADERS_URLPATH=$(LINUX_HEADERS_URLPATH) \
		-t $(TOOLS_IMAGE) \
		-f Dockerfile \
		. ;
	$(MAKE) copy

copy:
	@ID=`docker create $(TOOLS_IMAGE)` && \
	   docker cp $$ID:/opt/cartesi/$(TOOLS_DEB) . && \
	   docker rm $$ID

$(TOOLS_DEB) deb: build

control: Makefile control.in
	@sed 's|ARG_VERSION|$(VERSION)|g' control.in > control
package.json: Makefile package.json.in
	@sed 's|ARG_VERSION|$(VERSION)|g' package.json.in > package.json

$(TOOLS_ROOTFS) fs: $(TOOLS_DEB)
	@docker buildx build --platform=linux/riscv64 \
	  --build-arg TOOLS_DEB=$(TOOLS_DEB) \
	  --output type=tar,dest=rootfs.tar \
	  --file fs/Dockerfile \
	  . && \
	xgenext2fs -fzB 4096 -b 25600 -i 4096 -a rootfs.tar -L rootfs $(TOOLS_ROOTFS) && \
	rm -f rootfs.tar

libcmt:
	@docker buildx build --load \
		--target libcmt-debian-packager \
		--build-arg VERSION=$(VERSION) \
		-t $(TOOLS_IMAGE)-libcmt \
		-f Dockerfile \
		.
	$(MAKE) copy-libcmt

copy-libcmt:
	@mkdir libcmt
	@ID=`docker create $(TOOLS_IMAGE)-libcmt` && \
	   docker cp $$ID:/opt/cartesi/tools/sys-utils/libcmt/build/deb/ libcmt && \
	   docker rm $$ID

env:
	@echo VERSION=$(VERSION)
	@echo TOOLS_DEB=$(TOOLS_DEB)
	@echo TOOLS_ROOTFS=$(TOOLS_ROOTFS)
	@echo TOOLS_IMAGE=$(TOOLS_IMAGE)
	@echo IMAGE_KERNEL_VERSION=$(IMAGE_KERNEL_VERSION)
	@echo LINUX_VERSION=$(LINUX_VERSION)
	@echo LINUX_HEADERS_URLPATH=$(LINUX_HEADERS_URLPATH)

test:
	make -C sys-utils/libcmt/ test
	cd rollup-http/rollup-http-server && \
	MOCK_BUILD=true cargo test -- --show-output --test-threads=1

setup:
	@docker run --privileged --rm  linuxkit/binfmt:bebbae0c1100ebf7bf2ad4dfb9dfd719cf0ef132

setup-required:
	@echo 'riscv64 buildx setup required:' `docker buildx ls | grep -q riscv64 && echo no || echo yes`

build-tools-env:
	docker build --target tools-env -t $(TOOLS_IMAGE)-tools-env -f Dockerfile .

build-rust-builder-env:
	docker build --target rust-builder -t $(TOOLS_IMAGE)-rust-builder -f Dockerfile .

tools-env:
	@docker run --hostname rust-builder -it --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-v `pwd`:/opt/cartesi/machine-emulator-tools \
		-w /opt/cartesi/machine-emulator-tools \
		$(TOOLS_IMAGE)-tools-env /bin/bash


rust-builder-env:
	@docker run --hostname rust-builder -it --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-v `pwd`:/opt/cartesi/machine-emulator-tools \
		-w /opt/cartesi/machine-emulator-tools \
		$(TOOLS_IMAGE)-rust-builder /bin/bash

clean-image:
	@(docker rmi $(TOOLS_IMAGE) > /dev/null 2>&1 || true)

clean:
	@rm -f $(TOOLS_DEB) control rootfs* libcmt*
	@$(MAKE) -C sys-utils clean

distclean: clean clean-image

sys-utils:
	@$(MAKE) -C sys-utils

help:
	@echo 'available commands:'
	@echo '  deb             - build machine-emulator-tools .deb package'
	@echo '  fs              - build rootfs.ext2'
	@echo '  setup           - setup riscv64 buildx'
	@echo '  setup-required  - check if riscv64 buildx setup is required'
	@echo '  help            - list makefile commands'
	@echo '  env             - print useful Makefile variables as a KEY=VALUE list'
	@echo '  clean           - remove the generated artifacts'

.PHONY: build fs deb libcmt env setup setup-required help distclean
