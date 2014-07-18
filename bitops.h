#ifndef _BITOPS_H
#define _BITOPS_H

#include <stdint.h>

#include "helper.h"

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
/* Technically wrong, but this avoids compilation errors on some gcc
 *    versions. */
#define BITOP_ADDR(x) "=m" (*(volatile long *) (x))
#else
#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))
#endif

#define ADDR                            BITOP_ADDR(addr)

/*
 * We do the locked ops that don't return the old value as
 * a mask operation on a byte.
 */
#define IS_IMMEDIATE(nr)                (__builtin_constant_p(nr))
#define CONST_MASK_ADDR(nr, addr)       BITOP_ADDR((void *)(addr) + ((nr)>>3))
#define CONST_MASK(nr)                  (1 << ((nr) & 7))

#if defined(__x86_64__) || defined(__i386__)

#define BIT(nr)                 (1UL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE           8
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

static inline unsigned long __ffz(unsigned long word)
{
    asm volatile("bsf %1,%0"
                  :"=r" (word)
                  :"r" (~word));
    return word;
}

static inline unsigned long __ffs(unsigned long word)
{
    asm volatile("bsf %1,%0"
                :"=r" (word)
                :"rm" (word));
    return word;
}

static inline unsigned long __fls(unsigned long word)
{
    asm volatile("bsr %1,%0"
                  :"=r" (word)
                  :"rm" (word));
    return word;
}

static __inline__ int get_bitmask_order(unsigned int count)
{
	int order;

	order = __fls(count);
	return order;   /* We could be slightly more clever with -1 here... */
}

static __inline__ int get_count_order(unsigned int count)
{
	int order;

	order = __fls(count) - 1;
	if (count & (count - 1))
		order++;
	return order;
}

/**
 * ffs - find first set bit in word
 * @x: the word to search
 *
 * This is defined the same way as the libc and compiler builtin ffs
 * routines, therefore differs in spirit from the other bitops.
 *
 * ffs(value) returns 0 if value is 0 or the position of the first
 * set bit if value is nonzero. The first (least significant) bit
 * is at position 1.
 */
static inline int helper_ffs(int x)
{
	int r;
        /*
         * AMD64 says BSFL won't clobber the dest reg if x==0; Intel64 says the
         * dest reg is undefined if x==0, but their CPU architect says its
         * value is written to set it to the same as before, except that the
         * top 32 bits will be cleared.
         *
         * We cannot do this on 32 bits because at the very least some
         * 486 CPUs did not behave this way.
         */
        long tmp = -1;
        asm("bsfl %1,%0"
            : "=r" (r)
            : "rm" (x), "0" (tmp));

	return r + 1;
}

/**
 * fls - find last set bit in word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffs, but returns the position of the most significant set bit.
 *
 * fls(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 32.
 */
static inline int fls(int x)
{
	int r;

        /*
         * AMD64 says BSRL won't clobber the dest reg if x==0; Intel64 says the
         * dest reg is undefined if x==0, but their CPU architect says its
         * value is written to set it to the same as before, except that the
         * top 32 bits will be cleared.
         *
         * We cannot do this on 32 bits because at the very least some
         * 486 CPUs did not behave this way.
         */
        long tmp = -1;
        asm("bsrl %1,%0"
            : "=r" (r)
            : "rm" (x), "0" (tmp));
	return r + 1;
}

/**
 * fls64 - find last set bit in a 64-bit word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffsll, but returns the position of the most significant set bit.
 *
 * fls64(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 64.
 */
#if BITS_PER_LONG == 32
static __always_inline int fls64(uint64_t x)
{
	uint32_t h = x >> 32;
	if (h)
		return fls(h) + 32;
	return fls(x);
}
#elif BITS_PER_LONG == 64
static __always_inline int fls64(uint64_t x)
{
        long bitpos = -1;
        /*
         * AMD64 says BSRQ won't clobber the dest reg if x==0; Intel64 says the
         * dest reg is undefined if x==0, but their CPU architect says its
         * value is written to set it to the same as before.
         */
        asm("bsrq %1,%0"
            : "+r" (bitpos)
            : "rm" (x));
        return bitpos + 1;
}
#else
#error BITS_PER_LONG not 32 or 64
#endif

static inline unsigned fls_long(unsigned long l)
{
	if (sizeof(l) == 4)
		return fls(l);
	return fls64(l);
}

/**
 * __ffs64 - find first set bit in a 64 bit word
 * @word: The 64 bit word
 *
 * On 64 bit arches this is a synomyn for __ffs
 * The result is not defined if no bits are set, so check that @word
 * is non-zero before calling this.
 */
static inline unsigned long __ffs64(uint64_t word)
{
#if BITS_PER_LONG == 32
	if (((u32)word) == 0UL)
		return __ffs((u32)(word >> 32)) + 32;
#elif BITS_PER_LONG != 64
#error BITS_PER_LONG not 32 or 64
#endif
	return __ffs((unsigned long)word);
}

#else
#  error only i386 and x86_64 supported
#endif

#endif

