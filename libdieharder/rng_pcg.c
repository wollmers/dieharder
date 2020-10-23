/*
 * rng_pcg.c
 * From https://www.pcg-random.org/download.html
 * and https://github.com/rkern/pcg64/
 *
 * Copyright (C) 2014 Melissa O'Neill <oneill@pcg-random.org>
 * Copyright (C) 2015 Robert Kern <robert.kern@gmail.com>
 * Copyright (C) 2020 Reini Urban, GSL/dieharder packaging
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>

#ifdef __SIZEOF_INT128__
#define HAVE_UINT128_T
#endif

/* *Really* minimal PCG32 code. Period 2^64 */

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

static unsigned long int pcg32_get(void *vstate)
{
  pcg32_random_t* rng = (pcg32_random_t*) vstate;
  uint64_t oldstate = rng->state;
  // Advance internal state
  rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
  // Calculate output function (XSH RR), uses old state for max ILP
  uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
  uint32_t rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

// state for global RNGs
#define PCG32_INITIALIZER   { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }
static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

static void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
  rng->state = 0U;
  rng->inc = (initseq << 1u) | 1u;
  (void)pcg32_get(rng);
  rng->state += initstate;
  (void)pcg32_get(rng);
}

static void pcg32_set(void *vstate, unsigned long int seed)
{
  pcg32_random_t* rng = (pcg32_random_t*) vstate;
  pcg32_srandom_r(&pcg32_global, seed, (intptr_t)&rng);
}

static double
pcg32_get_double (void *vstate)
{
  return pcg32_get(vstate) / (double) UINT32_MAX;
}

static const gsl_rng_type pcg32_type =
{"pcg32",                       /* name */
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (pcg32_random_t),
 &pcg32_set,
 &pcg32_get,
 &pcg32_get_double};

const gsl_rng_type *gsl_rng_pcg32 = &pcg32_type;

#if defined(HAVE_UINT128_T) && !defined(HAVE_32BITLONG)

/* pcg64: avoid emulated 128bit math for now. gcc/clang 64bit only */

typedef __uint128_t pcg128_t;
#define PCG_128BIT_CONSTANT(high,low)           \
            (((pcg128_t)(high) << 64) + low)
#define PCG_HIGH(a) (uint64_t)(a >> 64)
#define PCG_LOW(a) (uint64_t)(a)

typedef struct {
    pcg128_t state;
} pcg_state_128;

typedef struct {
    pcg128_t state;
    pcg128_t inc;
} pcg_state_setseq_128;

#define PCG_DEFAULT_MULTIPLIER_LOW 4865540595714422341ULL
#define PCG_DEFAULT_MULTIPLIER_128 \
  PCG_128BIT_CONSTANT(2549297995355413924ULL,PCG_DEFAULT_MULTIPLIER_LOW)
#define PCG_CHEAP_MULTIPLIER_LOW 0xda942042e4dd58b5ULL
#define PCG_CHEAP_MULTIPLIER_128                        \
  PCG_128BIT_CONSTANT(0ULL, PCG_CHEAP_MULTIPLIER_LOW)

static inline uint64_t pcg_rotr_64(uint64_t value, unsigned int rot)
{
    return (value >> rot) | (value << ((- rot) & 63));
}

static inline void pcg_setseq_128_step_r(pcg_state_setseq_128* rng)
{
    rng->state = rng->state * PCG_DEFAULT_MULTIPLIER_128 + rng->inc;
}

static inline uint64_t pcg_output_xsl_rr_128_64(pcg128_t state)
{
    return pcg_rotr_64(((uint64_t)(state >> 64u)) ^ (uint64_t)state,
                       state >> 122u);
}

static inline void pcg_setseq_128_srandom_r(pcg_state_setseq_128* rng,
                                            pcg128_t initstate, pcg128_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg_setseq_128_step_r(rng);
    rng->state += initstate;
    pcg_setseq_128_step_r(rng);
}

