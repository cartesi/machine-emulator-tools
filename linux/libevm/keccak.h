/** @file
 * @defgroup libevm_keccak keccak
 * Keccak 256 digest
 *
 * Data can be inserted in pieces:
 * @code
 * ...
 * uint8_t h[EVM_KECCAK_LEN];
 * evm_keccak_t st[1];
 *
 * evm_keccak_init(st);
 * evm_keccak_update(st, 1, "h");
 * evm_keccak_update(st, 1, "e");
 * evm_keccak_update(st, 1, "l");
 * evm_keccak_update(st, 1, "l");
 * evm_keccak_update(st, 1, "o");
 * evm_keccak_final(st, h);
 * ...
 * @endcode
 *
 * all at once:
 * @code
 * ...
 * const char data[] = "hello";
 * uint8_t h[EVM_KECCAK_LEN];
 * evm_keccak_data(h, sizeof(data)-1, data);
 * ...
 * @endcode
 *
 * or with a specialized call to generate @ref funsel data:
 * @code
 * ...
 * uint32_t funsel = evm_keccak_funsel("FunctionCall(address,bytes)");
 * ...
 * @endcode
 * @ingroup libevm
 * @{ */
#ifndef EVM_KECCAK_H
#define EVM_KECCAK_H

/** Opaque Keccak state, used to do hash computations, initialize with:
 * - @ref evm_keccak_init
 * - @ref EVM_KECCAK_INIT
 * - @ref EVM_KECCAK_DECL */
typedef struct {
    union {
        uint8_t b[200];
        uint64_t q[25];
    } st;
    int pt, rsiz;
} evm_keccak_t;

/** Bytes in the hash digest */
#define EVM_KECCAK_LEN 32

/** Initialize a keccak state */
#define EVM_KECCAK_INIT(STATE) {            \
	.st.q = {                           \
		0, 0, 0, 0, 0,              \
		0, 0, 0, 0, 0,              \
		0, 0, 0, 0, 0,              \
		0, 0, 0, 0, 0,              \
		0, 0, 0, 0, 0,              \
	},                                  \
	.pt = 0,                            \
	.rsiz = 200 - 2 * EVM_KECCAK_LEN,   \
}

/** Declare and initialize a keccak state */
#define EVM_KECCAK_DECL(NAME) evm_keccak_t NAME[1] = {EVM_KECCAK_INIT(NAME)}

/** Initialize a @ref evm_keccak_t hasher state.
 *
 * @param [out] state uninitialized @ref evm_keccak_t */
void evm_keccak_init(evm_keccak_t *state);

/** Hash @b n bytes of @b data 
 *
 * @param [in,out] state initialize the hasher state
 * @param [in]     n     bytes in @b data to process
 * @param [in]     data  data to hash */
void evm_keccak_update(evm_keccak_t *state, size_t n, const void *data);

/** Finalize the hash calculation from @b state and store it in @b md
 *
 * @param [in]  state initialize the hasher state (with all data already added to it)
 * @param [out] md    32bytes to store the computed hash */
void evm_keccak_final(evm_keccak_t *state, void *md);

/** Hash all @b n bytes of @b data at once
 *
 * @param [in]  n     - bytes in @b data to process
 * @param [in]  data  - data to hash
 * @param [out] md    - 32bytes to store the computed hash
 * @return pointer to @b md
 *
 * Equivalent to:
 * @code
 * evm_keccak_t st = EVM_KECCAK_INIT(&st);
 * evm_keccak_update(&st, n, data);
 * evm_keccak_final(&st, md);
 * return md;
 * @endcode */
uint8_t *evm_keccak_data(size_t n, const void *data
                        ,uint8_t md[EVM_KECCAK_LEN]);

/** Compute the function selector from the solidity declaration @p decl
 *
 * @param [in]  decl   solidity call declaration, without variable names
 * @param [out] funsel function selector as described by @ref funsel
 * @return A @p funsel value as if defined by @ref EVM_ABI_FUNSEL
 *
 * Example usage:
 * @code
 * ...
 * uint32_t funsel = evm_keccak_funsel("FunctionCall(address,bytes)");
 * ...
 * @endcode
 */
uint32_t evm_keccak_funsel(const char *decl);
#endif /* EVM_KECCAK_H */
/** $@} */
