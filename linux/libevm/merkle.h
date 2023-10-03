/** @file
 * @defgroup libevm_merkle merkle
 * merkle tree of keccak256 hashes
 *
 * @ingroup libevm
 * @{ */
#ifndef EVM_MERKLE_H
#define EVM_MERKLE_H
#include"keccak.h"

/** Opaque Merkle tree state, used to do hash computations, initialize with:
 * - @ref evm_merkle_init */
typedef struct {
	uint64_t n;
	const uint8_t (*zero)[EVM_KECCAK_LEN];
	uint8_t siblings[EVM_KECCAK_LEN][EVM_KECCAK_LEN];
} evm_merkle_t;

/** Initialize a @ref evm_merkle_t tree state.
 *
 * @param [in] me    uninitialized state */
void evm_merkle_init(evm_merkle_t *me);

/** Append a leaf node
 *
 * @param [in,out] me initialized state
 * @param [in] hash   value of the new leaf
 * @return
 * - 0       success
 * - ENOBUFS tree is full, no more space available
 *
 * @code
 * ...
 * evm_buf_t tx, it = ...;
 * uint8_t md[EVM_KECCAL_MDLEN];
 *
 * // used bytes
 * size_t n = it.p - tx.p;
 * void  *p =        tx.p;
 *
 * evm_merkle_t merkle = EVM_MERKLE_INIT(&merkle);
 * evm_merkle_put(evm_keccak_data(md, n, p), md);
 * @endcode */
int evm_merkle_put(evm_merkle_t *me, uint8_t hash[EVM_KECCAK_LEN]);

/** Compute the hash of @p data, then append it as a leaf node
 *
 * @param [in,out] me    initialized state */
int evm_merkle_put_data(evm_merkle_t *me, size_t n, void *data);

/** Retrieve the root hash of the merkle tree
 *
 * @param [in]  me   initialized state
 * @param [out] root root hash of the merkle tree */
void evm_merkle_get_root(evm_merkle_t *me, uint8_t root[EVM_KECCAK_LEN]);

#endif /* EVM_MERKLE_H */