#define PCG_DEFAULT_INCREMENT_128                                       \
  PCG_128BIT_CONSTANT(6364136223846793005ULL,1442695040888963407ULL)
#define PCG_STATE_SETSEQ_128_INITIALIZER                                \
  { PCG_128BIT_CONSTANT(0x979c9a98d8462005ULL, 0x7d3e9cb6cfe0549bULL),  \
      PCG_128BIT_CONSTANT(0x0000000000000001ULL, 0xda3e39cb94b95bdbULL) }

// unused. i.e. the default pcg64_get
static inline uint64_t
pcg_setseq_128_xsl_rr_64_random_r(pcg_state_setseq_128* rng)
{
    pcg_setseq_128_step_r(rng);
    return pcg_output_xsl_rr_128_64(rng->state);
}

// unused
static inline uint64_t
pcg_setseq_128_xsl_rr_64_boundedrand_r(pcg_state_setseq_128* rng,
                                       uint64_t bound)
{
    uint64_t threshold = -bound % bound;
    for (;;) {
        uint64_t r = pcg_setseq_128_xsl_rr_64_random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}

// unused
static pcg128_t pcg_advance_lcg_128(pcg128_t state, pcg128_t delta, pcg128_t cur_mult,
                                    pcg128_t cur_plus)
{
   pcg128_t acc_mult = 1u;
   pcg128_t acc_plus = 0u;
   while (delta > 0) {
       if (delta & 1) {
	   acc_mult *= cur_mult;
	   acc_plus = acc_plus * cur_mult + cur_plus;
       }
       cur_plus = (cur_mult + 1) * cur_plus;
       cur_mult *= cur_mult;
       delta /= 2;
   }
   return acc_mult * state + acc_plus;
}

// unused
static inline void pcg_setseq_128_advance_r(pcg_state_setseq_128* rng, pcg128_t delta)
{
    rng->state = pcg_advance_lcg_128(rng->state, delta,
                                     PCG_DEFAULT_MULTIPLIER_128, rng->inc);
}

#define pcg64_random_r          pcg_setseq_128_xsl_rr_64_random_r
#define pcg64_boundedrand_r     pcg_setseq_128_xsl_rr_64_boundedrand_r
#define pcg64_advance_r         pcg_setseq_128_advance_r
#define PCG64_INITIALIZER       PCG_STATE_SETSEQ_128_INITIALIZER

typedef pcg_state_setseq_128    pcg64_random_t;
#define pcg64_srandom_r         pcg_setseq_128_srandom_r

/* pcg64_xslrr 1.0 with the original XSL-RR random rotation mixer.
   i.e. pcg_setseq_128_xsl_rr_64_random_r
*/
static unsigned long int pcg64_get(void *vstate)
{
  pcg64_random_t* rng = (pcg64_random_t*) vstate;
  pcg_setseq_128_step_r(rng);
  return (unsigned long int)pcg_output_xsl_rr_128_64(rng->state);
}

static void pcg64_set(void *vstate, unsigned long int seed)
{
  pcg64_random_t* rng = (pcg64_random_t*) vstate;
  // rng_test requires non-random initseq, so we cannot use the pointer
  pcg64_srandom_r(rng, seed, 57 /*(intptr_t)&rng*/);
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double
pcg64_get_double (void *vstate)
{
  return TO_DOUBLE(pcg64_get(vstate));
}

static const gsl_rng_type pcg64_type =
{"pcg64",                       /* name */
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (pcg64_random_t),
 &pcg64_set,
 &pcg64_get,
 &pcg64_get_double};

const gsl_rng_type *gsl_rng_pcg64 = &pcg64_type;

/* pcg64_cmdxsm cheap multiplier with a 64bit DXSM mixer.
   This is also called PCG64 variant 2.0.
 */
static inline uint64_t pcg_output_cmdxsm(uint64_t high, uint64_t low) {
  uint64_t hi = high;
  uint64_t lo = low;

  lo |= 1;
  hi ^= hi >> 32;
  hi *= PCG_CHEAP_MULTIPLIER_LOW;
  hi ^= hi >> 48;
  hi *= lo;
  return hi;
}
static inline uint64_t pcg_output_dxsm(uint64_t high, uint64_t low) {
  uint64_t hi = high;
  uint64_t lo = low;

  lo |= 1;
  hi ^= hi >> 32;
  hi *= PCG_DEFAULT_MULTIPLIER_LOW;
  hi ^= hi >> 48;
  hi *= lo;
  return hi;
}
static inline void pcg64_cm_step_r(pcg64_random_t *rng) {
  rng->state = (rng->state * PCG_CHEAP_MULTIPLIER_128) + rng->inc;
}
static inline void pcg64_step_r(pcg64_random_t *rng) {
  rng->state = (rng->state * PCG_DEFAULT_MULTIPLIER_128) + rng->inc;
}
static inline void pcg64_cmdxsm_srandom_r(pcg_state_setseq_128* rng,
                                          pcg128_t initstate, pcg128_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg64_cm_step_r(rng);
    rng->state += initstate;
    pcg64_cm_step_r(rng);
}

static void pcg64_cmdxsm_set(void *vstate, unsigned long int seed)
{
  pcg64_random_t* rng = (pcg64_random_t*) vstate;
  // rng_test requires non-random initseq, so we cannot use the pointer
  pcg64_cmdxsm_srandom_r(rng, seed, 57 /*(intptr_t)&rng*/);
}

static unsigned long int pcg64_cmdxsm_get(void *vstate)
{
  pcg64_random_t* rng = (pcg64_random_t*) vstate;
  pcg64_cm_step_r(rng);
  return (unsigned long int)pcg_output_cmdxsm(PCG_HIGH(rng->state), PCG_LOW(rng->state));
}

static double
pcg64_cmdxsm_get_double (void *vstate)
{
  return TO_DOUBLE(pcg64_cmdxsm_get(vstate));
}

static const gsl_rng_type pcg64_cmdxsm_type =
{"pcg64_cmdxsm",                  /* name */
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (pcg64_random_t),
 &pcg64_cmdxsm_set,
 &pcg64_cmdxsm_get,
 &pcg64_cmdxsm_get_double};
const gsl_rng_type *gsl_rng_pcg64_cmdxsm = &pcg64_cmdxsm_type;

/* pcg64_dxsm with a strong 128bit DXSM mixer.
 */
static inline void pcg64_dxsm_srandom_r(pcg_state_setseq_128* rng,
                                      pcg128_t initstate, pcg128_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg64_step_r(rng);
    rng->state += initstate;
    pcg64_step_r(rng);
}
static void pcg64_dxsm_set(void *vstate, unsigned long int seed)
{
  pcg64_random_t* rng = (pcg64_random_t*) vstate;
  // rng_test requires non-random initseq, so we cannot use the pointer
  pcg64_dxsm_srandom_r(rng, seed, 57 /*(intptr_t)&rng*/);
}

static unsigned long int pcg64_dxsm_get(void *vstate)
{
  pcg64_random_t* rng = (pcg64_random_t*) vstate;
  pcg64_step_r(rng);
  return (unsigned long int)pcg_output_dxsm(PCG_HIGH(rng->state), PCG_LOW(rng->state));
}

static double
pcg64_dxsm_get_double (void *vstate)
{
  return TO_DOUBLE(pcg64_dxsm_get(vstate));
}

static const gsl_rng_type pcg64_dxsm_type =
{"pcg64_dxsm",                  /* name */
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (pcg64_random_t),
 &pcg64_dxsm_set,
 &pcg64_dxsm_get,
 &pcg64_dxsm_get_double};
const gsl_rng_type *gsl_rng_pcg64_dxsm = &pcg64_dxsm_type;

#endif
