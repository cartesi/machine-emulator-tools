/** @file
 * @defgroup libevm_buf buf
 * slice of contiguous memory
 *
 * @ingroup libevm
 * @{ */
#ifndef EVM_BUF_H
#define EVM_BUF_H
#include<stdint.h>
#include<stddef.h>

/** A slice of contiguous memory from @b p to @b q.
 * Size can be taken with: `q - p`. */
typedef struct {
	uint8_t *p; /**< start of memory region */
	uint8_t *q; /**< end of memory region */
} evm_buf_t;

/** Initialize a @b evm_buf_t */
#define EVM_BUF_INIT(S, N, XS)      \
{	.p = (uint8_t *)(XS),       \
	.q = (uint8_t *)(XS) + (N), \
}

/** Declare a evm_buf_t with memory backed by the stack.
 * @param [in] N - size in bytes */
#define EVM_BUF_DECL(S, N) evm_buf_t S = \
{	.p = (uint8_t [N]){0},           \
	.q = (&S)->p + N,                \
}

/** Initialize @p me buffer backed by @p data, @p n bytes in size
 *
 * @param [out] me   a uninitialized instance
 * @param [in]  n    size in bytes of @b data
 * @param [in]  data the backing memory to be used. 
 *
 * @note @p data memory must outlive @p me.
 * user must copy the contents otherwise */
void evm_buf_init(evm_buf_t *me, size_t n, void *data);

/** Split a buffer in two, @b lhs with @b n bytes and @b rhs with the rest
 *
 * @param [in,out] me  initialized buffer
 * @param [in]     n   bytes in @b lhs
 * @param [out]    lhs left hand side
 * @param [out]    rhs right hand side
 *
 * @return
 * - 0 success
 * - ENOBUFS requested size @b n is not available */
int evm_buf_split(const evm_buf_t *me, size_t n, evm_buf_t *lhs, evm_buf_t *rhs);

/** Length in bytes of @p me, can also signify the space left if used as iterator.
 *
 * @param [in] me     initialized buffer
 * @return
 * - size in bytes */
size_t evm_buf_length(const evm_buf_t *me);

/** Print the contents of @b me buffer to stdout
 *
 * @param [in] p     start of memory region
 * @param [in] q     end of memory region
 * @param [in] l     bytes per line (must be a power of 2). */
void evm_buf_xxd(void *p, void *q, int l);

#endif /* EVM_BUF_H */
/** @} */
