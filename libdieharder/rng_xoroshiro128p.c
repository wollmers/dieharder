/*
 * XOROSHIRO128+ from:
 * David Blackman and Sebastiano Vigna. Scrambled linear pseudorandom number
 * generators, 2019.
 * http://vigna.di.unimi.it/ftp/papers/ScrambledLinear.pdf
 * This uniform 64 bit generator has but 128 bits of state, and is optimized for
 * generation of 32 bit floating point numbers.
 *
 * In my (Marcus Müller) experience, it also lends itself to generation of
 * normal numerical fixed point noise through application of the central limit
 * theorem.
 */

#include <dieharder/libdieharder.h>

/*
 * We keep the actual PRNG code in a separate header file on purpose: That way,
 * a user can really trivially use a *verified* PRNG instead of just one where
 * they think the code is equivalent.
 */
#include <dieharder/xoroshiro128p.h>

/*
 * This is just a thin wrapper for the XOROSHIRO128+ PRNG
 * For more information, see xoroshiro128p.h
 */
static unsigned long int xoroshiro128p_get(void *vstate);
static double xoroshiro128p_get_double(void *vstate);
static void xoroshiro128p_set(void *vstate, unsigned long int s);

typedef struct {
  uint64_t state[2];
} xoroshiro128p_state_t;

static unsigned long int xoroshiro128p_get(void *vstate) {
  xoroshiro128p_state_t *statep = (xoroshiro128p_state_t *)vstate;
  uint64_t *state = statep->state;
  return xoroshiro128p_next(state);
}

static double xoroshiro128p_get_double(void *vstate) {
  return xoroshiro128p_get(vstate) / (double)UINT64_MAX;
}

static void xoroshiro128p_set(void *vstate, unsigned long int s) {
  xoroshiro128p_state_t *statep = (xoroshiro128p_state_t *)vstate;
  uint64_t *state = statep->state;
  xoroshiro128p_seed(state, s);
  return;
}

static const gsl_rng_type xoroshiro128p_type = {"XOROSHIRO128+", /* name */
                                                UINT64_MAX,      /* RAND_MAX */
                                                0,               /* RAND_MIN */
                                                sizeof(xoroshiro128p_state_t),
                                                &xoroshiro128p_set,
                                                &xoroshiro128p_get,
                                                &xoroshiro128p_get_double};

const gsl_rng_type *gsl_rng_xoroshiro128p = &xoroshiro128p_type;
