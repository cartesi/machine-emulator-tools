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
MINOR := 17
PATCH := 0
LABEL :=
VERSION := $(MAJOR).$(MINOR).$(PATCH)$(LABEL)

TOOLS_TARGZ  := machine-guest-tools_riscv64.tar.gz
TOOLS_IMAGE  := cartesi/machine-guest-tools:$(VERSION)
TOOLS_ROOTFS := rootfs-tools.ext2
TOOLS_ROOTFS_IMAGE := cartesi/rootfs-tools:$(VERSION)

LINUX_IMAGE_VERSION ?= v0.20.0
LINUX_VERSION ?= 6.5.13-ctsi-1
LINUX_HEADERS_URLPATH := https://github.com/cartesi/machine-linux-image/releases/download/${LINUX_IMAGE_VERSION}/linux-libc-dev-riscv64-cross-${LINUX_VERSION}-${LINUX_IMAGE_VERSION}.deb
LINUX_HEADERS_SHA256 := 2723435e8b45d8fb7a79e9344f6dc517b3dbc08e03ac17baab311300ec475c08

PREFIX ?= /usr
DESTDIR ?= .
CARGO ?= cargo
ARCH = $(shell uname -m)

ifneq ($(ARCH),riscv64)
all: build
else
all: build-riscv64 ## Build all tools
endif

build-riscv64: sys-utils rollup-http package.json ## Build riscv64 tools

build: targz fs ## Build targz and fs (cross compiling with Docker)

targz: ## Build tools .tar.gz (cross compiling with Docker)
	@docker buildx build --load \
		--build-arg TOOLS_TARGZ=$(TOOLS_TARGZ) \
		--build-arg LINUX_HEADERS_URLPATH=$(LINUX_HEADERS_URLPATH) \
		--build-arg LINUX_HEADERS_SHA256=$(LINUX_HEADERS_SHA256) \
		-t $(TOOLS_IMAGE) \
		-f Dockerfile \
		. ;
	$(MAKE) copy

copy:
	@ID=`docker create $(TOOLS_IMAGE)` && \
	   docker cp $$ID:/work/$(TOOLS_TARGZ) . && \
	   docker rm $$ID

package.json: Makefile package.json.in
	@sed 's|ARG_VERSION|$(VERSION)|g' package.json.in > package.json

fs: $(TOOLS_ROOTFS) ## Build tools rootfs image (cross compiling with Docker)

$(TOOLS_ROOTFS): $(TOOLS_TARGZ) fs/Dockerfile
	@docker buildx build --platform linux/riscv64 \
	  --build-arg TOOLS_TARGZ=$(TOOLS_TARGZ) \
	  --output type=tar,dest=rootfs.tar \
	  --file fs/Dockerfile \
	  . && \
	xgenext2fs -fzB 4096 -i 4096 -r +4096 -a rootfs.tar -L rootfs $(TOOLS_ROOTFS) && \
	rm -f rootfs.tar

fs-license: ## Build tools rootfs image HTML with license information
	@docker buildx build --load --platform linux/riscv64 \
	  --build-arg TOOLS_TARGZ=$(TOOLS_TARGZ) \
	  -t $(TOOLS_ROOTFS_IMAGE) \
	  --file fs/Dockerfile \
	  .
	TMPFILE=$$(mktemp) && (cd fs/third-party/repo-info/; ./scan-local.sh $(TOOLS_ROOTFS_IMAGE) linux/riscv64) | tee $$TMPFILE && pandoc -s -f markdown -t html5 -o $(TOOLS_ROOTFS).html $$TMPFILE && rm -f $$TMPFILE

env: ## Print useful Makefile information
	@echo VERSION=$(VERSION)
	@echo TOOLS_TARGZ=$(TOOLS_TARGZ)
	@echo TOOLS_ROOTFS=$(TOOLS_ROOTFS)
	@echo TOOLS_IMAGE=$(TOOLS_IMAGE)
	@echo TOOLS_ROOTFS_IMAGE=$(TOOLS_ROOTFS_IMAGE)
	@echo LINUX_IMAGE_VERSION=$(LINUX_IMAGE_VERSION)
	@echo LINUX_VERSION=$(LINUX_VERSION)
	@echo LINUX_HEADERS_URLPATH=$(LINUX_HEADERS_URLPATH)
	@echo LINUX_HEADERS_SHA256=$(LINUX_HEADERS_SHA256)

test: ## Test tools using mock builds
	make -C sys-utils/libcmt/ test
	cd rollup-http/rollup-http-server && \
	MOCK_BUILD=true $(CARGO) test -- --show-output --test-threads=1

setup: ## Setup riscv64 buildx
	@docker run --privileged --rm  linuxkit/binfmt:bebbae0c1100ebf7bf2ad4dfb9dfd719cf0ef132

setup-required: ## Check if riscv64 buildx setup is required
	@echo 'riscv64 buildx setup required:' `docker buildx ls | grep -q riscv64 && echo no || echo yes`

image: ## Build tools cross compilation Docker image
	docker build \
		--build-arg LINUX_HEADERS_URLPATH=$(LINUX_HEADERS_URLPATH) \
		--build-arg LINUX_HEADERS_SHA256=$(LINUX_HEADERS_SHA256) \
		--target tools-env -t $(TOOLS_IMAGE)-tools-env -f Dockerfile .

shell: ## Spawn a cross compilation shell with tools Docker image
	@docker run --hostname rust-builder -it --rm \
		-e USER=$$(id -u -n) \
		-e GROUP=$$(id -g -n) \
		-e UID=$$(id -u) \
		-e GID=$$(id -g) \
		-u $$(id -u):$$(id -g) \
		-v `pwd`:/work/machine-guest-tools \
		-w /work/machine-guest-tools \
		$(TOOLS_IMAGE)-tools-env /bin/bash

libcmt: ## Compile libcmt
	@$(MAKE) -C sys-utils/libcmt

sys-utils: libcmt ## Compile system utilities tools
	@$(MAKE) -C sys-utils

rollup-http: libcmt ## Compile rollup http tools
	@$(MAKE) -C rollup-http

install-share: package.json
	install -Dm644 package.json $(DESTDIR)$(PREFIX)/share/package.json

install: install-share ## Install all tools into ${DESTDIR}/${PREFIX}
	@$(MAKE) -C sys-utils/libcmt install
	@$(MAKE) -C sys-utils install
	@$(MAKE) -C rollup-http install

clean-image: ## Clean docker images
	@(docker rmi $(TOOLS_IMAGE) > /dev/null 2>&1 || true)

clean: ## Clean built files
	@rm -f $(TOOLS_TARGZ) rootfs*.ext2
	@$(MAKE) -C sys-utils/libcmt clean
	@$(MAKE) -C sys-utils clean
	@$(MAKE) -C rollup-http clean

distclean: clean clean-image ## Clean built files and docker images

help: ## Show this help
	@sed \
		-e '/^[a-zA-Z0-9_\-]*:.*##/!d' \
		-e 's/:.*##\s*/:/' \
		-e 's/^\(.\+\):\(.*\)/$(shell tput setaf 6)\1$(shell tput sgr0):\2/' \
		$(MAKEFILE_LIST) | column -c2 -t -s :

.PHONY: all build targz fs fs-license env test setup setup-required image shell libcmt sys-utils rollup-http install clean-image clean distclean help
