This is a C library to facilitate the development of applications running on the cartesi-machine.
It handles the IO and communication protocol with the machine-emulator.

The high level @ref libcmt\_rollup API provides functions for common operations, such as generating vouchers, notices, retrieving the next input, etc.
Check the [cartesi documentation](https://docs.cartesi.io/) for an explanation of the rollup interaction model.

In addition to the above mentioned module, we provide @ref libcmt\_io\_driver, a thin abstraction of the linux kernel driver.

And finally, a couple of utility modules used by the high level API are also exposed.
- @ref libcmt\_abi is a Ethereum Virtual Machine Application Binary Interface (EVM-ABI) encoder / decoder.
- @ref libcmt\_buf is a bounds checking buffer.
- @ref libcmt\_merkle is a sparse merkle tree implementation on top of keccak.
- @ref libcmt\_keccak is the hashing function used extensively by Ethereum.

The header files and a compiled RISC-V version of this library can be found [here](https://github.com/cartesi/machine-emulator-tools/).
We also provide `.pc` (pkg-config) files to facilitate linking.
