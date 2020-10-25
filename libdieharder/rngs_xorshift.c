/*
 * rngs_xorshift.c
 * Most based on http://www.jstatsoft.org/v08/i14/paper
 * George Marsaglia 2003
 * 
 * Copyright (c) 2018 Arvid Gerstmann for xorshift32_trunc.
 * xorshift32_trunc is licensed under MIT license.
 *
 * Copyright (c) 2020 Reini Urban. The rest and dieharder/gsl integration.
 * See COPYING
 */

#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>

typedef struct {
  uint64_t x;
} xorshift64_state_t;

static void xorshift32_set(void *vstate, unsigned long int seed)
{
  xorshift64_state_t* state = (xorshift64_state_t*) vstate;
  state->x = (uint64_t)seed | 1;
}

/* https://lemire.me/blog/2018/07/02/predicting-the-truncated-xorshift32-random-number-generator/
 * https://arvid.io/2018/07/02/better-cxx-prng/
 * Easily reversible via https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2018/07/02/cracktrunc.c
 */
static unsigned long int xorshift32_truncated(void *vstate)
{
  xorshift64_state_t* state = (xorshift64_state_t*) vstate;
  uint64_t result = state->x * 0xd989bcacc137dcd5ull;
  state->x ^= state->x >> 11;
  state->x ^= state->x << 31;
  state->x ^= state->x >> 18;
  return (unsigned long int)((uint32_t)(result >> 32ull));
}

static double xorshift32_truncated_double (void *vstate)
{
  xorshift64_state_t* state = (xorshift64_state_t*) vstate;
  uint32_t v = (uint32_t)xorshift32_truncated(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}

static const gsl_rng_type xorshift32_truncated_type =
{"xorshift32-trunc",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xorshift64_state_t),
 &xorshift32_set,
 &xorshift32_truncated,
 &xorshift32_truncated_double};

const gsl_rng_type *gsl_rng_xorshift32_truncated = &xorshift32_truncated_type;

/* Lemire's variant, Apache 2.0 LICENSE */

static unsigned long int xorshift32(void *vstate)
{
  xorshift64_state_t* state = (xorshift64_state_t*) vstate;
  state->x ^= (state->x << 13);
  state->x ^= (state->x >> 17);
  state->x ^= (state->x << 5);
  return (unsigned long int)(state->x & 0xffffffff);
}

static double xorshift32_double (void *vstate)
{
  xorshift64_state_t* state = (xorshift64_state_t*) vstate;
  uint32_t v = (uint32_t)xorshift32(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}
 
static const gsl_rng_type xorshift32_type =
{"xorshift32",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xorshift64_state_t),
 &xorshift32_set,
 &xorshift32,
 &xorshift32_double};

const gsl_rng_type *gsl_rng_xorshift32 = &xorshift32_type;

/* Marsaglia's original xorshift from 2003.
   Written from scratch by rurban. */
typedef struct {
  uint32_t x, y, z, w;
} xor128_state_t;

static void xor128_set(void *vstate, unsigned long int seed)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint64_t tmp = (uint64_t)seed;
  state->x = (uint32_t)tmp | 1;
  state->y = (uint32_t)splitmix64_next(&tmp);
  state->z = (uint32_t)splitmix64_next(&tmp);
  state->w = (uint32_t)splitmix64_next(&tmp);
}

static unsigned long int xor128(void *vstate)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint32_t tmp = state->x ^ (state->x << 15);
  state->x = state->y;
  state->y = state->z;
  state->z = state->w;
  state->w ^= state->w >> 21 ^ (tmp ^ (tmp >> 4));
  return (unsigned long int)state->w & 0xffffffff;
}

static double xor128_double (void *vstate)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint32_t v = (uint32_t)xor128(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}

static const gsl_rng_type xor128_type =
{"xor128",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xor128_state_t),
 &xor128_set,
 &xor128,
 &xor128_double};

const gsl_rng_type *gsl_rng_xor128 = &xor128_type;

/* Another variant based on Marsaglia's 2003 paper */
static unsigned long int xorshift_k4(void *vstate)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint32_t tmp = state->x ^= state->x << 11;
  state->x = state->y;
  state->y = state->z;
  state->z = state->w;
  state->w ^= state->w >> 19 ^ (tmp ^ (tmp >> 8));
  return (unsigned long int)(state->w & 0xffffffff);
}

static double xorshift_k4_double (void *vstate)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint32_t v = (uint32_t)xorshift_k4(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}

static const gsl_rng_type xorshift_k4_type =
{"xorshift-k4",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xor128_state_t),
 &xor128_set,
 &xorshift_k4,
 &xorshift_k4_double};

