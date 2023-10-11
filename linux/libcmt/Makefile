# paths
PREFIX = /usr
TARGET_PREFIX ?= $(PREFIX)

TARGET_CC := $(CROSS_COMPILE)gcc
TARGET_AR := $(CROSS_COMPILE)ar
TARGET_CFLAGS := -Wvla -O2 -g -Wall -pedantic -Wextra -Iinclude -Iinclude/libcmt/ioctl
CFLAGS := -Wvla -O2 -g -Wall -pedantic -Wextra -fsanitize=address,undefined -Iinclude -Iinclude/libcmt/mock

mock_BINS :=  build/libcmt_mock/yield-driver \
              build/libcmt_mock/rollup-driver \
              build/libcmt_mock/rollup \
              build/libcmt_mock/merkle-state \
              libcmt_mock.a

ioctl_BINS := build/libcmt/yield-driver \
              build/libcmt/rollup-driver \
              build/libcmt/rollup \
              libcmt.a

tests_BINS := build/libcmt_mock/abi \
              build/libcmt_mock/keccak \
              build/libcmt_mock/merkle

tools_BINS := tools/funsel \
              tools/merkle-table

all: mock.build ioctl.build tools.build examples.build
help:
	@echo "Cleaning targets: (default: '*')"
	@echo "  clean          - remove the binaries and objects."
	@echo "* mock.build     - build the reference mock library and examples for running in host mode."
	@echo "  mock.install   - install the reference mock library."
	@echo "* ioctl.build    - build the riscv ioctl library and examples."
	@echo "                   (needs the cartesi linux headers. accepts the 'CROSS_COMPILE=' variable)"
	@echo "  ioctl.install  - install the riscv ioctl library."
	@echo "  tests.build    - build host mode tests."
	@echo "  tests.run      - run host mode tests."
	@echo "* tools.build    - development tools and utilities."
	@echo "  doc.build      - build the doxygen documentation."
	@echo "  examples.build - build the documentation code."

base_SRC := \
	src/buf.c \
	src/abi.c \
	src/keccak.c \
	src/merkle.c \
	src/rollup.c \
	src/merkle-table.c

libcmt_mock_SRC := \
	src/mock/rollup-driver.c \
	src/mock/yield-driver.c \
	$(base_SRC)

libcmt_SRC := \
	src/ioctl/rollup-driver.c \
	src/ioctl/yield-driver.c \
	$(base_SRC)

examples_SRC := \
	examples/abi_decode_000.c \
	examples/abi_decode_001.c \
	examples/abi_encode_000.c \
	examples/abi_encode_001.c \
	examples/abi_encode_002.c \
	examples/abi_get.c \
	examples/abi_multi.c \
	examples/rollup-driver.c \
	examples/rollup.c \
	examples/yield-driver.c

libcmt_mock_OBJDIR  := build/libcmt_mock/
libcmt_OBJDIR := build/libcmt/
examples_OBJDIR := build/examples/

libcmt_mock_OBJ  := $(patsubst %.c,$(libcmt_mock_OBJDIR)%.o,$(libcmt_mock_SRC))
libcmt_OBJ := $(patsubst %.c,$(libcmt_OBJDIR)%.o,$(libcmt_SRC))
examples_OBJ := $(patsubst %.c,$(examples_OBJDIR)%.o,$(examples_SRC))

OBJ := $(libcmt_mock_OBJ) $(libcmt_OBJ) $(examples_OBJ)

mock.build:  libcmt_mock.a $(mock_BINS)
ioctl.build: libcmt.a $(ioctl_BINS)
tests.build: $(tests_BINS)
tools.build: $(tools_BINS)
tests.run: tests.build
	$(foreach test,$(tests_BINS),$(test) &&) true
examples.build: $(examples_OBJ)

$(libcmt_mock_OBJ): $(libcmt_mock_OBJDIR)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(libcmt_OBJ): $(libcmt_OBJDIR)%.o: %.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(examples_OBJ): $(examples_OBJDIR)%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

libcmt_mock.a: $(libcmt_mock_OBJ)
	$(AR) rcs $@ $^

libcmt.a: $(libcmt_OBJ)
	$(TARGET_AR) rcs $@ $^

build/libcmt/yield-driver: examples/yield-driver.c libcmt.a
	$(TARGET_CC) $(TARGET_CFLAGS) -o $@ $^
build/libcmt/rollup-driver: examples/rollup-driver.c libcmt.a
	$(TARGET_CC) $(TARGET_CFLAGS) -o $@ $^
build/libcmt/rollup: examples/rollup.c libcmt.a
	$(TARGET_CC) $(TARGET_CFLAGS) -o $@ $^

build/libcmt_mock/yield-driver: examples/yield-driver.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^
build/libcmt_mock/rollup-driver: examples/rollup-driver.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^
build/libcmt_mock/rollup: examples/rollup.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^
build/libcmt_mock/merkle-state: examples/merkle-state.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^

build/libcmt_mock/abi: tests/abi.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^
build/libcmt_mock/keccak: tests/keccak.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^
build/libcmt_mock/merkle: tests/merkle.c libcmt_mock.a
	$(CC) $(CFLAGS) -o $@ $^

src/merkle.c: src/merkle-table.c
src/merkle-table.c: tools/merkle-table
	$< > $@

tools/merkle-table: tools/merkle-table.c src/keccak.c
	$(CC) $(CFLAGS) -o $@ $^
tools/funsel: tools/funsel.c src/keccak.c
	$(CC) $(CFLAGS) -o $@ $^

doc/theme:
	git clone git@github.com:jothepro/doxygen-awesome-css.git $@
	git -C doc/theme checkout 8cea9a073ecd50a5b2c0958a3df100292d6c7374

doc.build: examples.build doc/theme
	doxygen doc/Doxyfile

ioctl.install: libcmt.a
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	cp -f $< $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	cp -f include/libcmt/*.h $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	cp -f include/libcmt/ioctl/*.h $(TARGET_DESTDIR)$(TARGET_PREFIX)/include/libcmt/
	mkdir -p $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib/pkgconfig
	sed 's|ARG_PREFIX|$(TARGET_PREFIX)|g' tools/templates/libcmt.pc \
		> $(TARGET_DESTDIR)$(TARGET_PREFIX)/lib/pkgconfig/libcmt.pc

mock.install: libcmt_mock.a
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	cp -f $< $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include/libcmt/mock
	cp -f include/libcmt/*.h $(DESTDIR)$(PREFIX)/include/libcmt/
	cp -f include/libcmt/mock/*.h $(DESTDIR)$(PREFIX)/include/libcmt/mock/
	mkdir -p $(DESTDIR)$(PREFIX)/lib/pkgconfig
	sed 's|ARG_PREFIX|$(PREFIX)|g' tools/templates/libcmt_mock.pc \
		> $(DESTDIR)$(PREFIX)/lib/pkgconfig/libcmt_mock.pc

doc.install: doc.build
	mkdir -p $(DESTDIR)$(PREFIX)/share/libcmt/html
	cp -f doc/html $(DESTDIR)$(PREFIX)/share/libcmt/html

clean:
	rm -f $(mock_BINS) $(ioctl_BINS) $(tests_BINS) $(tools_BINS) $(OBJ) $(OBJ:%.o=%.d)
	rm -rf doc/html
distclean:
	rm -rf doc/theme
.PHONY: examples.build doc

-include $(OBJ:%.o=%.d)
