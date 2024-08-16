# Copyright Cartesi and individual authors (see AUTHORS)
#
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
PREFIX = /usr
TARGET_PREFIX ?= $(PREFIX)

TOOLCHAIN_PREFIX ?= riscv64-linux-gnu-
TARGET_CC := $(TOOLCHAIN_PREFIX)gcc
TARGET_AR := $(TOOLCHAIN_PREFIX)ar
COMMON_CFLAGS := -Wvla -O2 -g -Wall -pedantic -Wextra -Iinclude \
                 -fno-strict-aliasing -fno-strict-overflow -fPIC
TARGET_CFLAGS := $(COMMON_CFLAGS) -ftrivial-auto-var-init=zero -Wstrict-aliasing=3
CFLAGS := $(COMMON_CFLAGS)
CC := gcc

all: libcmt host
host: mock tools
#-------------------------------------------------------------------------------
examples_SRC := \
	doc/examples/abi_encode_000.c \
	doc/examples/abi_encode_001.c \
	doc/examples/abi_encode_002.c \
	doc/examples/abi_decode_000.c \
	doc/examples/abi_decode_001.c \
	doc/examples/abi_decode_002.c \
	doc/examples/io.c \
	doc/examples/rollup.c

examples_OBJDIR := build/examples
examples_OBJ    := $(patsubst %.c,$(examples_OBJDIR)/%.o,$(examples_SRC))

