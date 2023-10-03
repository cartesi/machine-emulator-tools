/** @file
 * @defgroup libevm_abi abi
 * 
 * This is a C library to encode and decode Ethereum Virtual Machine (EVM)
 * calldata. This format is used to interacts with contracts in the Ethereum
 * ecosystem.
 *
 * We will cover the basic concepts required to use the API we provide, but for
 * a complete reference consult the solidity specification, it can be found
 * here: https://docs.soliditylang.org/en/latest/abi-spec.html
 *
 * ## Function Selector {#funsel}
 *
 * 4 bytes that identify the function name and parameter types. This is used to
 * distinguish between different formats. To compute it, take the four first
 * bytes of the `keccak` digest of the solidity function declaration. It should
 * respect a canonical format and such as having no the variable names, check
 * docs for details. For reference, for a hypotetical "FunctionName" function,
 * it should look something like this:
 * `keccak("FunctionName(type1,type2,...,typeN)");`
 *
 * ## Data Sections {#sections}
 *
 * After the function selector, we can start encoding the function parameters.
 * One by one, left to right in two sections. `Static` for types with fixed
 * size. Think ints, bools, addresses, etc. And then there is the `Dynamic`
 * section. In this case, the encoded size depend on the data they contain, and
 * how much of it, such as of strings and bytes. So:
 *
 * `Static` for ints, bools, adresses.
 *
 * `Dynamic` for bytes.
 *
 * ### Static Section {#static-section}
 *
 * Static types are encoded directly in the buffer when the API call is made.
 * Put the data in and be done with it.
 *
 * ### Dynamic Section {#dynamic-section}
 *
 * Dynamic types require two calls. The first goes into the Static section to
 * reserve space for a data offset. @ref evm_abi_put_bytes_s.
 *
 * After we are done with with all data that goes into the static section. We
 * start to actually encode the dynamic data. And here is where the second call
 * comes in. We patch the offset we reserved earlier with the actual data
 * position and copy its contents over. @ref evm_abi_put_bytes_d.
 *
 * ## Encoder
 *
 * Lets look at some code:
 *
 * @includelineno "examples/abi_put.c"
 *
 * ## Decoder
 *
 * Lets look at some code:
 *
 * @includelineno "examples/abi_get.c"
 *
 * ## Complete
 *
 * Lets look at some code:
 *
 * @includelineno "examples/abi.c"
 *
 * @ingroup libevm
 * @{ */
#ifndef EVM_ABI_H
#define EVM_ABI_H
#include"buf.h"

/** length of a evm address in bytes */
#define EVM_ADDRESS_LEN 20

/** Encode 4 bytes as a function selector */
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#define EVM_ABI_FUNSEL(A, B, C, D) \
	( ((A) << 000) \
	| ((B) << 010) \
	| ((C) << 020) \
	| ((D) << 030))
#else
#define EVM_ABI_FUNSEL(A, B, C, D) \
	( ((D) << 000) \
	| ((C) << 010) \
	| ((B) << 020) \
	| ((A) << 030))
#endif

// put section ---------------------------------------------------------------
/** Encode a function selector into the buffer @p me
 *
 * @param [in,out] me     a initialized buffer working as iterator
 * @param [in]     funsel function selector
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 *
 * @note A function selector can be compute it with: @ref evm_keccak_funsel.
 * It is always represented in big endian. */
int
evm_abi_put_funsel(evm_buf_t *me, uint32_t funsel);

/** Encode a unsigned integer of up to 32bytes of data into the buffer
 *
 * @param [in,out] me   a initialized buffer working as iterator
 * @param [in]     n    size of @p data in bytes
 * @param [in]     data poiter to a integer
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * - EDOM requested @p n is too large
 *
 * @code
 * ...
 * evm_buf_t it = ...;
 * uint64_t x = UINT64_C(0xdeadbeef);
 * evm_abi_put_uint(&it, sizeof x, &x);
 * ...
 * @endcode
 * @note This function takes care of endianess conversions */
int
evm_abi_put_uint(evm_buf_t *me, size_t n, const void *data);

