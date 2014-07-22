#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>

#if LONG_MAX == 2147483647L
# define BITS_PER_LONG 32
#elif LONG_MAX == 9223372036854775807L
# define BITS_PER_LONG 64
#else
# error Wacky LONG_MAX!
#endif

#define nr_cpu_ids (8)
#define LOCK_PREFIX "\n\tlock; "

#define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
#define PTR_ALIGN(p, a)         ((typeof(p))ALIGN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a)                (((x) & ((typeof(x))(a) - 1)) == 0)

/*
 * swap - swap value of @a and @b
 */
#define helperswap(a, b) \
        do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

/* Force a compilation error if condition is true */
#define BUILD_BUG_ON(condition) ((void)BUILD_BUG_ON_ZERO(condition))

/* Force a compilation error if condition is constant and true */
#define MAYBE_BUILD_BUG_ON(cond) ((void)sizeof(char[1 - 2 * !!(cond)]))

/* Force a compilation error if a constant expression is not a power of 2 */
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)                  \
        BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))

/*
 * Prevent the compiler from merging or refetching accesses.  The compiler
 * is also forbidden from reordering successive instances of ACCESS_ONCE(),
 * but only when the compiler is aware of some particular ordering.  One way
 * to make the compiler aware of ordering is to put the two invocations of
 * ACCESS_ONCE() in different C statements.
 *
 * This macro does absolutely -nothing- to prevent the CPU from reordering,
 * merging, or refetching absolutely anything at any time.  Its main intended
 * use is to mediate communication between process-level code and irq/NMI
 * handlers, all running on the same CPU.
 */
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

/*
 * From the GCC manual:
 *
 * Many functions have no effects except the return value and their
 * return value depends only on the parameters and/or global
 * variables.  Such a function can be subject to common subexpression
 * elimination and loop optimization just as an arithmetic operator
 * would be.
 * [...]
 */
#define __pure                          __attribute__((pure))
#define __aligned(x)                    __attribute__((aligned(x)))
#define __printf(a,b)                   __attribute__((format(printf,a,b)))
#define  noinline                       __attribute__((noinline))
#ifndef __attribute_const__
#define __attribute_const__             __attribute__((__const__))
#endif
#define __maybe_unused                  __attribute__((unused))
#define __always_unused                 __attribute__((unused))
#ifndef __always_inline
#define __always_inline         inline __attribute__((always_inline))
#endif

#if defined(__clang__)
#define uninitialized_var(x) x = *(&(x))
#else
#define uninitialized_var(x) x = x
#endif

#if defined(__INTEL_COMPILER)
/* Intel ECC compiler doesn't support __builtin_types_compatible_p() */
#define __must_be_array(a) 0

#ifndef __HAVE_BUILTIN_BSWAP16__
/* icc has this, but it's called _bswap16 */
#define __HAVE_BUILTIN_BSWAP16__
#define __builtin_bswap16 _bswap16
#endif
#endif

#ifndef offsetof
#define offsetof(___TYPE, ___MEMBER) __builtin_offsetof(___TYPE, ___MEMBER)
#endif

/* To avoid that a compiler optimizes certain memset calls away, these
 *    macros may be used instead. */
#define wipememory_set(_ptr,_set,_len) do { \
              volatile char *_vptr=(volatile char *)(_ptr); \
              size_t _vlen=(_len); \
              while(_vlen) { *_vptr=(_set); _vptr++; _vlen--; } \
                  } while(0)
#define wipememory(_ptr,_len) wipememory_set(_ptr,0,_len)

/*
 * min()/max()/clamp() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define helpermin(x,y) ({             \
	__typeof__(x) _min1 = (x);     \
	__typeof__(y) _min2 = (y);     \
	(void) (&_min1 == &_min2);    \
	_min1 < _min2 ? _min1 : _min2; })

#define helpermax(x,y) ({             \
	__typeof__(x) _max1 = (x);     \
	__typeof__(y) _max2 = (y);     \
	(void) (&_max1 == &_max2);    \
	_max1 > _max2 ? _max1 : _max2; })

#define helpermin3(x, y, z) ({                        \
	typeof(x) _min1 = (x);                  \
	typeof(y) _min2 = (y);                  \
	typeof(z) _min3 = (z);                  \
	(void) (&_min1 == &_min2);              \
	(void) (&_min1 == &_min3);              \
	_min1 < _min2 ? (_min1 < _min3 ? _min1 : _min3) : \
		(_min2 < _min3 ? _min2 : _min3); })

#define helpermax3(x, y, z) ({                        \
	typeof(x) _max1 = (x);                  \
	typeof(y) _max2 = (y);                  \
	typeof(z) _max3 = (z);                  \
	(void) (&_max1 == &_max2);              \
	(void) (&_max1 == &_max3);              \
	_max1 > _max2 ? (_max1 > _max3 ? _max1 : _max3) : \
		(_max2 > _max3 ? _max2 : _max3); })

/**
 *  * min_not_zero - return the minimum that is _not_ zero, unless both are zero
 *   * @x: value1
 *    * @y: value2
 *     */
