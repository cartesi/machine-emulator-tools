# DApp example echo application

For Cartesi decentralized application to communicate with the client outside of emulator, `/dev/rollup` linux device is used. Rollup HTTP server translates requests read from Linux device to http `advance/inspect` requests and conveys them to the DApp using its HTTP api interface. DApp communicates results of its execution to the outside world by using `voucher/notice/report/finish` API of the Rollup HTTP Server.

Rollup HTTP Server application implements interface defined [here](https://github.com/cartesi/rollups/blob/develop/openapi/rollup.yaml) and DApp application as http client pools Rollup HTTP Server for new requests, and pushes to it results of request processing voucher/notices/reports.  


## Getting Started
This project requires Rust.
To install Rust follow the instructions [here](https://www.rust-lang.org/tools/install).


## Build for risc-v cpu
Prerequisites: `riscv64-cartesi-linux-gnu` toolchain must be in the $PATH

###Build
```shell
$ source ./environment.sh
$ cargo +nightly build -Z build-std=std,core,alloc,panic_abort,proc_macro --target riscv64ima-cartesi-linux-gnu.json --release
```

## Authors

* *Marko Atanasievski*

## License

The http-dispatcher project and all contributions are licensed under
[APACHE 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please review our [LICENSE](LICENSE) file.