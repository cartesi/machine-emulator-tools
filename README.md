# Cartesi Machine Emulator Tools

The Cartesi Machine Emulator Tools is a repository that contains a set of tools developed for the RISC-V Linux OS. It provides a RISC-V Debian package and a root filesystem for Ubuntu 22.04.

## Getting Started

Users looking to create cartesi-machine applications can use the provided Debian package and root filesystem directly from one of the prepackaged [releases](https://github.com/cartesi/machine-emulator-tools/releases), without needing to build this repository themselves.

### Requirements

- Docker >= 18.x
- GNU Make >= 3.81
- bsdtar >= 3.7.2
- xgenext2fs >= 1.5.3

### Docker buildx setup

To set up riscv64 buildx, use the following command:

```bash
$ make setup
```

### Building

Invoking make will build all tools and create the `machine-emulator-tools-$VERSION.deb` Debian package along with the `rootfs-tools-$VERSION.ext2` root filesystem artifacts.

```bash
$ make
```

#### Makefile targets

The following commands are available as `make` targets:

- **all**: Build Debian package and rootfs (Default)
- **deb**: Build machine-emulator-tools.deb package
- **fs**: Build rootfs.ext2
- **setup**: Setup riscv64 buildx
- **setup-required**: Check if riscv64 buildx setup is required
- **help**: List Makefile commands
- **env**: Print useful Makefile variables as a KEY=VALUE list
- **clean**: Remove the generated artifacts

## Contributing

Thank you for your interest in Cartesi! Head over to our [Contributing Guidelines](CONTRIBUTING.md) for instructions on how to sign our Contributors Agreement and get started with
Cartesi!

Please note we have a [Code of Conduct](CODE_OF_CONDUCT.md), please follow it in all your interactions with the project.

## License

The machine-emulator-tools repository and all contributions are licensed under
[APACHE 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please review our [LICENSE](LICENSE) file.
