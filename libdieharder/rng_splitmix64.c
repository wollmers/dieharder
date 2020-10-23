/*
 * rng_splitmix64.c
 * Copyright (c) 2015 by Sebastiano Vigna (vigna@acm.org)
 * Copyright (c) 2020 Reini Urban. dieharder/gsl integration.
 *
 * From http://dx.doi.org/10.1145/2714064.2660195
 * Fast Splittable Pseudorandom Number Generators
 * Steele Jr, Guy L., Doug Lea, and Christine H. Flood. "Fast splittable
 * pseudorandom number generators."
 * ACM SIGPLAN Notices 49.10 (2014): 453-472
 *
 * To the extent possible under law, the authors have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Also available generally in our header for many others.
 */

#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>

typedef struct splitmix_state {
  uint64_t x;
} splitmix_state_t;

static void splitmix_set(void *vstate, unsigned long int seed)
{
  splitmix_state_t* state = (splitmix_state_t*) vstate;
  state->x = (uint64_t)seed;
}

static unsigned long int splitmix_get(void *vstate)
{
  splitmix_state_t* state = (splitmix_state_t*) vstate;
  return (unsigned long int)splitmix64_next (&state->x);
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double splitmix_get_double (void *vstate)
{
  splitmix_state_t* state = (splitmix_state_t*) vstate;
  return TO_DOUBLE(splitmix64_next (&state->x));
}

static const gsl_rng_type splitmix64_type =
{"splitmix64",
 RNG64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (splitmix_state_t),
 &splitmix_set,
 &splitmix_get,
 &splitmix_get_double};

const gsl_rng_type *gsl_rng_splitmix64 = &splitmix64_type;
