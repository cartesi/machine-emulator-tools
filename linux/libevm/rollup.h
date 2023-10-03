/** @file
 * @defgroup libevm_rollup rollup
 * rollup abstraction layer
 *
 * Abstract ioctl calls when running on real hardware. Mock calls otherwise.
 *
 * @ingroup libevm
 * @{ */
#ifndef EVM_ROLLUP_H
#define EVM_ROLLUP_H
#include"buf.h"
#include"merkle.h"

/** Opaque rollup state, initialize with:
 * - @ref evm_rollup_init */
typedef struct {
	void *priv;
	evm_buf_t tx[1],
	          rx[1];
	evm_merkle_t outputs;
} evm_rollup_t;

/** Initialize a @ref evm_rollup_t state.
 *
 * @param [in] me    uninitialized state
 * @param [in] tx    buffer used as tx
 * @param [in] rx    buffer used as rx
 *
 * @note backing memory from @p tx and @p rx must outlive @p me */
void
evm_rollup_init(evm_rollup_t *me);

/**
 */
int
evm_rollup_emit_output(evm_rollup_t *me, size_t n);

/**
 */
int
evm_rollup_emit_verifiable_output(evm_rollup_t *me, size_t n);

/**
 */
int
evm_rollup_await_input(evm_rollup_t *me, size_t *n);

// impl ------------------------------------------------------------------------
/**
 */
int
evm_rollup_init_impl(evm_rollup_t *me
                    ,evm_buf_t tx[1]
                    ,evm_buf_t rx[1]
                    ,void *priv);

/**
 */
int
evm_rollup_emit_output_impl(evm_rollup_t *me, size_t n);

#endif /* EVM_ROLLUP_H */
