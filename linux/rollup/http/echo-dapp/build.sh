#!/bin/bash

TOOLCHAIN_IMAGE="${TOOLCHAIN_IMAGE:-cartesi/toolchain}"
TOOLCHAIN_TAG="${TOOLCHAIN_TAG:-0.11.0}"
CONTAINER_BASE="${CONTAINER_BASE:-/opt/cartesi/tools}"

docker run --rm -e USER=`id -u -n` -e GROUP=`id -g -n` -e UID=`id -u` -e GID=`id -g` -v `pwd`/../:$CONTAINER_BASE \
--env CC=riscv64-cartesi-linux-gnu-gcc --env CXX=riscv64-cartesi-linux-gnu-g++ --env CFLAGS="-march=rv64ima -mabi=lp64" \
-w $CONTAINER_BASE/echo-dapp $TOOLCHAIN_IMAGE:$TOOLCHAIN_TAG \
cargo build -Z build-std=std,core,alloc,panic_abort,proc_macro --target riscv64ima-cartesi-linux-gnu.json --release


