> :warning: The Cartesi team keeps working internally on the next version of this repository, following its regular development roadmap. Whenever there's a new version ready or important fix, these are published to the public source tree as new releases.

# Cartesi Machine Emulator Tools 

The Cartesi Machine Emulator Tools a repository that contains a set of tools developed for the RISC-V Linux OS.

## Getting Started

### Requirements

- Docker 18.x
- GNU Make >= 3.81

### Build htif yield tool

```bash
$ cd linux/htif
$ make
```

#### Makefile targets

The following options are available as `make` targets:

- **all**: builds the RISC-V yield executable and put it on the `extra` directory
- **extra.ext2**: builds the extra.ext2 image based on `extra` directory
- **toolchain-env**: runs the toolchain image with current user UID and GID
- **clean**: clean generated artifacts 

#### Makefile container options

You can pass the following variables to the make target if you wish to use different docker image tags.

- TOOLCHAIN\_IMAGE: toolchain image name 
- TOOLCHAIN\_TAG: toolchain image tag

```
$ make TOOLCHAIN_TAG=mytag
```

It's useful when you want to use prebuilt images like `cartesi/toolchain:latest` 

#### Usage

The purpose of the yield tool please see the emulator documentation.

The purpose of the `extra.ext2` image is to help the development creating a filesystem that contains the yield tool so it can be used with the emulator. For instructions on how to do that, please see the emulator documentation.

## Contributing

Thank you for your interest in Cartesi! Head over to our [Contributing Guidelines](CONTRIBUTING.md) for instructions on how to sign our Contributors Agreement and get started with
Cartesi!

Please note we have a [Code of Conduct](CODE_OF_CONDUCT.md), please follow it in all your interactions with the project.

## Authors

* *Diego Nehab*
* *Victor Fusco*

## License

The machine-emulator-tools repository and all contributions are licensed under
[APACHE 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please review our [LICENSE](LICENSE) file.

