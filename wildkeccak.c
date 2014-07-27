// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Modified for CPUminer by Lucas Jones

// Memory-hard extension of keccak for PoW
// Copyright (c) 2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include "cpuminer-config.h"
#include <string.h>

#include "miner.h"
#include "reciprocal_div64.h"

enum {
  HASH_SIZE = 32,
};

#define KK_MIXIN_SIZE 24
__attribute__((const)) static inline uint64_t rotl641(uint64_t x) { return((x << 1) | (x >> 63)); }
__attribute__((const)) static inline uint64_t rotl64_1(uint64_t x, uint64_t y) { return((x << y) | (x >> (64 - y))); }
__attribute__((const)) static inline uint64_t bitselect(uint64_t a, uint64_t b, uint64_t c) { return(a ^ (c & (b ^ a))); }

static __always_inline void keccakf_mul(uint64_t *s)
{
    uint64_t bc[5], t[5];
    uint64_t tmp1, tmp2;
	int i;
	
	for(i = 0; i < 5; i++)
		t[i] = s[i + 0] ^ s[i + 5] ^ s[i + 10] * s[i + 15] * s[i + 20];
	
	bc[0] = t[0] ^ rotl641(t[2]);
	bc[1] = t[1] ^ rotl641(t[3]);
	bc[2] = t[2] ^ rotl641(t[4]);
	bc[3] = t[3] ^ rotl641(t[0]);
	bc[4] = t[4] ^ rotl641(t[1]);
	
	tmp1 = s[1] ^ bc[0];
	
	s[0] ^= bc[4];
	s[1] = rotl64_1(s[6] ^ bc[0], 44);
	s[6] = rotl64_1(s[9] ^ bc[3], 20);
	s[9] = rotl64_1(s[22] ^ bc[1], 61);
	s[22] = rotl64_1(s[14] ^ bc[3], 39);
	s[14] = rotl64_1(s[20] ^ bc[4], 18);
	s[20] = rotl64_1(s[2] ^ bc[1], 62);
	s[2] = rotl64_1(s[12] ^ bc[1], 43);
	s[12] = rotl64_1(s[13] ^ bc[2], 25);
	s[13] = rotl64_1(s[19] ^ bc[3], 8);
	s[19] = rotl64_1(s[23] ^ bc[2], 56);
	s[23] = rotl64_1(s[15] ^ bc[4], 41);
	s[15] = rotl64_1(s[4] ^ bc[3], 27);
	s[4] = rotl64_1(s[24] ^ bc[3], 14);
	s[24] = rotl64_1(s[21] ^ bc[0], 2);
	s[21] = rotl64_1(s[8] ^ bc[2], 55);
	s[8] = rotl64_1(s[16] ^ bc[0], 45);
	s[16] = rotl64_1(s[5] ^ bc[4], 36);
	s[5] = rotl64_1(s[3] ^ bc[2], 28);
	s[3] = rotl64_1(s[18] ^ bc[2], 21);
	s[18] = rotl64_1(s[17] ^ bc[1], 15);
	s[17] = rotl64_1(s[11] ^ bc[0], 10);
	s[11] = rotl64_1(s[7] ^ bc[1], 6);
	s[7] = rotl64_1(s[10] ^ bc[4], 3);
	s[10] = rotl64_1(tmp1, 1);
	
	tmp1 = s[0]; tmp2 = s[1]; s[0] = bitselect(s[0] ^ s[2], s[0], s[1]); s[1] = bitselect(s[1] ^ s[3], s[1], s[2]); s[2] = bitselect(s[2] ^ s[4], s[2], s[3]); s[3] = bitselect(s[3] ^ tmp1, s[3], s[4]); s[4] = bitselect(s[4] ^ tmp2, s[4], tmp1);
	tmp1 = s[5]; tmp2 = s[6]; s[5] = bitselect(s[5] ^ s[7], s[5], s[6]); s[6] = bitselect(s[6] ^ s[8], s[6], s[7]); s[7] = bitselect(s[7] ^ s[9], s[7], s[8]); s[8] = bitselect(s[8] ^ tmp1, s[8], s[9]); s[9] = bitselect(s[9] ^ tmp2, s[9], tmp1);
	tmp1 = s[10]; tmp2 = s[11]; s[10] = bitselect(s[10] ^ s[12], s[10], s[11]); s[11] = bitselect(s[11] ^ s[13], s[11], s[12]); s[12] = bitselect(s[12] ^ s[14], s[12], s[13]); s[13] = bitselect(s[13] ^ tmp1, s[13], s[14]); s[14] = bitselect(s[14] ^ tmp2, s[14], tmp1);
	tmp1 = s[15]; tmp2 = s[16]; s[15] = bitselect(s[15] ^ s[17], s[15], s[16]); s[16] = bitselect(s[16] ^ s[18], s[16], s[17]); s[17] = bitselect(s[17] ^ s[19], s[17], s[18]); s[18] = bitselect(s[18] ^ tmp1, s[18], s[19]); s[19] = bitselect(s[19] ^ tmp2, s[19], tmp1);
	tmp1 = s[20]; tmp2 = s[21]; s[20] = bitselect(s[20] ^ s[22], s[20], s[21]); s[21] = bitselect(s[21] ^ s[23], s[21], s[22]); s[22] = bitselect(s[22] ^ s[24], s[22], s[23]); s[23] = bitselect(s[23] ^ tmp1, s[23], s[24]); s[24] = bitselect(s[24] ^ tmp2, s[24], tmp1);
	s[0] ^= 0x0000000000000001ULL;
}

