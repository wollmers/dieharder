/*
 * rng_rdrand.c
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

#include <dieharder/libdieharder.h>

#define ITERATION_SIZE_L2 5
#define ITERATION_SIZE 32 /* 1 << ITERATION_SIZE_L2 */
#define INDIRECTION_SIZE_L2 4
#define INDIRECTION_SIZE 16 /* 1 << INDIRECTION_SIZE_L2 */

static inline uint64_t rotate(uint64_t x, int k) {
#ifdef _MSC_VER
  return _rotl64(x, k);
#else
  return (x << k) | (x >> (64 - k));
#endif
}

typedef struct efiix_state {
  uint64_t indirection_table[INDIRECTION_SIZE];
  uint64_t iteration_table[ITERATION_SIZE];
  uint64_t i, a, b, c;
  int has_uint32;
  uint32_t uinteger;
} efiix64_state_t;

typedef struct arbee_state {
  uint64_t a, b, c, d, e, i;
} arbee_state_t;

static inline uint64_t efiix64_raw64(efiix64_state_t *state) {
  uint64_t iterated = state->iteration_table[state->i % ITERATION_SIZE];
  uint64_t indirect = state->indirection_table[state->c % INDIRECTION_SIZE];
  state->indirection_table[state->c % INDIRECTION_SIZE] = iterated + state->a;
  state->iteration_table[state->i % ITERATION_SIZE] = indirect;
  uint64_t old = state->a ^ state->b;

  state->a = state->b + state->i++;
  state->b = state->c + indirect;
  state->c = old + rotate(state->c, 25);
  return state->b ^ iterated;
}

static inline uint64_t efiix64_next64(efiix64_state_t *state) {
  return efiix64_raw64(state);
}

static inline uint32_t efiix64_next32(efiix64_state_t *state) {
  uint64_t next;
  if (state->has_uint32) {
    state->has_uint32 = 0;
    return state->uinteger;
  }
  next = efiix64_raw64(state);
  state->has_uint32 = 1;
  state->uinteger = (uint32_t)(next >> 32);
  return (uint32_t)next;
}

static uint64_t arbee_raw64(arbee_state_t *state) {
    uint64_t e = state->a + rotate(state->b, 45);
    state->a = state->b ^ rotate(state->c, 13);
    state->b = state->c + rotate(state->d, 37);
    state->c = e + state->d + state->i++;
    state->d = e + state->a;
    return state->d;
}

static void arbee_mix(arbee_state_t *state) {
    for (int x = 0; x < 12; x++) {
        arbee_raw64(state);
    }
}

static void arbee_seed(arbee_state_t *state, uint64_t seed1, uint64_t seed2, uint64_t seed3,
                uint64_t seed4) {
    state->a = seed1;
    state->b = seed2;
    state->c = seed3;
    state->d = seed4;
    state->i = 1;
    arbee_mix(state);
}

static void efiix64_seed(efiix64_state_t *state, uint64_t seed[4]) {
    arbee_state_t seeder;
    uint64_t s1 = seed[0], s2 = seed[1], s3 = seed[2], s4 = seed[3];
    arbee_seed(&seeder, s1, s2, s3, s4);
    for (unsigned long w = 0; w < INDIRECTION_SIZE; w++) {
        state->indirection_table[w] = arbee_raw64(&seeder);
    }
    state->i = arbee_raw64(&seeder);
    for (unsigned long w = 0; w < ITERATION_SIZE; w++) {
        state->iteration_table[(w + state->i) % ITERATION_SIZE] = arbee_raw64(&seeder);
    }
    state->a = arbee_raw64(&seeder);
    state->b = arbee_raw64(&seeder);
    state->c = arbee_raw64(&seeder);
    for (unsigned long w = 0; w < 64; w++) {
        efiix64_raw64(state);
    }
    arbee_raw64(&seeder);
    s1 += arbee_raw64(&seeder);
    s2 += arbee_raw64(&seeder);
    s3 += arbee_raw64(&seeder);
    arbee_seed(&seeder, s1 ^ state->a, s2 ^ state->b, s3 ^ state->c, ~s4);
    for (unsigned long w = 0; w < INDIRECTION_SIZE; w++) {
        state->indirection_table[w] ^= arbee_raw64(&seeder);
        ;
    }
    for (unsigned long w = 0; w < ITERATION_SIZE + 16; w++) {
        efiix64_raw64(state);
    }
}

static void efiix64_set(void *vstate, unsigned long int seed)
{
  efiix64_state_t* state = (efiix64_state_t*) vstate;
  uint64_t seeds[4] = {0ul, 0ul, 0ul, 0ul};
  seeds[0] = seed;
  efiix64_seed (state, seeds);
}

static unsigned long int efiix64_get(void *vstate)
{
  efiix64_state_t* state = (efiix64_state_t*) vstate;
  return (unsigned long int)efiix64_next64 (state);
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double efiix64_get_double (void *vstate)
{
  efiix64_state_t* state = (efiix64_state_t*) vstate;
  return TO_DOUBLE(efiix64_next64 (state));
}

static const gsl_rng_type efiix64_type =
{"efiix64",                     /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (efiix64_state_t),
 &efiix64_set,
 &efiix64_get,
 &efiix64_get_double};

const gsl_rng_type *gsl_rng_efiix64 = &efiix64_type;
