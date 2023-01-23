#! /bin/sh

export CC=riscv64-cartesi-linux-gnu-gcc
export CXX=riscv64-cartesi-linux-gnu-g++
export RISCV_ARCH=rv64gc
export RISCV_ABI=lp64d
export CFLAGS="-march=$RISCV_ARCH -mabi=$RISCV_ABI"
