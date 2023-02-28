MACHINE_EMULATOR_TOOLS_VERSION ?= v0.10.0
MACHINE_EMULATOR_TOOLS_TAR_GZ  := machine-emulator-tools-$(MACHINE_EMULATOR_TOOLS_VERSION).tar.gz

LINUX_SOURCES_VERSION  ?= 5.15.63-ctsi-1
LINUX_SOURCES_FILEPATH := dep/linux-sources-$(LINUX_SOURCES_VERSION).tar.gz
LINUX_SOURCES_URLPATH  := https://github.com/cartesi/linux/archive/refs/tags/v$(LINUX_SOURCES_VERSION).tar.gz

RNDADDENTROPY_VERSION  ?= 3.0.0
RNDADDENTROPY_FILEPATH := dep/twuewand-$(RNDADDENTROPY_VERSION).tar.gz
RNDADDENTROPY_URLPATH  := https://www.finnie.org/software/twuewand/twuewand-$(RNDADDENTROPY_VERSION).tar.gz

SHASUMFILES := $(LINUX_SOURCES_FILEPATH) $(RNDADDENTROPY_FILEPATH)

all: tgz
tgz: $(MACHINE_EMULATOR_TOOLS_TAR_GZ)

$(MACHINE_EMULATOR_TOOLS_TAR_GZ): Dockerfile checksum
	docker buildx build --platform=linux/riscv64 \
		--build-arg MACHINE_EMULATOR_TOOLS_TAR_GZ=$(MACHINE_EMULATOR_TOOLS_TAR_GZ) \
		--build-arg LINUX_SOURCES_VERSION=$(LINUX_SOURCES_VERSION) \
		--build-arg LINUX_SOURCES_FILEPATH=$(LINUX_SOURCES_FILEPATH) \
		--build-arg RNDADDENTROPY_VERSION=$(RNDADDENTROPY_VERSION) \
		--build-arg RNDADDENTROPY_FILEPATH=$(RNDADDENTROPY_FILEPATH) \
		-t cartesi/tools:22.04 \
		-f $< \
		.
	ID=`docker create --platform=linux/riscv64 cartesi/tools:22.04` && \
	   docker cp $$ID:/opt/cartesi/$(MACHINE_EMULATOR_TOOLS_TAR_GZ) . && \
	   docker rm $$ID

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
	@echo LINUX_SOURCES_VERSION=$(LINUX_SOURCES_VERSION)
	@echo MACHINE_EMULATOR_TOOLS_TAR_GZ=$(MACHINE_EMULATOR_TOOLS_TAR_GZ)

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
	rm -f $(MACHINE_EMULATOR_TOOLS_TAR_GZ)

distclean: clean
	rm -rf dep

.PHONY: checksum env setup help
