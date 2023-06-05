# Cartesi Machine Emulator Tools

The Cartesi Machine Emulator Tools a repository that contains a set of tools developed for the RISC-V Linux OS.

## Getting Started

Users looking to create cartesi-machine applications can use the artifact directly, no need to build this repository themselves. Also check the `example/` folder for the details.

### Requirements

- Docker >= 18.x
- GNU Make >= 3.81

### Docker buildx setup

```bash
$ make setup
```

### Building

A `make` invocation will download the dependencies if they are not present, build the ubuntu sdk, all tools and finally create the `machine-emulator-tools-$VERSION.tar.gz` artifact.

```bash
$ make
```

#### Makefile targets

The following options are available as `make` targets:

- **tgz**: create "machine-emulator-tools.tar.gz"
- **setup**: setup riscv64 buildx
- **setup-required**: check if riscv64 buildx setup is required
- **clean**: clean generated artifacts
- **help**: list makefile targets

## Contributing

Thank you for your interest in Cartesi! Head over to our [Contributing Guidelines](CONTRIBUTING.md) for instructions on how to sign our Contributors Agreement and get started with
Cartesi!

Please note we have a [Code of Conduct](CODE_OF_CONDUCT.md), please follow it in all your interactions with the project.

## License

The machine-emulator-tools repository and all contributions are licensed under
[APACHE 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please review our [LICENSE](LICENSE) file.
