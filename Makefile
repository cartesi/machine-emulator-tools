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
MINOR := 14
PATCH := 0
LABEL :=
VERSION := $(MAJOR).$(MINOR).$(PATCH)$(LABEL)

MACHINE_EMULATOR_TOOLS_VERSION ?= v$(VERSION)
MACHINE_EMULATOR_TOOLS_DEB     := machine-emulator-tools-$(MACHINE_EMULATOR_TOOLS_VERSION).deb
MACHINE_EMULATOR_TOOLS_IMAGE   := cartesi/machine-emulator-tools:$(MACHINE_EMULATOR_TOOLS_VERSION)

LINUX_SOURCES_VERSION  ?= 6.5.9-ctsi-1
LINUX_SOURCES_URLPATH  := https://github.com/cartesi/linux/archive/refs/tags/v$(LINUX_SOURCES_VERSION).tar.gz

all: $(MACHINE_EMULATOR_TOOLS_DEB)

build: Dockerfile control
	docker buildx build --platform=linux/riscv64 --load \
		--build-arg MACHINE_EMULATOR_TOOLS_DEB=$(MACHINE_EMULATOR_TOOLS_DEB) \
		--build-arg LINUX_SOURCES_VERSION=$(LINUX_SOURCES_VERSION) \
		--build-arg LINUX_SOURCES_URLPATH=$(LINUX_SOURCES_URLPATH) \
		-t $(MACHINE_EMULATOR_TOOLS_IMAGE) \
		-f $< \
		.

copy:
	ID=`docker create --platform=linux/riscv64 $(MACHINE_EMULATOR_TOOLS_IMAGE)` && \
	   docker cp $$ID:/opt/cartesi/$(MACHINE_EMULATOR_TOOLS_DEB)    . && \
	   docker rm $$ID

control: Makefile control.template
	sed 's|ARG_VERSION|$(VERSION)|g' control.template > control

$(MACHINE_EMULATOR_TOOLS_DEB): build copy

fs: $(MACHINE_EMULATOR_TOOLS_DEB)

env:
	@echo MACHINE_EMULATOR_TOOLS_DEB=$(MACHINE_EMULATOR_TOOLS_DEB)
	@echo MACHINE_EMULATOR_TOOLS_VERSION=$(MACHINE_EMULATOR_TOOLS_VERSION)
	@echo LINUX_SOURCES_VERSION=$(LINUX_SOURCES_VERSION)
	@echo LINUX_SOURCES_URLPATH=$(LINUX_SOURCES_URLPATH)

setup:
	docker run --privileged --rm  linuxkit/binfmt:bebbae0c1100ebf7bf2ad4dfb9dfd719cf0ef132

setup-required:
	@echo 'riscv64 buildx setup required:' `docker buildx ls | grep -q riscv64 && echo no || echo yes`

help:
	@echo 'available commands:'
	@echo '  setup           - setup riscv64 buildx'
	@echo '  setup-required  - check if riscv64 buildx setup is required'
	@echo '  help            - list makefile commands'
	@echo '  env             - print useful Makefile variables as a KEY=VALUE list'
	@echo '  clean           - remove the generated artifacts'
	@echo '  distclean       - clean and remove dependencies'

clean:
	rm -f $(MACHINE_EMULATOR_TOOLS_DEB) control

.PHONY: build copy fs env setup setup-required help distclean
