// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Modified for CPUminer by Lucas Jones

//#include "cpuminer-config.h"
#include "miner.h"
#include "wildkeccak/KeccakNISTInterface.h"

enum {
  HASH_SIZE = 32
};

void wild_keccak_hash_dbl(const uint8_t *in, size_t inlen, uint8_t *md, const UINT64* pscr, UINT64 scr_sz)
{      
  if(!scr_sz)
    return;

  Hash(256, in, inlen*8, md, pscr, scr_sz);
  Hash(256, md, HASH_SIZE*8, md, pscr, scr_sz);
}


void wild_keccak_hash_dbl_use_global_scratch(const uint8_t *in, size_t inlen, uint8_t *md)
{      
  wild_keccak_hash_dbl(in, inlen, md, (UINT64*)pscratchpad_buff, (UINT64)scratchpad_size);
}

int scanhash_wildkeccak(int thr_id, uint32_t *pdata, const uint32_t *ptarget, uint32_t max_nonce, unsigned long *hashes_done) {
    
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
