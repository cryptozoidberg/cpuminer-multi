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
  HASH_DATA_AREA = 136,
};

#define KECCAK_ROUNDS 24
#define KK_MIXIN_SIZE 24
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

static const uint64_t keccakf_rndc[24] = 
{
    0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
    0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
    0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 
    0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

// compute a keccak hash (md) of given byte length from "in"
typedef uint64_t state_t_m[25];

static void keccakf_mul(uint64_t s[25], int rounds)
{
    int i;
    uint64_t t[5], u[5], v, w;

    for (i = 0; i < rounds; i++) {
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

        /* iota: a[0,0] ^= round constant */
        s[0] ^= keccakf_rndc[i];
    }
}

static void cb(uint64_t st[25], const uint64_t* pscr, uint64_t scr_sz, struct reciprocal_value64 recip)
{
    uint64_t idx[4];
    int i;

    for (i = 0; i < KK_MIXIN_SIZE; i += 4) {
        idx[0] = reciprocal_remainder64(st[i+0], scr_sz, recip) * 4;
        idx[1] = reciprocal_remainder64(st[i+1], scr_sz, recip) * 4;
        idx[2] = reciprocal_remainder64(st[i+2], scr_sz, recip) * 4;
        idx[3] = reciprocal_remainder64(st[i+3], scr_sz, recip) * 4;
        st[i+0] ^= pscr[idx[0] + 0] ^ pscr[idx[1] + 0] ^ pscr[idx[2] + 0] ^ pscr[idx[3] + 0];
        st[i+1] ^= pscr[idx[0] + 1] ^ pscr[idx[1] + 1] ^ pscr[idx[2] + 1] ^ pscr[idx[3] + 1];
        st[i+2] ^= pscr[idx[0] + 2] ^ pscr[idx[1] + 2] ^ pscr[idx[2] + 2] ^ pscr[idx[3] + 2];
        st[i+3] ^= pscr[idx[0] + 3] ^ pscr[idx[1] + 3] ^ pscr[idx[2] + 3] ^ pscr[idx[3] + 3];
    }
}

static int wild_keccak(const uint8_t *in, size_t inlen, uint8_t *md, size_t mdlen, const uint64_t* pscr, uint64_t scr_sz, struct reciprocal_value64 recip)
{
    state_t_m st;
    uint8_t temp[144];
    uint64_t rsiz, rsizw;

    rsiz = sizeof(state_t_m) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
    rsizw = rsiz / 8;
    memset(st, 0, sizeof(st));

    for ( ; inlen >= rsiz; inlen -= rsiz, in += rsiz) 
    {
      for (size_t i = 0; i < rsizw; i++)
        st[i] ^= ((uint64_t *) in)[i];

      for(size_t ll = 0; ll != KECCAK_ROUNDS; ll++)
      {
        if(ll != 0)
        {//skip first round
          cb(st, pscr, scr_sz, recip);
        }
        //print_state(&st[0], "before_permut", ll);
        keccakf_mul(st, 1);
        //print_state(&st[0], "after_permut", ll);
      }
    }

    // last block and padding
    memcpy(temp, in, inlen);
    temp[inlen++] = 1;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (size_t i = 0; i < rsizw; i++)
      st[i] ^= ((uint64_t *) temp)[i];

    for(size_t ll = 0; ll != KECCAK_ROUNDS; ll++)
    {
      if(ll != 0)
      {//skip first state with
        cb(st, pscr, scr_sz, recip);
      }
      keccakf_mul(st, 1);
    }

    memcpy(md, st, mdlen);

    return 0;
  }

void wild_keccak_hash_dbl(const uint8_t *in, size_t inlen, uint8_t *md, const uint64_t* pscr, uint64_t scr_sz)
{
    struct reciprocal_value64 recip;

    scr_sz /= 4; /* now in crypto::hash units (32 bytes) */
    if(!scr_sz)
        return;
    recip = reciprocal_value64(scr_sz);
    wild_keccak(in, inlen, md, HASH_SIZE, pscr, scr_sz, recip);
    wild_keccak(md, HASH_SIZE, md, HASH_SIZE, pscr, scr_sz, recip);
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
      if (unlikely(hash[7] < ptarget[7])) 
      {
        *hashes_done = n - first_nonce + 1;
        return true;
      }
    } while (likely((n <= max_nonce && !work_restart[thr_id].restart)));
   
    *hashes_done = n - first_nonce + 1;
    return 0;
}