$(examples_OBJ): $(examples_OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

# no need to link, just ensure examples build correctly
examples: $(examples_OBJ)

#-------------------------------------------------------------------------------
libcmt_SRC := \
	src/buf.c \
	src/abi.c \
	src/keccak.c \
	src/merkle.c \
	src/rollup.c \
	src/util.c \
	src/io.c

libcmt_OBJDIR    := build/riscv64
libcmt_OBJ       := $(patsubst %.c,$(libcmt_OBJDIR)/%.o,$(libcmt_SRC))
libcmt_LIB       := $(libcmt_OBJDIR)/libcmt.a
libcmt_SO        := $(libcmt_OBJDIR)/libcmt.so

$(libcmt_OBJ): $(libcmt_OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(libcmt_LIB): $(libcmt_OBJ)
	$(TARGET_AR) rcs $@ $^

$(libcmt_SO): $(libcmt_OBJ)
	$(TARGET_CC) -shared -o $@ $^

libcmt: $(libcmt_LIB) $(libcmt_SO)
install-run: $(libcmt_SO)
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	cp -f $(libcmt_SO) $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib

install: $(libcmt_LIB) build/ffi.h
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	cp -f $(libcmt_LIB) $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	cp -f include/libcmt/*.h $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	cp -f build/ffi.h $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib/pkgconfig
	sed -e 's|@PREFIX@|$(TARGET_PREFIX)|g' \
	    tools/libcmt.pc.in > $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib/pkgconfig/libcmt.pc

# requires either install-run or install
debian-package:
	mkdir -p $(dir $(DEB_FILENAME))
	dpkg-deb -Zxz --root-owner-group --build $(TARGET_DESTDIR) $(DEB_FILENAME)

$(TARGET_DESTDIR)/DEBIAN/control: tools/control.in
	mkdir -p $(@D)
	sed -e 's|@PACKAGE@|$(ARG_PACKAGE)|' \
	    -e 's|@VERSION@|$(ARG_VERSION)|' \
	    -e 's|@ARCHITECTURE@|$(ARG_ARCHITECTURE)|' \
	    -e 's|@SECTION@|$(ARG_SECTION)|' \
	    -e 's|@PROVIDES@|$(ARG_PROVIDES)|' \
	    -e 's|@DESCRIPTION@|$(ARG_DESCRIPTION)|' \
	    $< > $@

# debian package variants
libcmt-v$(ARG_VERSION).deb:
	$(MAKE) $(TARGET_DESTDIR)/DEBIAN/control \
	        ARG_PACKAGE=libcmt \
	        ARG_ARCHITECTURE=riscv64 \
	        ARG_PROVIDES=libcmt \
	        ARG_SECTION=libs \
	        ARG_DESCRIPTION="Cartesi Machine Tools"
	$(MAKE) debian-package DEB_FILENAME=build/deb/$@

libcmt-dev-v$(ARG_VERSION).deb:
	$(MAKE) $(TARGET_DESTDIR)/DEBIAN/control \
	        ARG_PACKAGE=libcmt-dev \
	        ARG_ARCHITECTURE=riscv64 \
	        ARG_PROVIDES=libcmt-dev \
	        ARG_SECTION=devel \
	        ARG_DESCRIPTION="Cartesi Machine Tools - (development files)"
	$(MAKE) debian-package DEB_FILENAME=build/deb/$@

libcmt-dev-riscv64-cross-v$(ARG_VERSION).deb:
	$(MAKE) $(TARGET_DESTDIR)/DEBIAN/control \
	        ARG_PACKAGE=libcmt-riscv64-cross \
	        ARG_ARCHITECTURE=all \
	        ARG_PROVIDES=libcmt-dev-riscv64-cross \
	        ARG_SECTION=devel \
	        ARG_DESCRIPTION="Cartesi Machine Tools - (development files) (riscv64)"
	$(MAKE) debian-package DEB_FILENAME=build/deb/$@

#-------------------------------------------------------------------------------
mock_SRC := \
	src/abi.c \
	src/buf.c \
	src/keccak.c \
	src/merkle.c \
	src/rollup.c \
	src/util.c \
	src/io-mock.c

mock_OBJDIR := build/mock
mock_OBJ    := $(patsubst %.c,$(mock_OBJDIR)/%.o,$(mock_SRC))
mock_LIB    := $(mock_OBJDIR)/libcmt.a
mock_SO     := $(mock_OBJDIR)/libcmt.so

$(mock_OBJ): $(mock_OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(mock_LIB): $(mock_OBJ)
	$(AR) rcs $@ $^

$(mock_SO): $(mock_OBJ)
	$(CC) -shared -o $@ $^

mock: $(mock_LIB) $(mock_SO)

install-mock: $(mock_LIB) $(mock_SO) build/ffi.h
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	cp -f $(mock_LIB) $(mock_SO) $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include/libcmt/
	cp -f include/libcmt/*.h $(DESTDIR)$(PREFIX)/include/libcmt/
	cp -f build/ffi.h $(DESTDIR)$(PREFIX)/include/libcmt/
	mkdir -p $(DESTDIR)$(PREFIX)/lib/pkgconfig
	sed -e 's|@ARG_PREFIX@|$(PREFIX)|g' tools/libcmt.pc.in > $(DESTDIR)$(PREFIX)/lib/pkgconfig/libcmt.pc

#-------------------------------------------------------------------------------
unittests_BINS := \
	$(mock_OBJDIR)/abi-multi \
	$(mock_OBJDIR)/abi-single \
	$(mock_OBJDIR)/buf \
	$(mock_OBJDIR)/gio \
	$(mock_OBJDIR)/keccak \
	$(mock_OBJDIR)/merkle \
	$(mock_OBJDIR)/progress \
	$(mock_OBJDIR)/rollup

$(mock_OBJDIR)/abi-multi: tests/abi-multi.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/abi-single: tests/abi-single.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/buf: tests/buf.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/keccak: tests/keccak.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/merkle: tests/merkle.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/gio: tests/gio.c tests/data.h $(mock_LIB)
	$(CC) -Itests $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/rollup: tests/rollup.c tests/data.h $(mock_LIB)
	$(CC) -Itests $(CFLAGS) -o $@ $^

$(mock_OBJDIR)/progress: tests/progress.c $(mock_LIB)
	$(CC) -Itests $(CFLAGS) -o $@ $^

test: $(unittests_BINS)
	$(foreach test,$(unittests_BINS),$(test) &&) true

tests/data.h: tests/create-data.sh
	$< > $@

#-------------------------------------------------------------------------------
tools_OBJDIR := build/tools
tools_BINS := \
	$(tools_OBJDIR)/funsel

$(tools_OBJDIR)/funsel: tools/funsel.c $(mock_LIB)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

tools: $(tools_BINS)

HDRS := $(patsubst %,include/libcmt/%, buf.h abi.h keccak.h merkle.h io.h rollup.h)
build/ffi.h: $(HDRS)
	cat $^ | sh tools/prepare-ffi.sh > $@
#-------------------------------------------------------------------------------
LINTER_IGNORE_SOURCES=src/io.c
LINTER_IGNORE_HEADERS=
LINTER_SOURCES=$(filter-out $(LINTER_IGNORE_SOURCES),$(strip $(wildcard src/*.c) $(wildcard tests/*.c) $(wildcard tools/*.c)))
LINTER_HEADERS=$(filter-out $(LINTER_IGNORE_HEADERS),$(strip $(wildcard src/*.h)))

CLANG_TIDY=clang-tidy
CLANG_TIDY_TARGETS=$(patsubst %.c,%.clang-tidy,$(LINTER_SOURCES))

CLANG_FORMAT=clang-format
CLANG_FORMAT_FILES:=$(wildcard src/*.c) $(wildcard src/*.h) $(wildcard tests/*.c) $(wildcard tools/*.c)
CLANG_FORMAT_IGNORE_FILES:=
CLANG_FORMAT_FILES:=$(strip $(CLANG_FORMAT_FILES))
CLANG_FORMAT_FILES:=$(filter-out $(CLANG_FORMAT_IGNORE_FILES),$(strip $(CLANG_FORMAT_FILES)))

EMPTY:=
SPACE:=$(EMPTY) $(EMPTY)
CLANG_TIDY_HEADER_FILTER=$(CURDIR)/($(subst $(SPACE),|,$(LINTER_HEADERS)))

%.clang-tidy: %.c
	@$(CLANG_TIDY) --header-filter='$(CLANG_TIDY_HEADER_FILTER)' $< -- $(CFLAGS) 2>/dev/null
	@$(CC) $(CFLAGS) $< -MM -MT $@ -MF $@.d > /dev/null 2>&1
	@touch $@

clangd-config:
	@echo "$(CFLAGS)" | sed -e $$'s/ \{1,\}/\\\n/g' | grep -v "MMD" > compile_flags.txt

format:
	@$(CLANG_FORMAT) -i $(CLANG_FORMAT_FILES)

check-format:
	@$(CLANG_FORMAT) -Werror --dry-run $(CLANG_FORMAT_FILES)

lint: $(CLANG_TIDY_TARGETS)

#-------------------------------------------------------------------------------

help:
	@echo "Targets: (default: '*')"
	@echo "* all          - Build libcmt and host targets"
	@echo "  host         - Build mock and tools targets"
	@echo "  libcmt       - Build the library, tools and examples; to run on the cartesi-machine."
	@echo "                 (requires the cartesi Linux headers to build)"
	@echo "  mock         - Build a mocked version of the library, tools and examples; to run on the host system."
	@echo "  tools        - Build tools on top of the mocked library to run on the host system."
	@echo "  test         - Build and run tests on top of the mocked library on the host system."
	@echo "  doc          - Build the documentation and API references as html."
	@echo "  clean        - remove the binaries and objects."
	@echo "  install      - Install the library and C headers; on the host system."
	@echo "                 Use TARGET_DESTDIR and TARGET_PREFIX to customize the installation."
	@echo "  install-mock - Install the mocked version of the library and C headers; on the host system."
	@echo "                 Use DESTDIR and PREFIX to customize the installation."

doc/theme:
	git clone git@github.com:jothepro/doxygen-awesome-css.git $@
	git -C doc/theme checkout 8cea9a073ecd50a5b2c0958a3df100292d6c7374

doc: doc/theme examples
	doxygen doc/Doxyfile

clean:
	@rm -rf build
	@rm -rf src/*.clang-tidy src/*.d
	@rm -rf tests/*.clang-tidy tests/*.d
	@rm -rf tools/*.clang-tidy tools/*.d
	@rm -rf *.bin

distclean: clean
	@rm -rf doc/html doc/theme
	@rm -rf compile_flags.txt

OBJ := $(mock_OBJ) $(libcmt_OBJ) $(examples_OBJ) $(tools_OBJ)

.PHONY: install
-include $(OBJ:%.o=%.d)
