#!/bin/bash

TOOLCHAIN_IMAGE="${TOOLCHAIN_IMAGE:-cartesi/toolchain}"
TOOLCHAIN_TAG="${TOOLCHAIN_TAG:-0.16.0}"
RISCV_ARCH="${RISCV_ARCH:-rv64gc}"
RISCV_ABI="${RISCV_ABI:-lp64d}"
CONTAINER_BASE="${CONTAINER_BASE:-/opt/cartesi/tools}"

docker run --rm -e USER=`id -u -n` -e GROUP=`id -g -n` -e UID=`id -u` -e GID=`id -g` -v `pwd`/../:$CONTAINER_BASE \
--env CC=riscv64-cartesi-linux-gnu-gcc --env CXX=riscv64-cartesi-linux-gnu-g++ --env CFLAGS="-march=$RISCV_ARCH -mabi=$RISCV_ABI" \
--env CARGO_TARGET_RISCV64GC_UNKNOWN_LINUX_GNU_LINKER=riscv64-cartesi-linux-gnu-gcc \
-w $CONTAINER_BASE/rollup-http-server $TOOLCHAIN_IMAGE:$TOOLCHAIN_TAG \
cargo build --target riscv64gc-unknown-linux-gnu --release