static __always_inline void wildkeccak(uint64_t *restrict st, const uint64_t *restrict pscr, uint64_t scr_size, struct reciprocal_value64 recip)
{
    uint64_t x, i;
    uint64_t idx[KK_MIXIN_SIZE];
	
	keccakf_mul(st);
	
    for (i = 1; i < KK_MIXIN_SIZE; ++i)
    {
        /* force CPU to prefetch cache line from RAM in the background */
        for (x = 0; x < KK_MIXIN_SIZE; x++)
        {
            idx[x] = reciprocal_remainder64(st[x], scr_size, recip) << 2;
            prefetch1(&pscr[idx[x]]);
        }

#if defined(__AVX2__)
#warning using AVX2 optimizations
        __m256i *st0 = (__m256i *)st;
        for(x = 0; x < KK_MIXIN_SIZE >> 2; ++x)
        {
            *st0 = _mm256_xor_si256(*st0, *((__m256i *)&pscr[idx[x*4 + 0]]));
            *st0 = _mm256_xor_si256(*st0, *((__m256i *)&pscr[idx[x*4 + 1]]));
            *st0 = _mm256_xor_si256(*st0, *((__m256i *)&pscr[idx[x*4 + 2]]));
            *st0 = _mm256_xor_si256(*st0, *((__m256i *)&pscr[idx[x*4 + 3]]));
            st0++;
        }
#elif defined(__SSE2__)
#warning using SSE2 optimizations
        __m128i *st0 = (__m128i *)st;
        for(x = 0; x < KK_MIXIN_SIZE >> 2; ++x)
        {
            st0[0] = _mm_xor_si128(st0[0], *((__m128i *)&pscr[idx[x*4 + 0]]));
            st0[0] = _mm_xor_si128(st0[0], *((__m128i *)&pscr[idx[x*4 + 1]]));
            st0[0] = _mm_xor_si128(st0[0], *((__m128i *)&pscr[idx[x*4 + 2]]));
            st0[0] = _mm_xor_si128(st0[0], *((__m128i *)&pscr[idx[x*4 + 3]]));
            st0[1] = _mm_xor_si128(st0[1], *((__m128i *)&pscr[idx[x*4 + 0] + 2]));
            st0[1] = _mm_xor_si128(st0[1], *((__m128i *)&pscr[idx[x*4 + 1] + 2]));
            st0[1] = _mm_xor_si128(st0[1], *((__m128i *)&pscr[idx[x*4 + 2] + 2]));
            st0[1] = _mm_xor_si128(st0[1], *((__m128i *)&pscr[idx[x*4 + 3] + 2]));
            st0 += 2;
        }
#else
#warning using non-optimized 64bit operations
        for(x = 0; x < KK_MIXIN_SIZE; x += 4) {
            st[x+0] ^= pscr[idx[x + 0] + 0] ^ pscr[idx[x + 1] + 0] ^ pscr[idx[x + 2] + 0] ^ pscr[idx[x + 3] + 0];
            st[x+1] ^= pscr[idx[x + 0] + 1] ^ pscr[idx[x + 1] + 1] ^ pscr[idx[x + 2] + 1] ^ pscr[idx[x + 3] + 1];
            st[x+2] ^= pscr[idx[x + 0] + 2] ^ pscr[idx[x + 1] + 2] ^ pscr[idx[x + 2] + 2] ^ pscr[idx[x + 3] + 2];
            st[x+3] ^= pscr[idx[x + 0] + 3] ^ pscr[idx[x + 1] + 3] ^ pscr[idx[x + 2] + 3] ^ pscr[idx[x + 3] + 3];
        }
#endif
		keccakf_mul(st);
    }
}

