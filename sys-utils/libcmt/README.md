# Cartesi Machine Tools

Is a C library to facilitate the development of applications running on the cartesi-machine.
It handles the IO and communication protocol with the machine-emulator.

The high level @ref libcmt\_rollup API provides functions for common operations, such as generating vouchers, notices, retrieving the next input, etc.
Check the [cartesi documentation](https://docs.cartesi.io/) for an explanation of the rollup interaction model.

In addition to the above mentioned module, we provide @ref libcmt\_io\_driver, a thin abstraction of the linux kernel driver.

And finally, a couple of utility modules used by the high level API are also exposed.
- @ref libcmt\_abi is a Ethereum Virtual Machine Application Binary Interface (EVM-ABI) encoder / decoder.
- @ref libcmt\_buf is a bounds checking buffer.
- @ref libcmt\_merkle is a sparse merkle tree implementation on top of keccak.
- @ref libcmt\_keccak is the hashing function used extensively by Ethereum.

The header files and a compiled RISC-V version of this library can be found [here](https://github.com/cartesi/machine-guest-tools/).
We also provide `.pc` (pkg-config) files to facilitate linking.

# mock and testing

This library provides a mock implementation of @ref libcmt\_io\_driver that is
able to simulate requests and replies via files on the host machine.

- Build it with: `make mock`.
- Install it with: `make install-mock`, use `PREFIX` to specify the installation path:
    (The command below will install the library and headers on `$PWD/_install` directory)
```
make install-mock PREFIX=$PWD/_install
```

## testing

Use the environment variable @p CMT\_INPUTS to inject inputs into applications compiled with the mock.
Outputs will be written to files with names derived from the input name.

example:
```
CMT_INPUTS="0:advance.bin" ./application
```

The first output will generate the file:
```
advance.output-0.bin
```

The first report will generate the file:
```
advance.report-0.bin
```

The first exception will generate the file:
```
advance.exception-0.bin
```

The (verifiable) outputs root hash:
```
advance.outputs_root_hash.bin
```

Inputs must follow this syntax, a comma separated list of reason number followed by a file path:
```
CMT_INPUTS="<reason-number> ':' <filepath> ( ',' <reason-number> ':' <filepath> ) *"
```

For rollup, available reasons are: `0` is advance and `1` is inspect.

In addition to @p CMT\_INPUTS, there is also the @p CMT\_DEBUG variable.
Enabling it will cause additional debug messages to be displayed.

```
CMT_DEBUG=yes ./application
```

## generating inputs

Inputs and Outputs are expected to be EVM-ABI encoded. Encoding and decoding
can be achieved multiple ways, including writing tools with this library. A
simple way to generate testing data is to use the @p cast tool from
[foundry](http://book.getfoundry.sh/reference/cast/cast.html) and `xxd`.

Encoding an @p EvmAdvance:
```
cast calldata "EvmAdvance(uint256,address,address,uint256,uint256,uint256,bytes)" \
	0x0000000000000000000000000000000000000001 \
	0x0000000000000000000000000000000000000002 \
	0x0000000000000000000000000000000000000003 \
	0x0000000000000000000000000000000000000004 \
	0x0000000000000000000000000000000000000005 \
	0x0000000000000000000000000000000000000006 \
	0x`echo "advance-0" | xxd -p -c0` | xxd -r -p > 0.bin
```

Inspect states require no encoding.
```
echo -en "inspect-0" > 1.bin
```

## parsing outputs

Decoding a @p Voucher:
```
cast calldata-decode "Voucher(address,uint256,bytes)" 0x`xxd -p -c0 "$1"` | (
    read address
    read value
    read bytes

    echo "{"
    printf '\t"address" : "%s",\n' $address
    printf '\t"value"   : "%s",\n' $value
    printf '\t"bytes"   : "%s"\n' $bytes
    echo "}"
)

# sh decode-voucher.sh $1 | jq '.bytes' | xxd -r
```

Decoding a @p Notice:
```
cast calldata-decode "Notice(bytes)" 0x`xxd -p -c0 "$1"` | (
    read bytes

    echo "{"
    printf '\t"bytes"   : "%s"\n' $bytes
    echo "}"
)

# sh decode-notice.sh $1 | jq '.bytes' | xxd -r
```
