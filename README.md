# Cartesi Machine Guest Tools

The Cartesi Machine Guest Tools is a repository that contains a set of tools developed for the RISC-V Linux OS. It provides a RISC-V Debian package and a root filesystem for Ubuntu 24.04.

## Getting Started

Users looking to create cartesi-machine applications can use the provided Debian package and root filesystem directly from one of the prepackaged releases, without needing to build this repository themselves.

### Requirements

- Docker >= 18.x
- GNU Make >= 3.81
- xgenext2fs >= 1.5.6

### Docker buildx setup

To set up riscv64 buildx, use the following command:

```bash
$ make setup
```

### Building (from riscv64 environment)

Assuming you are inside a riscv64 environment, you can build tools and install in the system with:

```sh
make
make install PREFIX=/usr
```

### Building (cross compiling)

In case you need to patch and develop tools, you can cross compile using Docker with:

```sh
make build
```

This creates the `machine-guest-tools_riscv64.tar.gz` archive and the `rootfs-tools.ext2` root filesystem artifacts.
Both should only be use for testing and development purposes, not directly in dapps.

#### Makefile targets

Check all targets available with `make help`.

## Contributing

Thank you for your interest in Cartesi! Head over to our [Contributing Guidelines](CONTRIBUTING.md) for instructions on how to sign our Contributors Agreement and get started with
Cartesi!

Please note we have a [Code of Conduct](CODE_OF_CONDUCT.md), please follow it in all your interactions with the project.

## License

The machine-guest-tools repository and all contributions are licensed under
[APACHE 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please review our [LICENSE](LICENSE) file.