#define min_not_zero(x, y) ({                   \
	typeof(x) __x = (x);                    \
	typeof(y) __y = (y);                    \
	__x == 0 ? __y : ((__y == 0) ? __x : min(__x, __y)); })

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */
#define min_t(type, x, y) ({                    \
        type __min1 = (x);                      \
        type __min2 = (y);                      \
        __min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({                    \
        type __max1 = (x);                      \
        type __max2 = (y);                      \
        __max1 > __max2 ? __max1: __max2; })

/**
 * clamp - return a value clamped to a given range with strict typechecking
 * @val: current value
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * This macro does strict typechecking of min/max to make sure they are of the
 * same type as val.  See the unnecessary pointer comparisons.
 */
#define clamp(val, min, max) ({                        \
       typeof(val) __val = (val);              \
       typeof(min) __min = (min);              \
       typeof(max) __max = (max);              \
       (void) (&__val == &__min);              \
       (void) (&__val == &__max);              \
       __val = __val < __min ? __min: __val;   \
       __val > __max ? __max: __val; })

/**
 * clamp_t - return a value clamped to a given range using a given type
 * @type: the type of variable to use
 * @val: current value
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * This macro does no typechecking and uses temporary variables of type
 * 'type' to make all the comparisons.
 */
#define clamp_t(type, val, min, max) ({                \
       type __val = (val);                     \
       type __min = (min);                     \
       type __max = (max);                     \
       __val = __val < __min ? __min: __val;   \
       __val > __max ? __max: __val; })

/**
 * clamp_val - return a value clamped to a given range using val's type
 * @val: current value
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * This macro does no typechecking and uses temporary variables of whatever
 * type the input argument 'val' is.  This is useful when val is an unsigned
 * type and min and max are literals that will otherwise be assigned a signed
 * integer type.
 */
#define clamp_val(val, min, max) ({            \
       typeof(val) __val = (val);              \
       typeof(val) __min = (min);              \
       typeof(val) __max = (max);              \
       __val = __val < __min ? __min: __val;   \
       __val > __max ? __max: __val; })

/* Data needs not stay in cache */
static inline void prefetch0(const void *ptr)
{
  __builtin_prefetch(ptr, 0, 0);
}

static inline void prefetch1(const void *ptr)
{
  __builtin_prefetch(ptr, 0, 1);
}

static inline void prefetch2(const void *ptr)
{
  __builtin_prefetch(ptr, 0, 2);
}

/* High degree of locality */
static inline void prefetch3(const void *ptr)
{
  __builtin_prefetch(ptr, 0, 3);
}

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

struct __una_u16 { u16 x __attribute__((packed)); };
struct __una_u32 { u32 x __attribute__((packed)); };
struct __una_u64 { u64 x __attribute__((packed)); };

static inline u16 __get_unaligned_cpu16(const void *p)
{
	const struct __una_u16 *ptr = (const struct __una_u16 *)p;
	return ptr->x;
}

static inline u32 __get_unaligned_cpu32(const void *p)
{
	const struct __una_u32 *ptr = (const struct __una_u32 *)p;
	return ptr->x;
}

static inline u64 __get_unaligned_cpu64(const void *p)
{
	const struct __una_u64 *ptr = (const struct __una_u64 *)p;
	return ptr->x;
}

static inline void __put_unaligned_cpu16(u16 val, void *p)
{
	struct __una_u16 *ptr = (struct __una_u16 *)p;
	ptr->x = val;
}

static inline void __put_unaligned_cpu32(u32 val, void *p)
{
	struct __una_u32 *ptr = (struct __una_u32 *)p;
	ptr->x = val;
}

static inline void __put_unaligned_cpu64(u64 val, void *p)
{
	struct __una_u64 *ptr = (struct __una_u64 *)p;
	ptr->x = val;
}

/*
 * The next routines deal with comparing 16/32/64 bit unsigned ints
 * and worry about wraparound (automatic with unsigned arithmetic).
 */

static inline int before16(u16 seq1, u16 seq2)
{
  return (s16)(seq1-seq2) < 0;
}
#define after16(seq2, seq1)       before16(seq1, seq2)

/* is s2<=s1<=s3 ? */
static inline int between16(u16 seq1, u16 seq2, u16 seq3)
{
  return seq3 - seq2 >= seq1 - seq2;
}

static inline int before32(u32 seq1, u32 seq2)
{
  return (s32)(seq1-seq2) < 0;
}
#define after32(seq2, seq1)       before32(seq1, seq2)

static inline int between32(u32 seq1, u32 seq2, u32 seq3)
{
  return seq3 - seq2 >= seq1 - seq2;
}

static inline int before64(u64 seq1, u64 seq2)
{
  return (s64)(seq1-seq2) < 0;
}
#define after64(seq2, seq1)       before64(seq1, seq2)

/* is s2<=s1<=s3 ? */
static inline int between64(u64 seq1, u64 seq2, u64 seq3)
{
  return seq3 - seq2 >= seq1 - seq2;
}

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({	type __dummy; \
	typeof(x) __dummy2; \
	(void)(&__dummy == &__dummy2); \
	1; \
})

/*
 * Check at compile time that 'function' is a certain type, or is a pointer
 * to that type (needs to use typedef for the function type.)
 */
#define typecheck_fn(type,function) \
({	typeof(type) __tmp = function; \
	(void)__tmp; \
})

#endif