const gsl_rng_type *gsl_rng_xorshift_k4 = &xorshift_k4_type;

/* xorshift-k5
   George Marsaglia
   https://groups.google.com/d/msg/comp.lang.c/qZFQgKRCQGg/rmPkaRHqxOMJ
*/
typedef struct {
  uint32_t x, y, z, w, v;
} xor160_state_t;
 
static void xor160_set(void *vstate, unsigned long int seed)
{
  xor160_state_t* state = (xor160_state_t*) vstate;
  uint64_t tmp = (uint64_t)seed;
  state->x = (uint32_t)(tmp | 1);
  state->y = (uint32_t)splitmix64_next(&tmp);
  state->z = (uint32_t)splitmix64_next(&tmp);
  state->w = (uint32_t)splitmix64_next(&tmp);
  state->v = (uint32_t)splitmix64_next(&tmp);
}

static unsigned long int xorshift_k5(void *vstate)
{
  xor160_state_t* state = (xor160_state_t*) vstate;
  uint32_t tmp = state->x ^= state->x >> 7;
  state->x = state->y;
  state->y = state->z;
  state->z = state->w;
  state->w = state->v;
  state->v ^= (state->v << 6);
  state->v ^= (tmp ^ (tmp << 13));
  return (unsigned long int)((state->y + state->y + 1) * state->v);
}

static double xorshift_k5_double (void *vstate)
{
  xor160_state_t* state = (xor160_state_t*) vstate;
  uint32_t v = (uint32_t)xorshift_k5(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}

static const gsl_rng_type xorshift_k5_type =
{"xorshift-k5",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xor160_state_t),
 &xor160_set,
 &xorshift_k5,
 &xorshift_k5_double};

const gsl_rng_type *gsl_rng_xorshift_k5 = &xorshift_k5_type;

/* xorshift-add adds robustness through an addition step; it was proposed
   by Mutsuo Saito and Makoto Matsumoto (author of MT19937)
   (http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/XSADD/index.html),
   and it passes all BigCrush tests; although it fails some if bits
   are reversed.
   https://github.com/MersenneTwister-Lab/XSadd
 */

typedef struct {
  uint32_t x[4];
} xsadd_state_t;
 
static void xsadd_set(void *vstate, unsigned long int seed)
{
  xsadd_state_t* state = (xsadd_state_t*) vstate;
  uint64_t tmp = (uint64_t)seed;
  state->x[0] = (uint32_t)(tmp | 1);
  state->x[1] = (uint32_t)splitmix64_next(&tmp);
  state->x[2] = (uint32_t)splitmix64_next(&tmp);
  state->x[3] = (uint32_t)splitmix64_next(&tmp);
}

static unsigned long int xsadd(void *vstate)
{
  xsadd_state_t* state = (xsadd_state_t*) vstate;
  uint32_t tmp = state->x[0];
  tmp ^= tmp << 15;
  tmp ^= tmp >> 18;
  tmp ^= state->x[3] << 11;
  state->x[0] = state->x[1];
  state->x[1] = state->x[2];
  state->x[2] = state->x[3];
  state->x[3] = tmp;
  return (unsigned long int)(state->x[3] + state->x[2]);
}

static double xsadd_double (void *vstate)
{
  xsadd_state_t* state = (xsadd_state_t*) vstate;
  uint64_t a = (uint64_t)xsadd(state);
  uint64_t b = (uint64_t)xsadd(state);
  a = (a << 21) || (b >> 11);
  return a * (1.0/9007199254740992.0);
}

static const gsl_rng_type xsadd_type =
{"xorshift-add",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xsadd_state_t),
 &xsadd_set,
 &xsadd,
 &xsadd_double};

const gsl_rng_type *gsl_rng_xsadd = &xsadd_type;

/* xorshift7 takes a different approach: it adds robustness by
   allowing more shifts than Marsaglia's original three. It is a
   7-shift generator with 256 bits, that passes BigCrush with no
   systmatic failures.
   François Panneton and Pierre L'ecuyer: 
   "On the Xorgshift Random Number Generators"
   http://saluc.engr.uconn.edu/refs/crypto/rng/panneton05onthexorshift.pdf
 */

typedef struct {
  uint32_t x[8];
  uint32_t k;
} xorshift7_state_t;
 
static void xorshift7_set(void *vstate, unsigned long int seed)
{
  xorshift7_state_t* state = (xorshift7_state_t*) vstate;
  uint64_t tmp = (uint64_t)seed;
  for (unsigned i=0; i<8; i++)
    state->x[i] = (uint32_t)splitmix64_next(&tmp);
  state->k = 0;
}

