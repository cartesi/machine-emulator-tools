# DApp example echo application

For Cartesi decentralized application to communicate with the client outside of emulator, `/dev/rollup` linux device is used. Http dispatcher application translates requests read from Linux device to http `advance/inspect` requests and passes them to the DApp. DApp communicates results of its execution to the outside world by using `voucher/notice/report/finish` API of the http dispatcher.

Http dispatcher application implements interface defined [here](https://github.com/cartesi/rollups/blob/develop/openapi/dapp.yaml).

DApp application starts `http-dispatcher` service as part of its startup. `Target-proxy` path is defined by `TARGET_PROXY_PATH` environment variable when starting Dapp.


## Getting Started
This project requires Rust.
To install Rust follow the instructions [here](https://www.rust-lang.org/tools/install).


## Build for risc-v cpu
Prerequisites: `riscv64-cartesi-linux-gnu` toolchain must be in the $PATH

###Build
```shell
$ source ./environment.sh
$ cargo build +nightly -Z build-std=std,core,alloc,panic_abort,proc_macro --target riscv64ima-unknown-linux-gnu.json --release
```
