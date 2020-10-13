/*
 *  rng_xoroshiro64bit.c
 *  From http://prng.di.unimi.it/
 *
 *  Copyright(c) 2020 Reini Urban <rurban@cpan.org>
 */

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#include <dieharder/libdieharder.h>

static inline uint64_t rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (32 - k));
}

/*
 * These are the wrappers for the xoroshiro128 64bit prngs
 */
#define RNG_WRAP(n) \
  static unsigned long int n##_get (void *vstate);       \
  static double n##_get_double (void *vstate)

// 64bit
RNG_WRAP (xoroshiro128_pp);
RNG_WRAP (xoroshiro128_ss);
RNG_WRAP (xoroshiro128_p);
static void xoroshiro64_set (void *vstate, unsigned long int s);
//RNG_WRAP (xoshiro256_pp)
//RNG_WRAP (xoshiro256_ss)
//RNG_WRAP (xoshiro256_p)

typedef struct
  {
    uint64_t s[4];
  }
xoshiro64_state_t;

/* This is xoshiro128++ 1.0, one of our 32-bit all-purpose, rock-solid
   generators. It has excellent speed, a state size (128 bits) that is
   large enough for mild parallelism, and it passes all tests we are aware
   of.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */
static inline unsigned long int
xoroshiro128_pp_get (void *vstate)
{
  #define s state->s
  xoshiro64_state_t *state = (xoshiro64_state_t*)vstate;
  const uint64_t result = rotl(s[0] + s[3], 7) + s[0];
  const uint64_t t = s[1] << 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 11);
  #undef s
  return (unsigned long int)result;
}

/* This is xoshiro128** 1.1, one of our 32-bit all-purpose, rock-solid
   generators. It has excellent speed, a state size (128 bits) that is
   large enough for mild parallelism, and it passes all tests we are aware
   of.

   Note that version 1.0 had mistakenly s[0] instead of s[1] as state
   word passed to the scrambler.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */
static inline unsigned long int
xoroshiro128_ss_get (void *vstate)
{
  #define s state->s
  xoshiro64_state_t *state = (xoshiro64_state_t*)vstate;
  const uint64_t result = rotl(s[1] * 5, 7) * 9;
  const uint64_t t = s[1] << 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 11);
  #undef s
  return (unsigned long int)result;
}

/* This is xoshiro128+ 1.0, our best and fastest 32-bit generator for 32-bit
   floating-point numbers. We suggest to use its upper bits for
   floating-point generation, as it is slightly faster than xoshiro128**.
   It passes all tests we are aware of except for
   linearity tests, as the lowest four bits have low linear complexity, so
   if low linear complexity is not considered an issue (as it is usually
   the case) it can be used to generate 32-bit outputs, too.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   The state must be seeded so that it is not everywhere zero. */