static void __always_inline wild_keccak_hash_dbl(const uint8_t *in, size_t inlen, uint8_t *md, const uint64_t* pscr, uint64_t scr_size)
{
    uint64_t st[25] __aligned(32);
    struct reciprocal_value64 recip;

    scr_size >>= 2; /* scr_size now in crypto::hash units (32 bytes) */
    recip = reciprocal_value64(scr_size);

    // Wild Keccak #1
    memcpy(st, in, 81);
    memset(&st[11], 0, 112);
    st[10] = (st[10] & 0x00000000000000FFULL) | 0x0000000000000100ULL;
    st[16] |= 0x8000000000000000ULL;
    wildkeccak(st, pscr, scr_size, recip);

    // Wild Keccak #2 - st[0]..st[3] contains resulting hash of #1
    memset(&st[5], 0, 160);
    st[4] = 0x0000000000000001ULL;
    st[16] |= 0x8000000000000000ULL;
    wildkeccak(st, pscr, scr_size, recip);

    memcpy(md, st, 32);
}

void wild_keccak_hash_dbl_use_global_scratch(const uint8_t *in, size_t inlen, uint8_t *md)
{
    wild_keccak_hash_dbl(in, inlen, md, (uint64_t*)pscratchpad_buff, (uint64_t)scratchpad_size);
}

int scanhash_wildkeccak(int thr_id, uint32_t *pdata, const uint32_t *ptarget, uint32_t max_nonce, unsigned long *hashes_done)
{
    uint32_t *nonceptr = (uint32_t*) (((char*)pdata) + 1);
    uint32_t n = *nonceptr - 1;
    const uint32_t first_nonce = n + 1;
    const uint32_t Htarg = ptarget[7];
    uint32_t hash[HASH_SIZE / 4] __attribute__((aligned(32)));

    do {
        *nonceptr = ++n;
        wild_keccak_hash_dbl_use_global_scratch((uint8_t*)pdata, 81, (uint8_t*)hash);
        //if (unlikely(  *((uint64_t*)&hash[6])    <   *((uint64_t*)&ptarget[6]) ))
        if (unlikely(hash[7] < ptarget[7])) {
            *hashes_done = n - first_nonce + 1;
            return true;
        }
    } while (likely((n <= max_nonce && !work_restart[thr_id].restart)));

    *hashes_done = n - first_nonce + 1;
    return 0;
}
