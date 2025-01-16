# DApp example echo application

For Cartesi decentralized application to communicate with the client outside of emulator, `/dev/rollup` linux device is used. Rollup HTTP server translates requests read from Linux device to http `advance/inspect` requests and conveys them to the DApp using its HTTP api interface. DApp communicates results of its execution to the outside world by using `voucher/notice/report/finish` API of the Rollup HTTP Server.

Rollup HTTP Server application implements interface defined [here](https://github.com/cartesi/rollups/blob/develop/openapi/rollup.yaml) and DApp application as http client pools Rollup HTTP Server for new requests, and pushes to it results of request processing voucher/notices/reports.


## Getting Started
This project requires Rust.
To install Rust follow the instructions [here](https://www.rust-lang.org/tools/install).

### Build 
This service is built within the docker build step when building all other tools: 

```shell
cd machine-guest-tools
make
```

### Build using the libcmt mock version (for development and tests)
First build libcmt mock on host:

```shell
cd machine-guest-tools/sys-utils/libcmt/
make host
```

Then build this project with:
```shell
MOCK_BUILD=true cargo build
```

### Run tests
```shell
MOCK_BUILD=true cargo test -- --show-output --test-threads=1
```


## License

The http-dispatcher project and all contributions are licensed under
[APACHE 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please review our [LICENSE](LICENSE) file.
