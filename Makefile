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
MINOR := 12
PATCH := 0
LABEL :=
VERSION := $(MAJOR).$(MINOR).$(PATCH)$(LABEL)

MACHINE_EMULATOR_TOOLS_VERSION ?= v$(VERSION)
MACHINE_EMULATOR_TOOLS_TAR_GZ  := machine-emulator-tools-$(MACHINE_EMULATOR_TOOLS_VERSION).tar.gz
MACHINE_EMULATOR_TOOLS_DEB     := machine-emulator-tools-$(MACHINE_EMULATOR_TOOLS_VERSION).deb
MACHINE_EMULATOR_TOOLS_IMAGE   := cartesi/machine-emulator-tools:$(MACHINE_EMULATOR_TOOLS_VERSION)

LINUX_SOURCES_VERSION  ?= 5.15.63-ctsi-2
LINUX_SOURCES_FILEPATH := dep/linux-$(LINUX_SOURCES_VERSION).tar.gz
LINUX_SOURCES_URLPATH  := https://github.com/cartesi/linux/archive/refs/tags/v$(LINUX_SOURCES_VERSION).tar.gz

RNDADDENTROPY_VERSION  ?= 3.0.0
RNDADDENTROPY_FILEPATH := dep/twuewand-$(RNDADDENTROPY_VERSION).tar.gz
RNDADDENTROPY_URLPATH  := https://www.finnie.org/software/twuewand/twuewand-$(RNDADDENTROPY_VERSION).tar.gz

SHASUMFILES := $(LINUX_SOURCES_FILEPATH) $(RNDADDENTROPY_FILEPATH)

all: build copy

build: Dockerfile checksum
	docker buildx build --platform=linux/riscv64 --load \
		--build-arg MACHINE_EMULATOR_TOOLS_TAR_GZ=$(MACHINE_EMULATOR_TOOLS_TAR_GZ) \
		--build-arg MACHINE_EMULATOR_TOOLS_DEB=$(MACHINE_EMULATOR_TOOLS_DEB) \
		--build-arg LINUX_SOURCES_VERSION=$(LINUX_SOURCES_VERSION) \
		--build-arg LINUX_SOURCES_FILEPATH=$(LINUX_SOURCES_FILEPATH) \
		--build-arg RNDADDENTROPY_VERSION=$(RNDADDENTROPY_VERSION) \
		--build-arg RNDADDENTROPY_FILEPATH=$(RNDADDENTROPY_FILEPATH) \
		-t $(MACHINE_EMULATOR_TOOLS_IMAGE) \
		-f $< \
		.

copy:
	ID=`docker create --platform=linux/riscv64 $(MACHINE_EMULATOR_TOOLS_IMAGE)` && \
	   docker cp $$ID:/opt/cartesi/$(MACHINE_EMULATOR_TOOLS_TAR_GZ) . && \
	   docker cp $$ID:/opt/cartesi/$(MACHINE_EMULATOR_TOOLS_DEB)    . && \
	   docker rm $$ID

deb:
	mkdir -p $(DESTDIR)/DEBIAN
	cat tools/template/control.template | sed 's|ARG_VERSION|$(VERSION)|g' > $(DESTDIR)/DEBIAN/control
	cp -R $(STAGING_BASE)* $(DESTDIR)$(PREFIX)
	dpkg-deb -Zxz --root-owner-group --build $(DESTDIR) $(MACHINE_EMULATOR_TOOLS_DEB)

$(LINUX_SOURCES_FILEPATH):
	T=`mktemp` && \
	    mkdir -p `dirname $@` &&  \
	    wget $(LINUX_SOURCES_URLPATH) -O $$T && \
	    mv $$T $@ || (rm $$T && false)

$(RNDADDENTROPY_FILEPATH):
	T=`mktemp` && \
	    mkdir -p `dirname $@` &&  \
	    wget $(RNDADDENTROPY_URLPATH) -O $$T && \
	    mv $$T $@ || (rm $$T && false)

shasumfile: $(SHASUMFILES)
	shasum -a 256 $^ > $@

checksum: $(SHASUMFILES)
	@shasum -ca 256 shasumfile

env:
	@echo MACHINE_EMULATOR_TOOLS_TAR_GZ=$(MACHINE_EMULATOR_TOOLS_TAR_GZ)
	@echo MACHINE_EMULATOR_TOOLS_DEB=$(MACHINE_EMULATOR_TOOLS_DEB)
	@echo MACHINE_EMULATOR_TOOLS_VERSION=$(MACHINE_EMULATOR_TOOLS_VERSION)
	@echo LINUX_SOURCES_VERSION=$(LINUX_SOURCES_VERSION)
	@echo LINUX_SOURCES_FILEPATH=$(LINUX_SOURCES_FILEPATH)
	@echo RNDADDENTROPY_VERSION=$(RNDADDENTROPY_VERSION)
	@echo RNDADDENTROPY_FILEPATH=$(RNDADDENTROPY_FILEPATH)

setup:
	docker run --privileged --rm  linuxkit/binfmt:bebbae0c1100ebf7bf2ad4dfb9dfd719cf0ef132

setup-required:
	@echo 'riscv64 buildx setup required:' `docker buildx ls | grep -q riscv64 && echo no || echo yes`

help:
	@echo 'available commands:'
	@echo '  setup           - setup riscv64 buildx'
	@echo '  setup-required  - check if riscv64 buildx setup is required'
	@echo '+ tgz             - create "$(MACHINE_EMULATOR_TOOLS_TAR_GZ)" (default)'
	@echo '  help            - list makefile commands'
	@echo '  checksum        - verify downloaded files'
	@echo '  shasumfile      - recreate shasumfile of downloaded files'
	@echo '  env             - print useful Makefile variables as a KEY=VALUE list'
	@echo '  clean           - remove the generated artifacts'
	@echo '  distclean       - clean and remove dependencies'

clean:
	rm -f $(MACHINE_EMULATOR_TOOLS_TAR_GZ) $(MACHINE_EMULATOR_TOOLS_DEB)

distclean: clean
	rm -rf dep

.PHONY: checksum env setup help
