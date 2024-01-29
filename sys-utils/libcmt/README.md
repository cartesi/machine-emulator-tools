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

# mock and testing

This library provides a mock implementation of @ref libcmt\_io\_driver that is
able to simulate requests and replies on the host machine.

- Build it with: `make mock.build`.
- Install it with: `make mock.install`, use `PREFIX` to specify the installation path:
    (The command below will install the library and headers on `$PWD/_install` directory)
```
make mock.install PREFIX=_install
```

## testing

Use the environment variable @p CMT\_INPUTS to inject inputs into the application compiled with the mock.
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

inputs must follow this syntax, a comma separated list of reason number followed by a filepath:
```
CMT_INPUTS="<reason-number> ':' <filepath> ( ',' <reason-number> ':' <filepath> ) *"
```

In addition to @p CMT\_INPUTS, there is also the @p CMT\_DEBUG variable. Enable it
for a verbose version of the low level calls.

```
CMT_DEBUG=yes ./application
```

## generating inputs

Inputs and Outputs are expected to be EVM-ABI encoded. Encoding and decoding
can be acheived multiple ways, including writing tools with this library. A
simple way for testing is to use the @p cast tool from
[foundry](http://book.getfoundry.sh/reference/cast/cast.html) and `xxd`.

Encoding an @p EvmAdvance:
```
cast calldata "EvmAdvance(address,uint256,uint256,uint256,bytes)" \
	0x0000000000000000000000000000000000000000 \
	0x0000000000000000000000000000000000000000 \
	0x0000000000000000000000000000000000000000 \
	0x0000000000000000000000000000000000000000 \
	0x`echo "advance-0" | xxd -p -c0` | xxd -r -p > 0.bin
```

Encoding an @p EvmInspect:
```
cast calldata "EvmInspect(bytes)" \
	0x`echo "inspect-0" | xxd -p -c0` | xxd -r -p > 1.bin
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

# binds

This library is intented to be used with programming languages other than C.
They acheive this by different means.

## Go

Go is able to include C headers directly.

The application can be compiled with: `go build main.go`. Assuming that the
mock was installed at `./libcmt-0.1.0`.
(Code below is for demonstration purposes and not intended for production)

```
package main

/*
#cgo CFLAGS: -Ilibcmt-0.1.0/include/
#cgo LDFLAGS: -Llibcmt-0.1.0/lib/ -lcmt

#include "libcmt/rollup.h"
*/
import "C"

import (
    "math/big"
    "fmt"
    "unsafe"
)

func main() {
    var rollup C.cmt_rollup_t
    err := C.cmt_rollup_init(&rollup)
    if err != 0 {
        fmt.Printf("initialization failed\n")
    }

    bytes_s := []byte{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    }
    wei := new(big.Int).SetBytes(bytes_s)

    finish := C.cmt_rollup_finish_t{
        accept_previous_request: true,
    }
    for {
        var advance C.cmt_rollup_advance_t
        err = C.cmt_rollup_finish(&rollup, &finish)
        if err != 0 { return; }

        err = C.cmt_rollup_read_advance_state(&rollup, &advance);
        if err != 0 { return; }

        bytes:= wei.Bytes()
        size := len(bytes)
        C.cmt_rollup_emit_voucher(&rollup,
                                  C.CMT_ADDRESS_LENGTH, &advance.sender[0],
                                  C.uint(size), unsafe.Pointer(&bytes[0]),
                                  advance.length, advance.data)
        C.cmt_rollup_emit_report(&rollup, advance.length, advance.data)
    }
}
```

## Rust

Rust interop with C requires bindings to be generated, we'll use bindgen to
accomplish this.

Create the initial directory layout with `cargo`, then add the library to it:
```
cargo init --bin cmt
# download the library and extract it into cmt/libcmt-0.1.0
cd cmt
```

Generate the bindings, execute:
```
# join all header files into one.
cat libcmt-0.1.0/include/libcmt/*.h > src/libcmt.h

# generate the bindings
bindgen src/libcmt.h -o src/bindings.rs --allowlist-function '^cmt_.*' --allowlist-type '^cmt_.*' --no-doc-comments -- -I libcmt-0.1.0/include/libcmt
```

Include the bindings to the project, add the following to: `src/lib.rs`
```
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!("bindings.rs");
```

Tell cargo to link with the C library, add the following to: `build.rs`
```
fn main() {
    println!("cargo:rustc-link-search=libcmt-0.1.0/lib");
    println!("cargo:rustc-link-lib=cmt");
}
```

An example of the raw calls to C, add the following to: `src/main.rs`
```
use std::mem;

fn main() {
    let mut rollup: cmt::cmt_rollup_t = unsafe { mem::zeroed() };
    let rc = unsafe { cmt::cmt_rollup_init(&mut rollup) };
    println!("got return value: {}!", rc);
}
```

Messages like the one below most likely mean that cargo didn't find the library
`libcmt.a` or `build.rs` is incomplete:
```
undefined reference to `cmt_rollup_emit_voucher'
```