/** Encode @p address (exacly @ref EVM_ADDRESS_LEN bytes) into the buffer
 *
 * @param [in,out] me      initialized buffer
 * @param [in]     address exactly @ref EVM_ADDRESS_LEN bytes
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
evm_abi_put_address(evm_buf_t *me, const uint8_t address[EVM_ADDRESS_LEN]);

/** Encode the static part of @b bytes into the message,
 * used in conjunction with @ref evm_abi_put_bytes_d
 *
 * @param [in,out] me     initialized buffer
 * @param [out]    offset initialize for @ref evm_abi_put_bytes_d
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
evm_abi_put_bytes_s(evm_buf_t *me, evm_buf_t *offset);

/** Encode the dynamic part of @b bytes into the message,
 * used in conjunction with @ref evm_abi_put_bytes_d
 *
 * @param [in,out] me     initialized buffer
 * @param [in]     offset initialized from @ref evm_abi_put_bytes_h
 * @param [in]     n      size of @b data
 * @param [in]     data   array of bytes
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
evm_abi_put_bytes_d(evm_buf_t *me, evm_buf_t *offset
                   ,size_t n, const void *data);

/** Reserve @b n bytes of data from the buffer into @b res to be filled by the
 * caller
 *
 * @param [in,out] me     initialized buffer
 * @param [in]     n      amount of bytes to reserve
 * @param [out]    res    slice of bytes extracted from @p me
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 *
 * @note @p me must outlive @p res.
 * Create a duplicate otherwise */
int
evm_abi_res_bytes(evm_buf_t *me, size_t n, evm_buf_t *res);

// get section ---------------------------------------------------------------

/** Read the funsel without advancing @p me
 *
 * @param [in]     me     initialized buffer
 * @return
 * - The function selector
 *
 * @code
 * ...
 * if (evm_buf_length(it) < 4)
 * 	return EXIT_FAILURE;
 * switch (evm_abi_peek_funsel(it) {
 * case EVM_ABI_FUNSEL(...): // known type, try to parse it
 * case EVM_ABI_FUNSEL(...): // known type, try to parse it
 * default:
 * 	return EXIT_FAILURE;
 * }
 * @endcode
 *
 * @note user must ensure there are at least 4 bytes in the buffer.
 * This function will fail and return 0 if that is not the case. */
uint32_t
evm_abi_peek_funsel(evm_buf_t *me);

/** Read the funsel from buffer @p me and match it against @p funsel
 *
 * @param [in,out] me              initialized buffer
 * @param [in]     expected_funsel expected function selector
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * - EBADMSG no case of a missmatch */
uint32_t
evm_abi_check_funsel(evm_buf_t *me, uint32_t expected_funsel);

/** Decode a unsigned integer of up to 32bytes from the buffer
 *
 * @param [in,out] me     initialized buffer
 * @param [in]     n      size in bytes of @p data
 * @param [out]    data   pointer to a integer
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * - EDOM    value won't fit into @p n bytes. */
int
evm_abi_get_uint(evm_buf_t *me, size_t n, void *data);

/** Decode @b address (exacly 20 bytes) from the buffer
 *
 * @param [in,out] me      initialized buffer
 * @param [out]    address exactly 20 bytes
 *
 * @return
 * - 0 success
 * - ENOBUFS requested size @b n is not available */
int
evm_abi_get_address(evm_buf_t *me, uint8_t address[EVM_ADDRESS_LEN]);

/** Decode the @p bytes offset
 *
 * @param [in,out] me initialized buffer
 * @param [out]    of offset of @p bytes, access contents with @ref evm_abi_get_bytes_d.
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
evm_abi_get_bytes_s(evm_buf_t *me, evm_buf_t of[1]);

/** Decode @b bytes from the buffer by taking a pointer to its contents.
 *
 * @param [in,out] me    initialized buffer
 * @param [out]    n     amount of data available in @b bytes
 * @param [out]    bytes memory range with contents
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * @note @p of can be initialized by calling @ref evm_abi_get_bytes_s */
int
evm_abi_get_bytes_d(evm_buf_t *me, evm_buf_t of[1], size_t *n, void **bytes);

#endif /* EVM_ABI_H */
/** @} */
