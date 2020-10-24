/*
 *  rngs_lemire.c
 * 
 *  Three missing and old RNGs from lemire's testingRNG repo
 *  From https://github.com/lemire/testingRNG/
 *  Copyright (C) 2020 Reini Urban, completely rewritten
 *
 */

#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>

/**
* D. H. Lehmer, Mathematical methods in large-scale computing units.
* Proceedings of a Second Symposium on Large Scale Digital Calculating
* Machinery;
* Annals of the Computation Laboratory, Harvard Univ. 26 (1951), pp. 141-146.
*
* P L'Ecuyer,  Tables of linear congruential generators of different sizes and
* good lattice structure. Mathematics of Computation of the American
* Mathematical Society 68.225 (1999): 249-260.
*/

#if !defined(HAVE_32BITLONG) && defined(__SIZEOF_INT128__)

typedef struct {
 __uint128_t state;
} lehmer64_state_t;

static void lehmer64_seed(void *vstate, unsigned long int s) {
  lehmer64_state_t *state = (lehmer64_state_t *) vstate;
  uint64_t seed = (uint64_t)s;
  state->state = (((__uint128_t)splitmix64_next(&seed)) << 64);
  // hmm, could be made much easier, 2x splitmix64_next(&seed).
  seed = (uint64_t)(s + 1);
  state->state += splitmix64_next(&seed);
}

static unsigned long int lehmer64_get(void *vstate) {
  lehmer64_state_t *state = (lehmer64_state_t *) vstate;
  state->state *= UINT64_C(0xda942042e4dd58b5);
  return (unsigned long int)(state->state >> 64);
}

#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double lehmer64_get_double (void *vstate)
{
  lehmer64_state_t *state = (lehmer64_state_t *) vstate;
  state->state *= UINT64_C(0xda942042e4dd58b5);
  return TO_DOUBLE(state->state >> 64);
}

static const gsl_rng_type lehmer64_type =
{"lehmer64",			/* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (lehmer64_state_t),
 &lehmer64_seed,
 &lehmer64_get,
 &lehmer64_get_double};

const gsl_rng_type *gsl_rng_lehmer64 = &lehmer64_type;

#endif

/* Mitchell-Moore algorithm from
 * "The Art of Computer Programming, Volume II"
 * by Donald E. Knuth
 *
 * Improvements based on
 * "Uniform Random Number Generators for Vector and Parallel Computers"
 * by Richard P. Brent */

#define R 250U
#define S 200U
#define T 103U
#define U 50U

typedef struct {
  uint32_t seq[R];
  unsigned int a, b, c, d;
} mitchellmoore_state_t;

static void mitchellmoore_set(void *vstate, unsigned long int seed) {
  mitchellmoore_state_t *state = (mitchellmoore_state_t *) vstate;
  state->a = R; state->b = S; state->c = T; state->d = U;

  for (unsigned i = 0; i < R * 2; i++)
    state->seq[i % R] = seed = (1664525 * seed + 1013904223);

  state->seq[0] <<= 1;
  state->seq[1] |= 1;
  return;
}

static unsigned long int mitchellmoore_get(void *vstate) {
  mitchellmoore_state_t *state = (mitchellmoore_state_t *) vstate;
  return state->seq[++state->a % R] +=
         state->seq[++state->b % R] +=
         state->seq[++state->c % R] +=
         state->seq[++state->d % R];
}

static double mitchellmoore_get_double (void *vstate)
{
  return (double) mitchellmoore_get (vstate) / (double) UINT_MAX;
}

#undef R
#undef S
#undef T
#undef U

static const gsl_rng_type mitchellmoore_type =
{"mitchelmoore",			/* name */
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (mitchellmoore_state_t),
 &mitchellmoore_set,
 &mitchellmoore_get,
 &mitchellmoore_get_double};
const gsl_rng_type *gsl_rng_mitchellmoore = &mitchellmoore_type;


/* Based on https://arxiv.org/pdf/1704.00358.pdf
 */
typedef struct {
#ifdef HAVE_32BITLONG
  uint32_t x, w, s;
#else
  uint64_t x, w, s;
#endif
} widynski_state_t;

static void widynski_set(void *vstate, unsigned long int seed) {
  widynski_state_t *state = (widynski_state_t *) vstate;
  state->w = 0;
  state->x = 0;
  state->s = seed;
  state->s |= 1;
  if ((state->s >> 32) == 0)
    state->s = state->s | (state->s << 32);
  return;
}

static unsigned long int widynski_get(void *vstate) {
  widynski_state_t *state = (widynski_state_t *) vstate;
  state->x *= state->x;
  state->x += (state->w += state->s);
  state->x = (state->x >> 32) | (state->x << 32);
#ifdef HAVE_32BITLONG
  return (unsigned long int)(state->x & 0xffffffff);
#else
  return (unsigned long int)state->x;
#endif
}

static double widynski_get_double (void *vstate)
{
  widynski_state_t *state = (widynski_state_t *) vstate;
  state->x *= state->x;
  state->x += (state->w += state->s);
  state->x = (state->x >> 32) | (state->x << 32);
#ifdef HAVE_32BITLONG
  return (((uint32_t)state->x) >> 11) * (1.0/9007199254740992.0);
#else
  return TO_DOUBLE(state->x);
#endif
}

static const gsl_rng_type widynski_type =
{"widynski",			/* name */
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (widynski_state_t),
 &widynski_set,
 &widynski_get,
 &widynski_get_double};
const gsl_rng_type *gsl_rng_widynski = &widynski_type;
