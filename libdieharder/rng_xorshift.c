/*
 * rng_xorshift.c
 * All based on http://www.jstatsoft.org/v08/i14/paper
 * George Marsaglia 2003
 * 
 * Copyright (c) 2018 Arvid Gerstmann for xorshift32_truncated.
 * xorshift32_truncated is licensed under MIT license.
 *
 * Copyright (c) 2020 Reini Urban. xor128 and dieharder/gsl integration.
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
  return (unsigned long int)(result >> 32ull);
}

static double xorshift32_truncated_double (void *vstate)
{
  xorshift64_state_t* state = (xorshift64_state_t*) vstate;
  uint32_t v = (uint32_t)xorshift32_truncated(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}

static const gsl_rng_type xorshift32_truncated_type =
{"xorshift32-truncated",
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

/* Marsaglia's original xorshift. Written from scratch by rurban. */
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
  uint32_t tmp = state->x ^= state->x << 15;
  state->x = state->y;
  state->y = state->z;
  state->z = state->w;
  state->w ^= state->w >> 21;
  return (unsigned long int)(state->w ^ (tmp ^ (tmp >> 4))) & 0xffffffff;
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

// another variant based on this 2003 paper
static unsigned long int xorshift_k4(void *vstate)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint32_t tmp = state->x ^= state->x << 11;
  state->x = state->y;
  state->y = state->z;
  state->z = state->w;
  state->w ^= state->w >> 19;
  return (unsigned long int)(state->w ^ (tmp ^ (tmp >> 8))) & 0xffffffff;
}

static double xorshift_k4_double (void *vstate)
{
  xor128_state_t* state = (xor128_state_t*) vstate;
  uint32_t v = (uint32_t)xorshift_k4(state);
  return (v >> 11) * (1.0/9007199254740992.0);
}

static const gsl_rng_type xorshift_k4_type =
{"xorshift_k4",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xor128_state_t),
 &xor128_set,
 &xorshift_k4,
 &xorshift_k4_double};

const gsl_rng_type *gsl_rng_xorshift_k4 = &xorshift_k4_type;

/* xorshift_k5
   George Marsaglia,
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
{"xorshift_k5",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xor160_state_t),
 &xor160_set,
 &xorshift_k5,
 &xorshift_k5_double};

const gsl_rng_type *gsl_rng_xorshift_k5 = &xorshift_k5_type;

// xsadd adds robustness through an addition step; it was proposed by Mutsuo Saito and Makoto Matsumoto (author of MT19937) (http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/XSADD/index.html), and it passes all BigCrush tests; although it fails some if bits are reversed.

                                                                                                     // xorshift7*, by François Panneton and Pierre L'ecuyer, takes a different approach: it adds robustness by allowing more shifts than Marsaglia's original three. It is a 7-shift generator with 256 bits, that passes BigCrush with no systmatic failures.

                                                                                                    // xor4096, by Richard Brent, is a 4096-bit xor-shift with a very long period that also adds a Weyl generator. It also passes BigCrush with no systematic failures. Its long period may be useful if you have many generators and need to avoid collisions.

// xorshift128plus
// Sebastiano Vigna recently proposed another algorithm based on the xsadd idea, "xorshift128+", which fixes weaknesses in the lower bits; unfortunatly, that improved algorithm requires 64-bit arithmetic and is not fast in Javascript.

// xorshift128star
 
