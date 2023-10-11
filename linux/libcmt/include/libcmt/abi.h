/* Copyright Cartesi and individual authors (see AUTHORS)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/** @file
 * @defgroup libcmt_abi abi
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
 * One by one, left to right in two sections. `Static` for fixed sized values.
 * Think ints, bools, addresses, offsets, etc. Then `Dynamic` for variable
 * sized data such as the contents of @b bytes. Values that need a dynamic
 * section will also have an entry in the static section.
 *
 * ### Static Section {#static-section}
 *
 * [uint](@ref cmt_abi_put_uint), [bool](@ref cmt_abi_put_bool) and
 * [address](@ref cmt_abi_put_address) values are encoded directly in the
 * static section. In addition to those, @b bytes gets an entry in both
 * sections. The static part is done with [this](@ref cmt_abi_put_bytes_s) call.
 *
 * ### Dynamic Section {#dynamic-section}
 *
 * The Dynamic section encodes the contents of variable sized types. Every entry
 * in this section requires a corresponding entry in the static section as well.
 *
 * So types with variable size are encoded in both sections.
 *
 * - `static` section gets some kind of reference / offset to the dynamic section.
 * - `dynamic` section gets the actual contents
 *
 * In more concrete terms, the @b bytes type is encoded first with a call to @ref
 * cmt_abi_put_bytes_s for its `static` section part and then with a call to
 * @ref cmt_abi_put_bytes_d for its `dynamic` section part.
 *
 * ## Encoder
 *
 * Lets look at some code starting with a simple case. A function that encodes
 * the function selector and a signle @b address value into the buffer:
 *
 * @includelineno "examples/abi_encode_000.c"
 *
 * We need both sections for encoding @b bytes. static and dynamic.
 *
 * @includelineno "examples/abi_encode_001.c"
 *
 * We do entreis in order when we have multiple values in the dynamic section.
 *
 * @includelineno "examples/abi_encode_002.c"
 *
 * ## Decoder
 *
 * Lets look at code that decodes the examples above. We'll _check_ instead of
 * _put_ for funsel. And _get_ instead _put_ for most of everything else.
 *
 * @includelineno "examples/abi_decode_000.c"
 *
 * Retrieving @b bytes is a bit different since the API doesn't do dynamic
 * memory allocation. @b data points inside the @p rd buffer itself, into its
 * dynamic section. This makes the API very lightweight and fast but requires
 * care in its usage. If @p rd gets free'd or reused while there is still a
 * reference to @p data, we'll get memory corruption. If in doublt create a
 * copy of @p data and use it instead.
 *
 * @includelineno "examples/abi_decode_001.c"
 *
 * ## Complete
 *
 * Lets look at some code on how to tie everything together. We'll build a @b
 * echo of sorts that decodes the contents of @p rd and re-encodes it into @p
 * wr. First we'll take two of the previous encode/decode examples as a
 * starting point. We'll add the suffix 000 and 001 to disambiguate them. We'll
 * use the @ref cmt_abi_peek_funsel in our new @p decode function to decide
 * which of the two message types we got and how to handle it.
 *
 * @includelineno "examples/abi_multi.c"
 *
 * @ingroup libcmt
 * @{ */
#ifndef CMT_ABI_H
#define CMT_ABI_H
#include<stdbool.h>
#include"buf.h"

enum {
	CMT_WORD_LENGTH = 32,    /**< length of a evm word in bytes */
	CMT_ADDRESS_LENGTH = 20, /**< length of a evm address in bytes */
};


/** Compile time equivalent to @ref cmt_abi_funsel
 * @note don't port. use @ref cmt_abi_funsel instead */
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#define CMT_ABI_FUNSEL(A, B, C, D) \
	( ((uint32_t)(D) << 000) \
	| ((uint32_t)(C) << 010) \
	| ((uint32_t)(B) << 020) \
	| ((uint32_t)(A) << 030))
#else
#define CMT_ABI_FUNSEL(A, B, C, D) \
	( ((uint32_t)(A) << 000) \
	| ((uint32_t)(B) << 010) \
	| ((uint32_t)(C) << 020) \
	| ((uint32_t)(D) << 030))
#endif

// put section ---------------------------------------------------------------

/** Create a function selector from an array of bytes
 * @param [in] funsel function selector bytes
 * @return
 * - function selector converted to big endian (as expected by EVM) */
