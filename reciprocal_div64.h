#ifndef RECIPROCAL_DIV64_H
#define RECIPROCAL_DIV64_H

#include <immintrin.h>

#include "helper.h"

/*
 * This algorithm is based on the paper "Division by Invariant
 * Integers Using Multiplication" by Torbjörn Granlund and Peter
 * L. Montgomery.
 *
 * The assembler implementation from Agner Fog, which this code is
 * based on, can be found here:
 * http://www.agner.org/optimize/asmlib.zip
 *
 * This optimization for A/B is helpful if the divisor B is mostly
 * runtime invariant. The reciprocal of B is calculated in the
 * slow-path with reciprocal_value(). The fast-path can then just use
 * a much faster multiplication operation with a variable dividend A
 * to calculate the division A/B.
 */

struct reciprocal_value64 {
	u64 m;
	u8 sh1, sh2;
};

static inline struct reciprocal_value64 reciprocal_value64(u64 d)
{
	struct reciprocal_value64 R;
	__uint128_t m;
	int l;

	BUILD_BUG_ON((sizeof(unsigned long long)) != (sizeof(u64)));
	l = ((sizeof(unsigned long long)*8)) - __builtin_clzll(d - 1);
	m = (((__uint128_t)1 << 64) * ((1ULL << l) - d));
        m /= d;
	++m;
	R.m = (u64)m;
	R.sh1 = helpermin(l, 1);
	R.sh2 = helpermax(l - 1, 0);

	return R;
}

static inline u64 reciprocal_divide64(u64 a, struct reciprocal_value64 R)
{
	u64 t = (u64)(((__uint128_t)a * R.m) >> 64);
	return (t + ((a - t) >> R.sh1)) >> R.sh2;
}

static __always_inline uint64_t reciprocal_remainder64(uint64_t A, uint64_t B, struct reciprocal_value64 R)
{
	uint64_t div, mod;

	div = reciprocal_divide64(A, R);
	mod = A - (uint64_t) (div * B);
	if (mod >= B) mod -= B;
	return mod;
}

#endif /* RECIPROCAL_DIV64_H */

