/*
 * rng_lxm.c
 * Copyright (c) 2020 Reini Urban. dieharder/gsl integration.

** This software is dual-licensed under the The University of Illinois/NCSA
   Open Source License (NCSA) and The 3-Clause BSD License**

# NCSA Open Source License
**Copyright (c) 2019 Kevin Sheppard. All rights reserved.**

Developed by: Kevin Sheppard (<kevin.sheppard@economics.ox.ac.uk>,
<kevin.k.sheppard@gmail.com>)
[http://www.kevinsheppard.com](http://www.kevinsheppard.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimers.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimers in the documentation and/or
other materials provided with the distribution.

Neither the names of Kevin Sheppard, nor the names of any contributors may be
used to endorse or promote products derived from this Software without specific
prior written permission.

**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.**
 */

#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>

#ifdef HAVE_32BITLONG
#define RNG64_MAX UINT32_MAX
#else
#define RNG64_MAX UINT64_MAX
#endif

/* a in a * s + b
 * https://nuclear.llnl.gov/CNP/rng/rngman/node4.html
 */
#define LCG_MULT 2862933555777941757ULL
#define LCG_ADD 3037000493ULL

struct lxm_state {
  uint64_t x[4]; /* xorshift */
  uint64_t lcg_state;    /* LCG */
  uint64_t b;    /* LCG add value: default 3037000493 */
  int has_uint32;
  uint32_t uinteger;
};

typedef struct lxm_state lxm_state_t;

static inline uint64_t rotl(const uint64_t x, int k) {
  /* http://prng.di.unimi.it/xoshiro256plus.c */
  return (x << k) | (x >> (64 - k));
}

/* Using David Stafford best parameters
 * https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html
 */

static inline uint64_t murmur_hash3(uint64_t key) {
  /* http://prng.di.unimi.it/splitmix64.c */
  key = (key ^ (key >> 30)) * 0xbf58476d1ce4e5b9;
  key = (key ^ (key >> 27)) * 0x94d049bb133111eb;
  return key ^ (key >> 31);
}

static inline void xorshift(lxm_state_t *state) {
  /* http://prng.di.unimi.it/xoshiro256plus.c */
  const uint64_t t = state->x[1] << 17;

  state->x[2] ^= state->x[0];
  state->x[3] ^= state->x[1];
  state->x[1] ^= state->x[2];
  state->x[0] ^= state->x[3];
  state->x[2] ^= t;
  state->x[3] = rotl(state->x[3], 45);
}

static inline void lcg(lxm_state_t *state) {
  /* https://nuclear.llnl.gov/CNP/rng/rngman/node4.html */
  state->lcg_state = LCG_MULT * state->lcg_state + state->b;
}

static inline uint64_t lxm_next64(lxm_state_t *state) {
  uint64_t next_val = murmur_hash3(state->x[0] + state->lcg_state);
  lcg(state);
  xorshift(state);
  return next_val;
}

static inline uint32_t lxm_next32(lxm_state_t *state) {
  uint64_t next;
  if (state->has_uint32) {
    state->has_uint32 = 0;
    return state->uinteger;
  }
  next = lxm_next64(state);
  state->has_uint32 = 1;
  state->uinteger = (uint32_t)(next >> 32);
  return (uint32_t)(next & 0xffffffff);
}

static void lcg_jump(lxm_state_t *state) {
    uint64_t acc_mult = 1u;
    uint64_t acc_plus = 0u;
    uint64_t cur_plus = state->b;
    uint64_t cur_mult = LCG_MULT;
    /* 2^128 has bit 1 in location 128, and 0 else where */
    for (int i=0; i < 129; i++){
        if (i == 128) {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
    }
    state->lcg_state = acc_mult * state->lcg_state + acc_plus;
}

static void xorshift_jump(lxm_state_t *state)
{
  static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa,
                                  0x39abdc4529b1661c};

  uint64_t s0 = 0;
  uint64_t s1 = 0;
  uint64_t s2 = 0;
  uint64_t s3 = 0;
  for (int i = 0; i < (int)(sizeof(JUMP) / sizeof(*JUMP)); i++)
    for (int b = 0; b < 64; b++)
    {
      if (JUMP[i] & UINT64_C(1) << b)
      {
        s0 ^= state->x[0];
        s1 ^= state->x[1];
        s2 ^= state->x[2];
        s3 ^= state->x[3];
      }
      xorshift(state);
    }

  state->x[0] = s0;
  state->x[1] = s1;
  state->x[2] = s2;
  state->x[3] = s3;
}

static void lxm_jump(lxm_state_t *state)
{
  /*
   * lcg jump is a no-op since we are using a multiplier
   * the full cycle
   *
   * lcg_jump(state);
   */
  xorshift_jump(state);
}

static void lxm_set(void *vstate, unsigned long int seed)
{
  lxm_state_t* state = (lxm_state_t*) vstate;
  uint64_t tmp = (uint64_t)seed;
  for (int i = 0; i < 4; i++) {
    state->x[i] = splitmix64_next(&tmp);
  }
}

static unsigned long int lxm_get(void *vstate)
{
  lxm_state_t* state = (lxm_state_t*) vstate;
#ifdef HAVE_32BITLONG
  return (unsigned long int)lxm_next32 (state);
#else
  return (unsigned long int)lxm_next64 (state);
#endif
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double lxm_get_double (void *vstate)
{
  lxm_state_t* state = (lxm_state_t*) vstate;
#ifdef HAVE_32BITLONG
  uint32_t v = lxm_next32(state);
  return (v >> 11) * (1.0/9007199254740992.0);
#else  
  return TO_DOUBLE(lxm_next64 (state));
#endif
}

static const gsl_rng_type lxm_type =
{"lxm",
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (lxm_state_t),
 &lxm_set,
 &lxm_get,
 &lxm_get_double};

const gsl_rng_type *gsl_rng_lxm = &lxm_type;