static unsigned long int xorshift7(void *vstate)
{
  xorshift7_state_t* state = (xorshift7_state_t*) vstate;
  uint32_t y, t;
  t = state->x[(state->k + 7) & 7];
  t = t ^ (t << 13);
  y = t ^ (t << 9);
  t = state->x[(state->k + 4) & 7];
  y ^= t ^ (t << 7);
  t = state->x[(state->k + 3) & 7];
  y ^= t ^ (t >> 3);
  t = state->x[(state->k + 1) & 7];
  y ^= t ^ (t >> 10);
  t = state->x[state->k];
  t = t ^ (t >> 7);
  y ^= t ^ (t << 24);
  state->x[state->k] = y;
  state->k = (state->k + 1) & 7;
  return (unsigned long int)y;
}

static double xorshift7_double (void *vstate)
{
  xorshift7_state_t* state = (xorshift7_state_t*) vstate;
  uint64_t a = (uint64_t)xorshift7(state);
  uint64_t b = (uint64_t)xorshift7(state);
  a = (a << 21) || (b >> 11);
  return a * (1.0/9007199254740992.0);
}

static const gsl_rng_type xorshift7_type =
{"xorshift7",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xorshift7_state_t),
 &xorshift7_set,
 &xorshift7,
 &xorshift7_double};

const gsl_rng_type *gsl_rng_xorshift7 = &xorshift7_type;

/* xor4096, by Richard Brent, is a 4096-bit xor-shift with a very long
   period that also adds a Weyl generator. It also passes BigCrush
   with no systematic failures. Its long period may be useful if you
   have many generators and need to avoid collisions.
   Completely rewritten, based on R. P. Brent's code from 20040802.
*/

typedef struct {
  uint32_t x[128];
  uint32_t w;
  int32_t i;
} xor4096_state_t;

#define WEIL 0x61c88647

static unsigned long xor4096 (void *vstate)
{
  xor4096_state_t* state = (xor4096_state_t*) vstate;
  unsigned int t, v;
  t = state->x[state->i = (state->i + 1) & 127];
  v = state->x[(state->i + (128 - 95)) & 127];
  t ^= t << 17;
  t ^= t >> 12;
  v ^= v << 13;
  v ^= t ^ (v >> 15);
  state->x[state->i] = v;
  return (v + (state->w += WEIL));      /* Return combination with Weil generator */
}

static void xor4096_seed (void *vstate, unsigned long seed)
{
  xor4096_state_t* state = (xor4096_state_t*) vstate;
  unsigned int t, v;
  int k;
  v = seed | 1;

  for (k = 32; k > 0; k--) {  /* Avoid correlations for close seeds, period 2^32-1 */
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
  }
  for (state->w = v, k = 0; k < 128; k++) {
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    state->x[k] = v + (state->w += WEIL);
  }

  for (state->i = 127, k = 4 * 128; k > 0; k--) /* Discard first 4*r results (Gimeno) */
    (void)xor4096(vstate);
}

#undef WEIL

static double xor4096_double (void *vstate)
{
  xor4096_state_t* state = (xor4096_state_t*) vstate;
#if 0 // more precise but twice as slow
  uint64_t a = (uint64_t)xor4096(state);
  uint64_t b = (uint64_t)xor4096(state);
  a = (a << 21) || (b >> 11);
  return a * (1.0/9007199254740992.0);
#else
  uint32_t v = (uint32_t)xor4096(state);
  return (v >> 11) * (1.0/9007199254740992.0);
#endif
}

static const gsl_rng_type xor4096_type =
{"xor4096",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xor4096_state_t),
 &xor4096_seed,
 &xor4096,
 &xor4096_double};

const gsl_rng_type *gsl_rng_xor4096 = &xor4096_type;

/* xorshift128plus "Further scramblings of Marsaglia’s
   xorshift generators" by Vigna. The improved version. */

typedef struct {
  uint64_t s0;
  uint64_t s1;
} xorshift128_state_t;
 
static void xorshift128_set(void *vstate, unsigned long int seed)
{
  xorshift128_state_t* state = (xorshift128_state_t*) vstate;  
  uint64_t tmp = (uint64_t)seed;
  state->s0 = splitmix64_next(&tmp);
  state->s1 = splitmix64_next(&tmp);
}

