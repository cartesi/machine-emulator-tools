# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Added rollup-exception handling to ioctl-echo-loop
- Added --reject-inspects as an option to ioctl-echo-loop

### Changed
- Enabled rollup-exception in rollup
- Updated yield to the new yield\_request format
- Removed --ack from yield

## [0.5.1] - 2022-02-22
### Changed
- Lock dependencies versions using Cargo.lock on http-dispatcher and echo-dapp
- Fix --reject option on ioctl-echo-loop

## [0.5.0] - 2022-01-13
### Changed
- Updated http-dispatcher and echo-dapp according to openapi-interfaces changes
- Remove index from voucher and notice http-requests on http-dispatcher and echo-dapp
- Fixed http-dispatcher payload handling

## [0.4.1] - 2021-12-29
### Changed
- Fixed deadlock on http-dispatcher

## [0.4.0] - 2021-12-21
### Added
- New ioctl-echo-loop tool to test rollups
- New rollup http-dispatcher
- New echo dapp based on the http-dispatcher architecture

### Changed
- Updated yield command line tool to be compatible with Linux kernel v5.5.19-ctsi-3

## [Previous Versions]
- [0.3.0]
- [0.2.0]
- [0.1.0]

[Unreleased]: https://github.com/cartesi/machine-emulator-tools/compare/v0.5.1...HEAD
[0.5.1]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.5.1
[0.5.0]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.5.0
[0.4.1]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.4.1
[0.4.0]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.4.0
[0.3.0]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.3.0
[0.2.0]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.2.0
[0.1.0]: https://github.com/cartesi/machine-emulator-tools/releases/tag/v0.1.0