uint32_t
cmt_abi_funsel(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

/** Encode a function selector into the buffer @p me
 *
 * @param [in,out] me     a initialized buffer working as iterator
 * @param [in]     funsel function selector
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 *
 * @note A function selector can be compute it with: @ref cmt_keccak_funsel.
 * It is always represented in big endian. */
int
cmt_abi_put_funsel(cmt_buf_t *me, uint32_t funsel);

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
 * cmt_buf_t it = ...;
 * uint64_t x = UINT64_C(0xdeadbeef);
 * cmt_abi_put_uint(&it, sizeof x, &x);
 * ...
 * @endcode
 * @note This function takes care of endianess conversions */
int
cmt_abi_put_uint(cmt_buf_t *me, size_t n, const void *data);

/** Encode a bool into the buffer
 *
 * @param [in,out] me    a initialized buffer working as iterator
 * @param [in]     value boolean value
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 *
 * @code
 * ...
 * cmt_buf_t it = ...;
 * cmt_abi_put_bool(&it, true);
 * ...
 * @endcode
 * @note This function takes care of endianess conversions */
int
cmt_abi_put_bool(cmt_buf_t *me, bool value);

/** Encode @p address (exacly @ref CMT_ADDRESS_LENGTH bytes) into the buffer
 *
 * @param [in,out] me      initialized buffer
 * @param [in]     address exactly @ref CMT_ADDRESS_LENGTH bytes
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
cmt_abi_put_address(cmt_buf_t *me, const uint8_t address[CMT_ADDRESS_LENGTH]);

/** Encode the static part of @b bytes into the message,
 * used in conjunction with @ref cmt_abi_put_bytes_d
 *
 * @param [in,out] me     initialized buffer
 * @param [out]    offset initialize for @ref cmt_abi_put_bytes_d
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
cmt_abi_put_bytes_s(cmt_buf_t *me, cmt_buf_t *offset);

/** Encode the dynamic part of @b bytes into the message,
 * used in conjunction with @ref cmt_abi_put_bytes_d
 *
 * @param [in,out] me     initialized buffer
 * @param [in]     offset initialized from @ref cmt_abi_put_bytes_h
 * @param [in]     n      size of @b data
 * @param [in]     data   array of bytes
 * @param [in]     start  starting point for offset calculation (first byte after funsel)
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
cmt_abi_put_bytes_d(cmt_buf_t *me, cmt_buf_t *offset
                   ,size_t n, const void *data, const void *start);

/** Reserve @b n bytes of data from the buffer into @b res to be filled by the
 * caller
 *
 * @param [in,out] me     initialized buffer
 * @param [in]     n      amount of bytes to reserve
 * @param [out]    res    slice of bytes extracted from @p me
 * @param [in]     start  starting point for offset calculation (first byte after funsel)
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 *
 * @note @p me must outlive @p res.
 * Create a duplicate otherwise */
int
cmt_abi_reserve_bytes_d(cmt_buf_t *me, cmt_buf_t *of, size_t n, cmt_buf_t *out, const void *start);

// get section ---------------------------------------------------------------

/** Read the funsel without consuming it from the buffer @p me
 *
 * @param [in]     me     initialized buffer
 * @return
 * - The function selector
 *
 * @code
 * ...
 * if (cmt_buf_length(it) < 4)
 * 	return EXIT_FAILURE;
 * switch (cmt_abi_peek_funsel(it) {
 * case CMT_ABI_FUNSEL(...): // known type, try to parse it
 * case CMT_ABI_FUNSEL(...): // known type, try to parse it
 * default:
 * 	return EXIT_FAILURE;
 * }
 * @endcode
 *
 * @note user must ensure there are at least 4 bytes in the buffer.
 * This function will fail and return 0 if that is not the case. */
uint32_t
cmt_abi_peek_funsel(cmt_buf_t *me);

/** Consume funsel from the buffer @p me and ensure it matches @p expected_funsel
 *
 * @param [in,out] me       initialized buffer
 * @param [in]     expected expected function selector
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * - EBADMSG in case of a missmatch */
int
cmt_abi_check_funsel(cmt_buf_t *me, uint32_t expected);

/** Decode a unsigned integer of up to 32bytes from the buffer
 *
 * @param [in,out] me     initialized buffer
 * @param [in]     n      size of @p data in bytes
 * @param [out]    data   pointer to a integer
 *
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * - EDOM    value won't fit into @p n bytes. */
int
cmt_abi_get_uint(cmt_buf_t *me, size_t n, void *data);

/** Consume and decode @b address from the buffer
 *
 * @param [in,out] me      initialized buffer
 * @param [out]    address exactly 20 bytes
 *
 * @return
 * - 0 success
 * - ENOBUFS requested size @b n is not available */
int
cmt_abi_get_address(cmt_buf_t *me, uint8_t address[CMT_ADDRESS_LENGTH]);

/** Consume and decode the offset @p of
 *
 * @param [in,out] me initialized buffer
 * @param [out]    of offset to @p bytes data, for use in conjunction with @ref cmt_abi_get_bytes_d
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me */
int
cmt_abi_get_bytes_s(cmt_buf_t *me, cmt_buf_t of[1]);

/** Decode @b bytes from the buffer by taking a pointer to its contents.
 *
 * @param [in]  start initialized buffer (from the start after funsel)
 * @param [out]    of    offset to @p bytes data
 * @param [out]    n     amount of data available in @b bytes
 * @param [out]    bytes memory range with contents
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * @note @p of can be initialized by calling @ref cmt_abi_get_bytes_s */
int
cmt_abi_get_bytes_d(const cmt_buf_t *start, cmt_buf_t of[1], size_t *n, void **bytes);

/** Decode @b bytes from the buffer by taking a pointer to its contents.
 *
 * @param [in]  start initialized buffer (from the start after funsel)
 * @param [out] of    offset to @p bytes data
 * @param [out] n     amount of data available in @b bytes
 * @param [out] bytes memory range with contents
 * @return
 * - 0 success
 * - ENOBUFS no space left in @p me
 * @note @p of can be initialized by calling @ref cmt_abi_get_bytes_s */
int
cmt_abi_peek_bytes_d(const cmt_buf_t *start, cmt_buf_t of[1], cmt_buf_t *bytes);

// raw codec section --------------------------------------------------------

/** Encode @p n bytes of @p data into @p out (up to 32).
 *
 * @param [in]  n    size of @p data in bytes
 * @param [in]  data integer value to encode into @p out
 * @param [out] out  encoded result
 * @return
 * - 0    success
 * - EDOM @p n is too large. */
int
cmt_abi_encode_uint(size_t n, const void *data, uint8_t out[CMT_WORD_LENGTH]);

/** Encode @p n bytes of @p data into @p out (up to 32) in reverse order.
 *
 * @param [in]  n    size of @p data in bytes
 * @param [in]  data integer value to encode into @p out
 * @param [out] out  encoded result
 * @return
 * - 0    success
 * - EDOM @p n is too large.
 * @note use @ref cmt_abi_encode_uint instead */
int
cmt_abi_encode_uint_nr(size_t n, const uint8_t *data, uint8_t out[CMT_WORD_LENGTH]);

/** Encode @p n bytes of @p data into @p out (up to 32) in normal order.
 *
 * @param [in]  n    size of @p data in bytes
 * @param [in]  data integer value to encode into @p out
 * @param [out] out  encoded result
 * @return
 * - 0    success
 * - EDOM @p n is too large.
 * @note use @ref cmt_abi_encode_uint instead */
int
cmt_abi_encode_uint_nn(size_t n, const uint8_t *data, uint8_t out[CMT_WORD_LENGTH]);

/** Decode @p n bytes of @p data into @p out (up to 32).
 *
 * @param [in]  data integer value to decode into @p out
 * @param [in]  n    size of @p data in bytes
 * @param [out] out  decoded output */
int
cmt_abi_decode_uint(const uint8_t data[CMT_WORD_LENGTH], size_t n, uint8_t *out);

/** Decode @p n bytes of @p data into @p out (up to 32) in reverse order.
 *
 * @param [in]  data integer value to decode into @p out
 * @param [in]  n    size of @p data in bytes
 * @param [out] out  decoded output
 * @note if in doubt, use @ref cmt_abi_decode_uint */
int
cmt_abi_decode_uint_nr(const uint8_t data[CMT_WORD_LENGTH], size_t n, uint8_t *out);

/** Decode @p n bytes of @p data into @p out (up to 32) in normal order.
 *
 * @param [in]  data integer value to decode into @p out
 * @param [in]  n    size of @p data in bytes
 * @param [out] out  decoded output
 * @note if in doubt, use @ref cmt_abi_decode_uint */
int
cmt_abi_decode_uint_nn(const uint8_t data[CMT_WORD_LENGTH], size_t n, uint8_t *out);

#endif /* CMT_ABI_H */
/** @} */
