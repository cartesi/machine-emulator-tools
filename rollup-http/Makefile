PREFIX ?= /usr
CARGO ?= cargo
CARGO_FLAGS += --release
ARCH = $(shell uname -m)

ifneq ($(ARCH),riscv64)
RUST_TARGET_DIR ?= target/riscv64gc-unknown-linux-gnu/release
RUST_TARGET = --target riscv64gc-unknown-linux-gnu
else
RUST_TARGET_DIR ?= target/release
endif

all:
	cd rollup-http-server && $(CARGO) build $(CARGO_FLAGS) $(RUST_TARGET)
	cd echo-dapp && $(CARGO) build $(CARGO_FLAGS) $(RUST_TARGET)

install: rollup-http-server echo-dapp
	install -Dm755 rollup-http-server/$(RUST_TARGET_DIR)/rollup-http-server $(DESTDIR)$(PREFIX)/bin/rollup-http-server
	install -Dm755 echo-dapp/$(RUST_TARGET_DIR)/echo-dapp $(DESTDIR)$(PREFIX)/bin/echo-dapp

clean:
	rm -rf rollup-http-server/target echo-dapp/target

.PHONY: all install clean