static inline unsigned long int
xoroshiro128_p_get (void *vstate)
{
  #define s state->s
  xoshiro64_state_t *state = (xoshiro64_state_t*)vstate;
  const uint64_t result = s[0] + s[3];
  const uint64_t t = s[1] << 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 11);
  #undef s
  return (unsigned long int)result;
}

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
static inline unsigned long int
xoshiro256_pp_get (void *vstate)
{
  #define s state->s
  xoshiro64_state_t *state = (xoshiro64_state_t*)vstate;
  const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 45);
  #undef s
  return (unsigned long int)result;
}
/* This is xoshiro256** 1.0, one of our all-purpose, rock-solid
   generators. It has excellent (sub-ns) speed, a state (256 bits) that is
   large enough for any parallel application, and it passes all tests we
   are aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
static inline unsigned long int
xoshiro256_ss_get (void *vstate)
{
  #define s state->s
  xoshiro64_state_t *state = (xoshiro64_state_t*)vstate;
  const uint64_t result = rotl(s[1] * 5, 7) * 9;

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 45);
  #undef s
  return (unsigned long int)result;
}

/* This is xoshiro256+ 1.0, our best and fastest generator for floating-point
   numbers. We suggest to use its upper bits for floating-point
   generation, as it is slightly faster than xoshiro256++/xoshiro256**. It
   passes all tests we are aware of except for the lowest three bits,
   which might fail linearity tests (and just those), so if low linear
   complexity is not considered an issue (as it is usually the case) it
   can be used to generate 64-bit outputs, too.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
static inline unsigned long int
xoshiro256_p_get (void *vstate)
{
  #define s state->s
  xoshiro64_state_t *state = (xoshiro64_state_t*)vstate;
  const uint64_t result = s[0] + s[3];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 45);  
  #undef s
  return (unsigned long int)result;
}

static double
xoroshiro128_pp_get_double (void *vstate)
{
  return xoroshiro128_pp_get(vstate) / (double) UINT64_MAX;
}
static double
xoroshiro128_ss_get_double (void *vstate)
{
  return xoroshiro128_ss_get(vstate) / (double) UINT64_MAX;
}
static double
xoroshiro128_p_get_double (void *vstate)
{
  return xoroshiro128_p_get(vstate) / (double) UINT64_MAX;
}
static double
xoshiro256_pp_get_double (void *vstate)
{
  return xoshiro256_pp_get(vstate) / (double) UINT64_MAX;
}
static double
xoshiro256_ss_get_double (void *vstate)
{
  return xoshiro256_ss_get(vstate) / (double) UINT64_MAX;
}
static double
xoshiro256_p_get_double (void *vstate)
{
  return xoshiro256_p_get(vstate) / (double) UINT64_MAX;
}

static void xoroshiro64_set (void *vstate, unsigned long int s) {

 /* Initialize automaton using specified seed. */
 xoshiro64_state_t *state __attribute__((unused)) = (xoshiro64_state_t *) vstate;
 state->s[0] = random_seed() ^ s;
 state->s[1] = random_seed() ^ rotl(s, 4);
 state->s[2] = random_seed() ^ rotl(s, 8);
 state->s[3] = random_seed() ^ rotl(s, 12);
}

static const gsl_rng_type xoroshiro128_pp_type =
{"xoroshiro128++",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xoshiro64_state_t),
 &xoroshiro64_set,
 &xoroshiro128_pp_get,
 &xoroshiro128_pp_get_double};

static const gsl_rng_type xoroshiro128_ss_type =
{"xoroshiro128**",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xoshiro64_state_t),
 &xoroshiro64_set,
 &xoroshiro128_ss_get,
 &xoroshiro128_ss_get_double};

static const gsl_rng_type xoroshiro128_p_type =
{"xoroshiro128+",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xoshiro64_state_t),
 &xoroshiro64_set,
 &xoroshiro128_p_get,
 &xoroshiro128_p_get_double};

static const gsl_rng_type xoshiro256_pp_type =
{"xoshiro256++",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xoshiro64_state_t),
 &xoroshiro64_set,
 &xoshiro256_pp_get,
 &xoshiro256_pp_get_double};

static const gsl_rng_type xoshiro256_ss_type =
{"xoshiro256**",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xoshiro64_state_t),
 &xoroshiro64_set,
 &xoshiro256_ss_get,
 &xoshiro256_ss_get_double};

static const gsl_rng_type xoshiro256_p_type =
{"xoshiro256+",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (xoshiro64_state_t),
 &xoroshiro64_set,
 &xoshiro256_p_get,
 &xoshiro256_p_get_double};

const gsl_rng_type *gsl_rng_xoroshiro128_pp = &xoroshiro128_pp_type;
const gsl_rng_type *gsl_rng_xoroshiro128_ss = &xoroshiro128_ss_type;
const gsl_rng_type *gsl_rng_xoroshiro128_p  = &xoroshiro128_p_type;
const gsl_rng_type *gsl_rng_xoshiro256_pp = &xoshiro256_pp_type;
const gsl_rng_type *gsl_rng_xoshiro256_ss = &xoshiro256_ss_type;
const gsl_rng_type *gsl_rng_xoshiro256_p  = &xoshiro256_p_type;
