# paths
PREFIX = /usr
TARGET_PREFIX ?= $(PREFIX)

TOOLCHAIN_PREFIX ?= riscv64-linux-gnu-
TARGET_CC := $(TOOLCHAIN_PREFIX)gcc
TARGET_AR := $(TOOLCHAIN_PREFIX)ar
TARGET_CFLAGS := -Wvla -O2 -g -Wall -pedantic -Wextra -Iinclude
CFLAGS := -Wvla -O2 -g -Wall -pedantic -Wextra -Iinclude

ALL := ioctl.build mock.build tools.build doc.build test.run
all: $(ALL)

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

examples_OBJDIR := .build/examples/
examples_OBJ    := $(patsubst %.c,$(examples_OBJDIR)%.o,$(examples_SRC))

$(examples_OBJ): $(examples_OBJDIR)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

# no need to link, just ensure examples build correctly
examples.build: $(examples_OBJ)

#-------------------------------------------------------------------------------
ioctl_SRC := \
	src/buf.c \
	src/abi.c \
	src/keccak.c \
	src/merkle.c \
	src/merkle-table.c \
	src/rollup.c \
	src/ioctl/io.c

ioctl_OBJDIR    := build/ioctl/
ioctl_OBJ       := $(patsubst %.c,$(ioctl_OBJDIR)%.o,$(ioctl_SRC))
ioctl_LIB       := $(ioctl_OBJDIR)libcmt.a

$(ioctl_OBJ): $(ioctl_OBJDIR)%.o: %.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(ioctl_LIB): $(ioctl_OBJ)
	$(TARGET_AR) rcs $@ $^

$(ioctl_OBJDIR)io_echo: src/tests/io_echo.c $(ioctl_LIB)
	$(TARGET_CC) $(TARGET_CFLAGS) -o $@ $^

$(ioctl_OBJDIR)rollup_echo: src/tests/rollup_echo.c $(ioctl_LIB)
	$(TARGET_CC) $(TARGET_CFLAGS) -o $@ $^

ioctl.build: $(ioctl_LIB) $(ioctl_OBJDIR)io_echo $(ioctl_OBJDIR)rollup_echo
ioctl.install: $(ioctl_LIB)
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	cp -f $< $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	cp -f include/libcmt/*.h $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib/pkgconfig
	sed \
		-e 's|@ARG_PREFIX@|$(TARGET_PREFIX)|g' \
		tools/templates/libcmt.pc > $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib/pkgconfig/libcmt.pc

#-------------------------------------------------------------------------------
mock_SRC := \
	src/abi.c \
	src/buf.c \
	src/keccak.c \
	src/merkle.c \
	src/merkle-table.c \
	src/rollup.c \
	src/mock/io.c

mock_OBJDIR := build/mock/
mock_OBJ    := $(patsubst %.c,$(mock_OBJDIR)%.o,$(mock_SRC))
mock_LIB    := $(mock_OBJDIR)libcmt.a

$(mock_OBJ): $(mock_OBJDIR)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(mock_LIB): $(mock_OBJ)
	$(AR) rcs $@ $^

$(mock_OBJDIR)io_echo: src/tests/io_echo.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)rollup_echo: src/tests/rollup_echo.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

mock.build: $(mock_LIB) $(mock_OBJDIR)io_echo $(mock_OBJDIR)rollup_echo

mock.install: $(mock_LIB)
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	cp -f $< $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include/libcmt/mock
	cp -f include/libcmt/*.h $(DESTDIR)$(PREFIX)/include/libcmt/
	mkdir -p $(DESTDIR)$(PREFIX)/lib/pkgconfig
	sed \
		-e 's|@ARG_PREFIX@|$(PREFIX)|g' \
		tools/templates/libcmt_mock.pc > $(DESTDIR)$(PREFIX)/lib/pkgconfig/libcmt.pc

#-------------------------------------------------------------------------------
testdata:
	cast calldata "EvmAdvance(address,uint256,uint256,uint256,bytes)" \
		`cast address-zero` 1 2 3 0xdeadbeef | xxd -r -p > dat/C.bin
	cast calldata "Voucher(address,bytes)" \
		`cast address-zero` 0xdeadbeef | xxd -r -p > dat/C.v0.bin
	cast calldata "Notice(bytes)" \
		0xdeadbeef | xxd -r -p > dat/C.n0.bin
	xxd -n advance -i dat/C.bin    > dat/C.h
	xxd -n voucher -i dat/C.v0.bin > dat/C.v0.h
	xxd -n notice  -i dat/C.n0.bin > dat/C.n0.h

unittests_BINS := \
	$(mock_OBJDIR)abi-multi \
	$(mock_OBJDIR)abi-single \
	$(mock_OBJDIR)keccak \
	$(mock_OBJDIR)merkle \

$(mock_OBJDIR)abi-multi: src/tests/abi-multi.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)abi-single: src/tests/abi-single.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)keccak: src/tests/keccak.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

$(mock_OBJDIR)merkle: src/tests/merkle.c $(mock_LIB)
	$(CC) $(CFLAGS) -o $@ $^

test.build: $(unittests_BINS) $(mock_OBJDIR)io_echo
test.run: test.build
	$(foreach test,$(unittests_BINS),$(test) &&) true

#-------------------------------------------------------------------------------
tools_OBJDIR := build/tools/

tools_BINS := \
	$(tools_OBJDIR)funsel \
	$(tools_OBJDIR)merkle-table

$(tools_OBJDIR)funsel: src/tools/funsel.c $(mock_LIB)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

$(tools_OBJDIR)merkle-table: src/tools/merkle-table.c src/keccak.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

# generated source code
src/merkle.c: src/merkle-table.c
src/merkle-table.c: $(tools_OBJDIR)merkle-table
	$< > $@

tools.build: $(tools_BINS)

#-------------------------------------------------------------------------------

help:
	@echo "Targets: (default: '*')"
	@echo "* all            - $(ALL)"
	@echo "  mock.build     - Build a mocked version of the library, tools and examples; to run on the host system."
	@echo "  mock.install   - Install the mocked version of the library and C headers; on the host system."
	@echo "                   Use DESTDIR and PREFIX to customize the installation."
	@echo "  ioctl.build    - Build the library, tools and examples; to run on the cartesi-machine."
	@echo "                   (requires the cartesi Linux headers to build)"
	@echo "  ioctl.install  - Install the library and C headers; on the host system."
	@echo "                   Use TARGET_DESTDIR and TARGET_PREFIX to customize the installation."
	@echo "  test.build     - Build tests on top of the mocked library to run on the host system."
	@echo "  test.run       - Execute the tests."
	@echo "  tools.build    - Build tools on top of the mocked library to run on the host system."
	@echo "  doc.build      - Build the documentation and API references as html."
	@echo "  clean          - remove the binaries and objects."

doc/theme:
	git clone git@github.com:jothepro/doxygen-awesome-css.git $@
	git -C doc/theme checkout 8cea9a073ecd50a5b2c0958a3df100292d6c7374

doc.build: doc/theme examples.build
	doxygen doc/Doxyfile

OBJ := $(mock_OBJ) $(ioctl_OBJ) $(examples_OBJ) $(tools_OBJ)
clean:
	rm -rf $(OBJ) $(unittests_BINS) $(tools_BINS) libcmt_mock.a libcmt.a

-include $(OBJ:%.o=%.d)