/* TODO https://github.com/lemire/SIMDxorshift
   Return a 256-bit random number with AVX2

typedef struct {
  __ms256i s0;
  __ms256i s1;
} avx2_xorshift128_state_t;

__m256i avx2_xorshift128plus(void *vstate) {
  avx2_xorshift128_state_t* state = (avx2_xorshift128_state_t*) vstate;
  __m256i s1 = key->s0;
  const __m256i s0 = key->s1;
  key->s0 = key->s1;
  s1 = _mm256_xor_si256(key->s1, _mm256_slli_epi64(key->s1, 23));
  key->s1 = _mm256_xor_si256(
    _mm256_xor_si256(_mm256_xor_si256(s1, s0),
    _mm256_srli_epi64(s1, 18)), _mm256_srli_epi64(s0, 5));
  return _mm256_add_epi64(key->part2, s0);
}
*/

static unsigned long int xorshift128plus(void *vstate)
{
  xorshift128_state_t* state = (xorshift128_state_t*) vstate;
  uint64_t s1 = state->s0;
  const uint64_t s0 = state->s1;
  const uint64_t result = s0 + s1;
  state->s0 = s0;
  s1 ^= s1 << 23;
  state->s1 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
  return (unsigned long int)result;
}

static double xorshift128plus_double (void *vstate)
{
  xorshift128_state_t* state = (xorshift128_state_t*) vstate;
  uint64_t a = (uint64_t)xorshift128plus(state);
#ifndef HAVE_32BITLONG
  return (a >> 11) * 0x1.0p-53;
#else
  return (a >> 11) * (1.0/9007199254740992.0);
#endif
}

static const gsl_rng_type xorshift128plus_type =
{"xorshift128+",
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xorshift128_state_t),
 &xorshift128_set,
 &xorshift128plus,
 &xorshift128plus_double};
const gsl_rng_type *gsl_rng_xorshift128plus = &xorshift128plus_type;

/* xorshift1024plus */
/* By Sebastiano Vigna (vigna@acm.org) 2014 */

typedef struct {
  uint64_t s[16];
  int p;
} xorshift1024_state_t;
 
static void xorshift1024_set(void *vstate, unsigned long int seed)
{
  xorshift1024_state_t* state = (xorshift1024_state_t*) vstate;  
  uint64_t tmp = (uint64_t)seed;
  for (unsigned i=0; i<16; i++)
    state->s[i] = splitmix64_next(&tmp);
  state->p = 0;
}

static unsigned long int xorshift1024plus(void *vstate)
{
  xorshift1024_state_t* state = (xorshift1024_state_t*) vstate;
  const uint64_t s0 = state->s[state->p];
  uint64_t s1 = state->s[state->p = (state->p + 1) & 15];
  const uint64_t result = s0 + s1;
  s1 ^= s1 << 31; // a
  state->s[state->p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30); // b, c
  return (unsigned long int)result;
}

static double xorshift1024plus_double (void *vstate)
{
  xorshift1024_state_t* state = (xorshift1024_state_t*) vstate;
  uint64_t a = (uint64_t)xorshift1024plus(state);
#ifndef HAVE_32BITLONG
  return (a >> 11) * 0x1.0p-53;
#else
  return (a >> 11) * (1.0/9007199254740992.0);
#endif
}

static const gsl_rng_type xorshift1024plus_type =
{"xorshift1024+",
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xorshift1024_state_t),
 &xorshift1024_set,
 &xorshift1024plus,
 &xorshift1024plus_double};

const gsl_rng_type *gsl_rng_xorshift1024plus = &xorshift1024plus_type;

/* xorshift128star */
/* By Sebastiano Vigna (vigna@acm.org) 2014 */
/* This is a fast, top-quality generator. If 1024 bits of state are too
   much, try a xoroshiro128+ generator.

   Note that the three lowest bits of this generator are LSFRs, and thus
   they are slightly less random than the other bits. We suggest to use a
   sign test to extract a random Boolean value.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

static unsigned long int xorshift1024star(void *vstate)
{
  xorshift1024_state_t* state = (xorshift1024_state_t*) vstate;
  const uint64_t s0 = state->s[state->p];
  uint64_t s1 = state->s[(state->p = (state->p + 1) % 16)];
  s1 ^= s1 << 31; // a
  state->s[state->p] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30); // b, c
  return (unsigned long int)state->s[state->p] * UINT64_C(1181783497276652981);
}

static double xorshift1024star_double (void *vstate)
{
  xorshift1024_state_t* state = (xorshift1024_state_t*) vstate;
  uint64_t a = (uint64_t)xorshift1024star(state);
#ifndef HAVE_32BITLONG
  return (a >> 11) * 0x1.0p-53;
#else
  return (a >> 11) * (1.0/9007199254740992.0);
#endif
}

static const gsl_rng_type xorshift1024star_type =
{"xorshift1024*",
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xorshift1024_state_t),
 &xorshift1024_set,
 &xorshift1024star,
 &xorshift1024star_double};

const gsl_rng_type *gsl_rng_xorshift1024star = &xorshift1024star_type;
