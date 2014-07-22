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
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

static __always_inline void keccakf_mul(uint64_t s[25])
{
    uint64_t t[5], u[5], v, w;

    /* theta: c = a[0,i] ^ a[1,i] ^ .. a[4,i] */
    t[0] = s[0] ^ s[5] ^ s[10] * s[15] * s[20];
    t[1] = s[1] ^ s[6] ^ s[11] * s[16] * s[21];
    t[2] = s[2] ^ s[7] ^ s[12] * s[17] * s[22];
    t[3] = s[3] ^ s[8] ^ s[13] * s[18] * s[23];
    t[4] = s[4] ^ s[9] ^ s[14] * s[19] * s[24];

    /* theta: d[i] = c[i+4] ^ rotl(c[i+1],1) */
    u[0] = t[4] ^ ROTL64(t[1], 1);
    u[1] = t[0] ^ ROTL64(t[2], 1);
    u[2] = t[1] ^ ROTL64(t[3], 1);
    u[3] = t[2] ^ ROTL64(t[4], 1);
    u[4] = t[3] ^ ROTL64(t[0], 1);

    /* theta: a[0,i], a[1,i], .. a[4,i] ^= d[i] */
    s[0] ^= u[0]; s[5] ^= u[0]; s[10] ^= u[0]; s[15] ^= u[0]; s[20] ^= u[0];
    s[1] ^= u[1]; s[6] ^= u[1]; s[11] ^= u[1]; s[16] ^= u[1]; s[21] ^= u[1];
    s[2] ^= u[2]; s[7] ^= u[2]; s[12] ^= u[2]; s[17] ^= u[2]; s[22] ^= u[2];
    s[3] ^= u[3]; s[8] ^= u[3]; s[13] ^= u[3]; s[18] ^= u[3]; s[23] ^= u[3];
    s[4] ^= u[4]; s[9] ^= u[4]; s[14] ^= u[4]; s[19] ^= u[4]; s[24] ^= u[4];

    /* rho pi: b[..] = rotl(a[..], ..) */
    v = s[ 1];
    s[ 1] = ROTL64(s[ 6], 44);
    s[ 6] = ROTL64(s[ 9], 20);
    s[ 9] = ROTL64(s[22], 61);
    s[22] = ROTL64(s[14], 39);
    s[14] = ROTL64(s[20], 18);
    s[20] = ROTL64(s[ 2], 62);
    s[ 2] = ROTL64(s[12], 43);
    s[12] = ROTL64(s[13], 25);
    s[13] = ROTL64(s[19],  8);
    s[19] = ROTL64(s[23], 56);
    s[23] = ROTL64(s[15], 41);
    s[15] = ROTL64(s[ 4], 27);
    s[ 4] = ROTL64(s[24], 14);
    s[24] = ROTL64(s[21],  2);
    s[21] = ROTL64(s[ 8], 55);
    s[ 8] = ROTL64(s[16], 45);
    s[16] = ROTL64(s[ 5], 36);
    s[ 5] = ROTL64(s[ 3], 28);
    s[ 3] = ROTL64(s[18], 21);
    s[18] = ROTL64(s[17], 15);
    s[17] = ROTL64(s[11], 10);
    s[11] = ROTL64(s[ 7],  6);
    s[ 7] = ROTL64(s[10],  3);
    s[10] = ROTL64(    v,  1);

    /* chi: a[i,j] ^= ~b[i,j+1] & b[i,j+2] */
    v = s[ 0]; w = s[ 1]; s[ 0] ^= (~w) & s[ 2]; s[ 1] ^= (~s[ 2]) & s[ 3]; s[ 2] ^= (~s[ 3]) & s[ 4]; s[ 3] ^= (~s[ 4]) & v; s[ 4] ^= (~v) & w;
    v = s[ 5]; w = s[ 6]; s[ 5] ^= (~w) & s[ 7]; s[ 6] ^= (~s[ 7]) & s[ 8]; s[ 7] ^= (~s[ 8]) & s[ 9]; s[ 8] ^= (~s[ 9]) & v; s[ 9] ^= (~v) & w;
    v = s[10]; w = s[11]; s[10] ^= (~w) & s[12]; s[11] ^= (~s[12]) & s[13]; s[12] ^= (~s[13]) & s[14]; s[13] ^= (~s[14]) & v; s[14] ^= (~v) & w;
    v = s[15]; w = s[16]; s[15] ^= (~w) & s[17]; s[16] ^= (~s[17]) & s[18]; s[17] ^= (~s[18]) & s[19]; s[18] ^= (~s[19]) & v; s[19] ^= (~v) & w;
    v = s[20]; w = s[21]; s[20] ^= (~w) & s[22]; s[21] ^= (~s[22]) & s[23]; s[22] ^= (~s[23]) & s[24]; s[23] ^= (~s[24]) & v; s[24] ^= (~v) & w;

    /* iota: a[0,0] ^= round constant (first round) */
    s[0] ^= 1;
}

static __always_inline void wildkeccak(uint64_t* st, const uint64_t* pscr, uint64_t scr_size, struct reciprocal_value64 recip)
{
    uint64_t x, i;
    uint64_t idx[KK_MIXIN_SIZE];
    __m128i *st0;

    for (i = 0; i < KK_MIXIN_SIZE; ++i) {
        if (i == 0) goto skipfirst;

        /* force CPU to prefetch cache line from RAM in the background */
        for (x = 0; x < KK_MIXIN_SIZE; x++) {
            idx[x] = reciprocal_remainder64(st[x], scr_size, recip) << 2;
            prefetch1(&pscr[idx[x]]);
        }

        st0 = (__m128i *)st;
        for(x = 0; x <  KK_MIXIN_SIZE / 4; ++x) {
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
skipfirst:
        keccakf_mul(st);
    }
}

static void __always_inline wild_keccak_hash_dbl(const uint8_t *in, size_t inlen, uint8_t *md, const uint64_t* pscr, uint64_t scr_size)
{
    uint64_t st[25] __aligned(32);
    struct reciprocal_value64 recip;

    scr_size >>= 2;
    recip = reciprocal_value64(scr_size);

    // Wild Keccak #1
    memcpy(st, in, 81);
    st[10] = (st[10] & 0x00000000000000FFULL) | 0x0000000000000100ULL;
    memset(&st[11], 0x00, 112);
    st[16] |= 0x8000000000000000ULL;
    wildkeccak(st, pscr, scr_size, recip);

    // Wild Keccak #2
    memset(&st[4], 0x00, 168);
    st[4] = 0x0000000000000001ULL;
    st[16] |= 0x8000000000000000ULL;
    wildkeccak(st, pscr, scr_size, recip);

    memcpy(md, st, 32);
    return;
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
