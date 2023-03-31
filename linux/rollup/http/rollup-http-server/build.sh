#!/bin/bash

TOOLCHAIN_IMAGE="${TOOLCHAIN_IMAGE:-cartesi/toolchain}"
TOOLCHAIN_TAG="${TOOLCHAIN_TAG:-0.14.0}"
RISCV_ARCH="${RISCV_ARCH:-rv64gc}"
RISCV_ABI="${RISCV_ABI:-lp64d}"
CONTAINER_BASE="${CONTAINER_BASE:-/opt/cartesi/tools}"

docker run --rm -e USER=`id -u -n` -e GROUP=`id -g -n` -e UID=`id -u` -e GID=`id -g` -v `pwd`/../:$CONTAINER_BASE \
--env CC=riscv64-cartesi-linux-gnu-gcc --env CXX=riscv64-cartesi-linux-gnu-g++ --env CFLAGS="-march=$RISCV_ARCH -mabi=$RISCV_ABI" \
-w $CONTAINER_BASE/rollup-http-server $TOOLCHAIN_IMAGE:$TOOLCHAIN_TAG \
cargo build -Z build-std=std,core,alloc,panic_abort,proc_macro --target rv64gc-cartesi-linux-gnu.json --release
