/**
 * @file  SFMT.c
 * @brief SIMD oriented Fast Mersenne Twister(SFMT)
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima University.
 * Copyright (C) 2012 Mutsuo Saito, Makoto Matsumoto, Hiroshima University and
 *                   The University of Tokyo.
 * Copyright (C) 2013 Mutsuo Saito, Makoto Matsumoto and Hiroshima University.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE-smft.txt
 *
 * uint32_t must be exactly 32-bit unsigned integer type (no more, no
 * less), and uint64_t must be exactly 64-bit unsigned integer type.
 * PRIu64 and PRIx64 are used for printf function to print 64-bit
 * unsigned int and 64-bit unsigned int in hexadecimal format.
 */

#if defined(__cplusplus)
extern "C" {
#endif

//#include "../common/randomgen_endian.h"
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>
#include "cpu_features.h"

#ifdef IS_BIG_ENDIAN
  #define BIG_ENDIAN64
  #if SIZEOF_SIZE_T == 8
    #define ONLY64
  #endif
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  #include <inttypes.h>
#elif defined(_MSC_VER) || defined(__BORLANDC__)
  typedef unsigned int uint32_t;
  typedef unsigned __int64 uint64_t;
  #ifndef inline
    #define inline __forceinline
  #endif
#else
  #include <inttypes.h>
  #if defined(__GNUC__)
    #define inline __inline__
  #endif
#endif

#ifndef PRIu64
  #if defined(_MSC_VER) || defined(__BORLANDC__)
    #define PRIu64 "I64u"
    #define PRIx64 "I64x"
  #else
    #define PRIu64 "llu"
    #define PRIx64 "llx"
  #endif
#endif

//#if !(RANDOMGEN_LITTLE_ENDIAN)
//#define BIG_ENDIAN64 1
//#endif

//#include "sfmt-params.h"
/*-----------------
  BASIC DEFINITIONS
  -----------------*/
/** Mersenne Exponent. The period of the sequence
 *  is a multiple of 2^MEXP-1.
 */
#define SFMT_MEXP 19937  
/** SFMT generator has an internal state array of 128-bit integers,
 * and N is its size. */
#define SFMT_N (SFMT_MEXP / 128 + 1)
/** N32 is the size of internal state array when regarded as an array
 * of 32-bit integers.*/
#define SFMT_N32 (SFMT_N * 4)
/** N64 is the size of internal state array when regarded as an array
 * of 64-bit integers.*/
#define SFMT_N64 (SFMT_N * 2)

/*----------------------
  the parameters of SFMT
  following definitions are in paramsXXXX.h file.
  ----------------------*/
/** the pick up position of the array.
#define SFMT_POS1 122
*/

/** the parameter of shift left as four 32-bit registers.
#define SFMT_SL1 18
 */

/** the parameter of shift left as one 128-bit register.
 * The 128-bit integer is shifted by (SFMT_SL2 * 8) bits.
#define SFMT_SL2 1
*/

/** the parameter of shift right as four 32-bit registers.
#define SFMT_SR1 11
*/

/** the parameter of shift right as one 128-bit register.
 * The 128-bit integer is shifted by (SFMT_SR2 * 8) bits.
#define SFMT_SR2 1
*/

/** A bitmask, used in the recursion.  These parameters are introduced
 * to break symmetry of SIMD.
#define SFMT_MSK1 0xdfffffefU
#define SFMT_MSK2 0xddfecb7fU
#define SFMT_MSK3 0xbffaffffU
#define SFMT_MSK4 0xbffffff6U
*/

/** These definitions are part of a 128-bit period certification vector.
#define SFMT_PARITY1    0x00000001U
#define SFMT_PARITY2    0x00000000U
#define SFMT_PARITY3    0x00000000U
#define SFMT_PARITY4    0xc98e126aU
*/

//#include "sfmt-params19937.h"
#define SFMT_POS1 122
#define SFMT_SL1 18
#define SFMT_SL2 1
#define SFMT_SR1 11
#define SFMT_SR2 1
#define SFMT_MSK1 0xdfffffefU
#define SFMT_MSK2 0xddfecb7fU
#define SFMT_MSK3 0xbffaffffU
#define SFMT_MSK4 0xbffffff6U
#define SFMT_PARITY1 0x00000001U
#define SFMT_PARITY2 0x00000000U
#define SFMT_PARITY3 0x00000000U
#define SFMT_PARITY4 0x13c9e684U

/* PARAMETERS FOR ALTIVEC */
#if defined(__APPLE__) /* For OSX */
#define SFMT_ALTI_SL1                                                          \
  (vector unsigned int)(SFMT_SL1, SFMT_SL1, SFMT_SL1, SFMT_SL1)
#define SFMT_ALTI_SR1                                                          \
  (vector unsigned int)(SFMT_SR1, SFMT_SR1, SFMT_SR1, SFMT_SR1)
#define SFMT_ALTI_MSK                                                          \
  (vector unsigned int)(SFMT_MSK1, SFMT_MSK2, SFMT_MSK3, SFMT_MSK4)
#define SFMT_ALTI_MSK64                                                        \
  (vector unsigned int)(SFMT_MSK2, SFMT_MSK1, SFMT_MSK4, SFMT_MSK3)
#define SFMT_ALTI_SL2_PERM                                                     \
  (vector unsigned char)(1, 2, 3, 23, 5, 6, 7, 0, 9, 10, 11, 4, 13, 14, 15, 8)
#define SFMT_ALTI_SL2_PERM64                                                   \
  (vector unsigned char)(1, 2, 3, 4, 5, 6, 7, 31, 9, 10, 11, 12, 13, 14, 15, 0)
#define SFMT_ALTI_SR2_PERM                                                     \
  (vector unsigned char)(7, 0, 1, 2, 11, 4, 5, 6, 15, 8, 9, 10, 17, 12, 13, 14)
#define SFMT_ALTI_SR2_PERM64                                                   \
  (vector unsigned char)(15, 0, 1, 2, 3, 4, 5, 6, 17, 8, 9, 10, 11, 12, 13, 14)
#else /* For OTHER OSs(Linux?) */
#define SFMT_ALTI_SL1                                                          \
  { SFMT_SL1, SFMT_SL1, SFMT_SL1, SFMT_SL1 }
#define SFMT_ALTI_SR1                                                          \
  { SFMT_SR1, SFMT_SR1, SFMT_SR1, SFMT_SR1 }
#define SFMT_ALTI_MSK                                                          \
  { SFMT_MSK1, SFMT_MSK2, SFMT_MSK3, SFMT_MSK4 }
#define SFMT_ALTI_MSK64                                                        \
  { SFMT_MSK2, SFMT_MSK1, SFMT_MSK4, SFMT_MSK3 }
#define SFMT_ALTI_SL2_PERM                                                     \
  { 1, 2, 3, 23, 5, 6, 7, 0, 9, 10, 11, 4, 13, 14, 15, 8 }
#define SFMT_ALTI_SL2_PERM64                                                   \
  { 1, 2, 3, 4, 5, 6, 7, 31, 9, 10, 11, 12, 13, 14, 15, 0 }
#define SFMT_ALTI_SR2_PERM                                                     \
  { 7, 0, 1, 2, 11, 4, 5, 6, 15, 8, 9, 10, 17, 12, 13, 14 }
#define SFMT_ALTI_SR2_PERM64                                                   \
  { 15, 0, 1, 2, 3, 4, 5, 6, 17, 8, 9, 10, 11, 12, 13, 14 }
#endif /* For OSX */
#define SFMT_IDSTR                                                             \
  "SFMT-19937:122-18-1-11-1:dfffffef-ddfecb7f-bffaffff-bffffff6"  

/*------------------------------------------
  128-bit SIMD like data type for standard C
  ------------------------------------------*/
#if defined(HAVE_ALTIVEC_H)
  #if !defined(__APPLE__)
    #include <altivec.h>
  #endif
/** 128-bit data structure */
union w128_u {
    vector unsigned int s;
    uint32_t u[4];
    uint64_t u64[2];
};
#elif defined(HAVE_ARM_NEON_H)
  #include <arm_neon.h>

/** 128-bit data structure */
union w128_u {
    uint32_t u[4];
    uint64_t u64[2];
    uint32x4_t si;
};
#elif defined(__SSE2__) && defined(HAVE_EMMINTRIN_H)
  #include <emmintrin.h>

/** 128-bit data structure */
union w128_u {
    uint32_t u[4];
    uint64_t u64[2];
    __m128i si;
};
#else
/** 128-bit data structure */
union w128_u {
    uint32_t u[4];
    uint64_t u64[2];
};
#endif

/** 128-bit data type */
typedef union w128_u w128_t;

/**
 * SFMT internal state
 */
struct sfmt {
    /** the 128-bit internal state array */
    w128_t state[SFMT_N];
    /** index counter to the 32-bit internal state array */
    int idx;
};

typedef struct sfmt sfmt_t;

static void sfmt_fill_array32(sfmt_t * sfmt, uint32_t * array, int size);
static void sfmt_fill_array64(sfmt_t * sfmt, uint64_t * array, int size);
static void sfmt_init_gen_rand(sfmt_t * sfmt, uint32_t seed);
static void sfmt_init_by_array(sfmt_t * sfmt, uint32_t * init_key, int key_length);
static const char * sfmt_get_idstring(sfmt_t * sfmt);
static int sfmt_get_min_array_size32(sfmt_t * sfmt);
static int sfmt_get_min_array_size64(sfmt_t * sfmt);
static void sfmt_gen_rand_all(sfmt_t * sfmt);

#ifndef ONLY64
/**
 * This function generates and returns 32-bit pseudorandom number.
 * init_gen_rand or init_by_array must be called before this function.
 * @param sfmt SFMT internal state
 * @return 32-bit pseudorandom number
 */
inline static uint32_t sfmt_genrand_uint32(sfmt_t * sfmt) {
    uint32_t r;
    uint32_t * psfmt32 = &sfmt->state[0].u[0];

    if (sfmt->idx >= SFMT_N32) {
        sfmt_gen_rand_all(sfmt);
        sfmt->idx = 0;
    }
    r = psfmt32[sfmt->idx++];
    return r;
}
#endif
/**
 * This function generates and returns 64-bit pseudorandom number.
 * init_gen_rand or init_by_array must be called before this function.
 * The function gen_rand64 should not be called after gen_rand32,
 * unless an initialization is again executed.
 * @param sfmt SFMT internal state
 * @return 64-bit pseudorandom number
 */
inline static uint64_t sfmt_genrand_uint64(sfmt_t * sfmt) {
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    uint32_t * psfmt32 = &sfmt->state[0].u[0];
    uint32_t r1, r2;
#else
    uint64_t r;
#endif
    uint64_t * psfmt64 = &sfmt->state[0].u64[0];
    assert(sfmt->idx % 2 == 0);

    if (sfmt->idx >= SFMT_N32) {
        sfmt_gen_rand_all(sfmt);
        sfmt->idx = 0;
    }
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    r1 = psfmt32[sfmt->idx];
    r2 = psfmt32[sfmt->idx + 1];
    sfmt->idx += 2;
    return ((uint64_t)r2 << 32) | r1;
#else
    r = psfmt64[sfmt->idx / 2];
    sfmt->idx += 2;
    return r;
#endif
}

/* =================================================
   The following real versions are due to Isaku Wada
   ================================================= */
/**
 * converts an unsigned 32-bit number to a double on [0,1]-real-interval.
 * @param v 32-bit unsigned integer
 * @return double on [0,1]-real-interval
 */
inline static double sfmt_to_real1(uint32_t v)
{
    return v * (1.0/4294967295.0);
    /* divided by 2^32-1 */
}

/**
 * generates a random number on [0,1]-real-interval
 * @param sfmt SFMT internal state
 * @return double on [0,1]-real-interval
 */
inline static double sfmt_genrand_real1(sfmt_t * sfmt)
{
    return sfmt_to_real1(sfmt_genrand_uint32(sfmt));
}

/**
 * converts an unsigned 32-bit integer to a double on [0,1)-real-interval.
 * @param v 32-bit unsigned integer
 * @return double on [0,1)-real-interval
 */
inline static double sfmt_to_real2(uint32_t v)
{
    return v * (1.0/4294967296.0);
    /* divided by 2^32 */
}

/**
 * generates a random number on [0,1)-real-interval
 * @param sfmt SFMT internal state
 * @return double on [0,1)-real-interval
 */
inline static double sfmt_genrand_real2(sfmt_t * sfmt)
{
    return sfmt_to_real2(sfmt_genrand_uint32(sfmt));
}

/**
 * converts an unsigned 32-bit integer to a double on (0,1)-real-interval.
 * @param v 32-bit unsigned integer
 * @return double on (0,1)-real-interval
 */
inline static double sfmt_to_real3(uint32_t v)
{
    return (((double)v) + 0.5)*(1.0/4294967296.0);
    /* divided by 2^32 */
}

/**
 * generates a random number on (0,1)-real-interval
 * @param sfmt SFMT internal state
 * @return double on (0,1)-real-interval
 */
inline static double sfmt_genrand_real3(sfmt_t * sfmt)
{
    return sfmt_to_real3(sfmt_genrand_uint32(sfmt));
}

/**
 * converts an unsigned 32-bit integer to double on [0,1)
 * with 53-bit resolution.
 * @param v 32-bit unsigned integer
 * @return double on [0,1)-real-interval with 53-bit resolution.
 */
inline static double sfmt_to_res53(uint64_t v)
{
    return (v >> 11) * (1.0/9007199254740992.0);
}

/**
 * generates a random number on [0,1) with 53-bit resolution
 * @param sfmt SFMT internal state
 * @return double on [0,1) with 53-bit resolution
 */
inline static double sfmt_genrand_res53(sfmt_t * sfmt)
{
    return sfmt_to_res53(sfmt_genrand_uint64(sfmt));
}


/* =================================================
   The following function are added by Saito.
   ================================================= */
/**
 * generates a random number on [0,1) with 53-bit resolution from two
 * 32 bit integers
 */
inline static double sfmt_to_res53_mix(uint32_t x, uint32_t y)
{
    return sfmt_to_res53(x | ((uint64_t)y << 32));
}

/**
 * generates a random number on [0,1) with 53-bit resolution
 * using two 32bit integers.
 * @param sfmt SFMT internal state
 * @return double on [0,1) with 53-bit resolution
 */
inline static double sfmt_genrand_res53_mix(sfmt_t * sfmt)
{
    uint32_t x, y;

    x = sfmt_genrand_uint32(sfmt);
    y = sfmt_genrand_uint32(sfmt);
    return sfmt_to_res53_mix(x, y);
}

#if defined(__cplusplus)
}
#endif

typedef struct sfmt_state {
  sfmt_t *state;
  int has_uint32;
  uint32_t uinteger;

  uint64_t *buffered_uint64;
  int buffer_loc;
} sfmt_state_t;

static inline uint64_t sfmt_next_buffer(sfmt_state_t *state) {
  if (state->buffer_loc < SFMT_N64) {
    uint64_t out = state->buffered_uint64[state->buffer_loc];
    state->buffer_loc++;
    return out;
  }
  sfmt_fill_array64(state->state, state->buffered_uint64, SFMT_N64);
  state->buffer_loc = 1;
  return state->buffered_uint64[0];
}

static inline uint64_t sfmt_next64(sfmt_state_t *state) {
  return sfmt_next_buffer(state);
}

static inline uint32_t sfmt_next32(sfmt_state_t *state) {
  /* Discard bottom 16 bits */
  uint64_t next;
  if (state->has_uint32) {
    state->has_uint32 = 0;
    return state->uinteger;
  }
  next = sfmt_next_buffer(state);

  state->has_uint32 = 1;
  state->uinteger = (uint32_t)(next >> 32);
  return (uint32_t)(next & 0xffffffff);
}

//#include "sfmt-common.h"
/**
 * This function simulates SIMD 128-bit right shift by the standard C.
 * The 128-bit integer given in in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
#ifdef ONLY64
inline static void rshift128(w128_t *out, w128_t const *in, int shift) {
  uint64_t th, tl, oh, ol;

  th = ((uint64_t)in->u[2] << 32) | ((uint64_t)in->u[3]);
  tl = ((uint64_t)in->u[0] << 32) | ((uint64_t)in->u[1]);

  oh = th >> (shift * 8);
  ol = tl >> (shift * 8);
  ol |= th << (64 - shift * 8);
  out->u[0] = (uint32_t)(ol >> 32);
  out->u[1] = (uint32_t)ol;
  out->u[2] = (uint32_t)(oh >> 32);
  out->u[3] = (uint32_t)oh;
}
#else
inline static void rshift128(w128_t *out, w128_t const *in, int shift) {
  uint64_t th, tl, oh, ol;

  th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
  tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

  oh = th >> (shift * 8);
  ol = tl >> (shift * 8);
  ol |= th << (64 - shift * 8);
  out->u[1] = (uint32_t)(ol >> 32);
  out->u[0] = (uint32_t)ol;
  out->u[3] = (uint32_t)(oh >> 32);
  out->u[2] = (uint32_t)oh;
}
#endif
/**
 * This function simulates SIMD 128-bit left shift by the standard C.
 * The 128-bit integer given in in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
#ifdef ONLY64
inline static void lshift128(w128_t *out, w128_t const *in, int shift) {
  uint64_t th, tl, oh, ol;

  th = ((uint64_t)in->u[2] << 32) | ((uint64_t)in->u[3]);
  tl = ((uint64_t)in->u[0] << 32) | ((uint64_t)in->u[1]);

  oh = th << (shift * 8);
  ol = tl << (shift * 8);
  oh |= tl >> (64 - shift * 8);
  out->u[0] = (uint32_t)(ol >> 32);
  out->u[1] = (uint32_t)ol;
  out->u[2] = (uint32_t)(oh >> 32);
  out->u[3] = (uint32_t)oh;
}
#else
inline static void lshift128(w128_t *out, w128_t const *in, int shift) {
  uint64_t th, tl, oh, ol;

  th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
  tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

  oh = th << (shift * 8);
  ol = tl << (shift * 8);
  oh |= tl >> (64 - shift * 8);
  out->u[1] = (uint32_t)(ol >> 32);
  out->u[0] = (uint32_t)ol;
  out->u[3] = (uint32_t)(oh >> 32);
  out->u[2] = (uint32_t)oh;
}
#endif
/**
 * This function represents the recursion formula.
 * @param r output
 * @param a a 128-bit part of the internal state array
 * @param b a 128-bit part of the internal state array
 * @param c a 128-bit part of the internal state array
 * @param d a 128-bit part of the internal state array
 */
#ifdef ONLY64
inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
                                w128_t *d) {
  w128_t x;
  w128_t y;

  lshift128(&x, a, SFMT_SL2);
  rshift128(&y, c, SFMT_SR2);
  r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SFMT_SR1) & SFMT_MSK2) ^ y.u[0] ^
            (d->u[0] << SFMT_SL1);
  r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SFMT_SR1) & SFMT_MSK1) ^ y.u[1] ^
            (d->u[1] << SFMT_SL1);
  r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SFMT_SR1) & SFMT_MSK4) ^ y.u[2] ^
            (d->u[2] << SFMT_SL1);
  r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SFMT_SR1) & SFMT_MSK3) ^ y.u[3] ^
            (d->u[3] << SFMT_SL1);
}
#else
inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
                                w128_t *d) {
  w128_t x;
  w128_t y;

  lshift128(&x, a, SFMT_SL2);
  rshift128(&y, c, SFMT_SR2);
  r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SFMT_SR1) & SFMT_MSK1) ^ y.u[0] ^
            (d->u[0] << SFMT_SL1);
  r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SFMT_SR1) & SFMT_MSK2) ^ y.u[1] ^
            (d->u[1] << SFMT_SL1);
  r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SFMT_SR1) & SFMT_MSK3) ^ y.u[2] ^
            (d->u[2] << SFMT_SL1);
  r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SFMT_SR1) & SFMT_MSK4) ^ y.u[3] ^
            (d->u[3] << SFMT_SL1);
}
#endif

//#include "sfmt-jump.h"
//static void SFMT_jump(sfmt_t *sfmt, const char *jump_str);
//inline static void next_state(sfmt_t *sfmt);

#if defined(HAVE_SSE2)
/**
 * add internal state of src to dest as F2-vector.
 * @param dest destination state
 * @param src source state
 */
inline static void add(sfmt_t *dest, sfmt_t *src) {
  int dp = dest->idx / 4;
  int sp = src->idx / 4;
  int diff = (sp - dp + SFMT_N) % SFMT_N;
  int p;
  int i;
  for (i = 0; i < SFMT_N - diff; i++) {
    p = i + diff;
    dest->state[i].si = _mm_xor_si128(dest->state[i].si, src->state[p].si);
  }
  for (; i < SFMT_N; i++) {
    p = i + diff - SFMT_N;
    dest->state[i].si = _mm_xor_si128(dest->state[i].si, src->state[p].si);
  }
}
#else
inline static void add(sfmt_t *dest, sfmt_t *src) {
  int dp = dest->idx / 4;
  int sp = src->idx / 4;
  int diff = (sp - dp + SFMT_N) % SFMT_N;
  int p;
  int i, j;
  for (i = 0; i < SFMT_N - diff; i++) {
    p = i + diff;
    for (int j = 0; j < 4; j++) {
      dest->state[i].u[j] ^= src->state[p].u[j];
    }
  }
  for (; i < SFMT_N; i++) {
    p = i + diff - SFMT_N;
    for (j = 0; j < 4; j++) {
      dest->state[i].u[j] ^= src->state[p].u[j];
    }
  }
}
#endif

/**
 * calculate next state
 * @param sfmt SFMT internal state
 */
inline static void next_state(sfmt_t *sfmt) {
  int idx = (sfmt->idx / 4) % SFMT_N;
  w128_t *r1, *r2;
  w128_t *pstate = sfmt->state;

  r1 = &pstate[(idx + SFMT_N - 2) % SFMT_N];
  r2 = &pstate[(idx + SFMT_N - 1) % SFMT_N];
  do_recursion(&pstate[idx], &pstate[idx], &pstate[(idx + SFMT_POS1) % SFMT_N],
               r1, r2);
  r1 = r2;
  r2 = &pstate[idx];
  sfmt->idx = sfmt->idx + 4;
}

/**
 * jump ahead using jump_string
 * @param sfmt SFMT internal state input and output.
 * @param jump_string string which represents jump polynomial.
 */
void SFMT_jump(sfmt_t *sfmt, const char *jump_string) {
  sfmt_t work;
  int index = sfmt->idx;
  int bits, i, j;
  memset(&work, 0, sizeof(sfmt_t));
  sfmt->idx = SFMT_N32;

  for (i = 0; jump_string[i] != '\0'; i++) {
    bits = jump_string[i];
    assert(isxdigit(bits));
    bits = tolower(bits);
    if (bits >= 'a' && bits <= 'f') {
      bits = bits - 'a' + 10;
    } else {
      bits = bits - '0';
    }
    bits = bits & 0x0f;
    for (j = 0; j < 4; j++) {
      if ((bits & 1) != 0) {
        add(&work, sfmt);
      }
      next_state(sfmt);
      bits = bits >> 1;
    }
  }
  *sfmt = work;
  sfmt->idx = index;
}

//#include "sfmt-params.h"
#include "sfmt-poly.h"
#include <assert.h>
#include <string.h>

//#if defined(__BIG_ENDIAN__) && !defined(__amd64) && !defined(BIG_ENDIAN64)
//#define BIG_ENDIAN64 1
//#endif
//#if defined(HAVE_ALTIVEC) && !defined(BIG_ENDIAN64)
//#define BIG_ENDIAN64 1
//#endif
//#if defined(ONLY64) && !defined(BIG_ENDIAN64)
//#if defined(__GNUC__)
//#error "-DONLY64 must be specified with -DBIG_ENDIAN64"
//#endif
//#undef ONLY64
//#endif

/*----------------
  STATIC FUNCTIONS
  ----------------*/
//inline static int idxof(int i);
//inline static void gen_rand_array(sfmt_t *sfmt, w128_t *array, int size);
//inline static uint32_t func1(uint32_t x);
//inline static uint32_t func2(uint32_t x);
//static void period_certification(sfmt_t *sfmt);
//#if defined(BIG_ENDIAN64) && !defined(ONLY64)
//inline static void swap(w128_t *array, int size);
//#endif

#if defined(HAVE_ALTIVEC_H)
#define HAVE_ALTIVEC
#include "sfmt-alti.h"
#elif defined(__SSE2__) && __SSE2__
#define HAVE_SSE2
/**
 * parameters used by sse2.
 */
#if defined(__GNUC__) && !defined(__ICC)
/* #warning "Compiling with SSE2 support" */
#endif
#if defined(_MSC_VER)
/* #pragma message("Compiling with SSE2 support") */
#endif
static const w128_t sse2_param_mask = {
    {SFMT_MSK1, SFMT_MSK2, SFMT_MSK3, SFMT_MSK4}};
#if defined(_MSC_VER)
#include "sfmt-sse2-msc.h"
#else
#include "sfmt-sse2.h"
#endif
#elif defined(HAVE_NEON_H)
#define HAVE_NEON
#include "sfmt-neon.h"
#endif

/**
 * This function simulate a 64-bit index of LITTLE ENDIAN
 * in BIG ENDIAN machine.
 */
#ifdef ONLY64
inline static int idxof(int i) { return i ^ 1; }
#else
inline static int idxof(int i) { return i; }
#endif

#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2)) && (!defined(HAVE_NEON))
/**
 * This function fills the user-specified array with pseudorandom
 * integers.
 *
 * @param sfmt SFMT internal state
 * @param array an 128-bit array to be filled by pseudorandom numbers.
 * @param size number of 128-bit pseudorandom numbers to be generated.
 */
inline static void gen_rand_array(sfmt_t *sfmt, w128_t *array, int size) {
  int i, j;
  w128_t *r1, *r2;

  r1 = &sfmt->state[SFMT_N - 2];
  r2 = &sfmt->state[SFMT_N - 1];
  for (i = 0; i < SFMT_N - SFMT_POS1; i++) {
    do_recursion(&array[i], &sfmt->state[i], &sfmt->state[i + SFMT_POS1], r1,
                 r2);
    r1 = r2;
    r2 = &array[i];
  }
  for (; i < SFMT_N; i++) {
    do_recursion(&array[i], &sfmt->state[i], &array[i + SFMT_POS1 - SFMT_N], r1,
                 r2);
    r1 = r2;
    r2 = &array[i];
  }
  for (; i < size - SFMT_N; i++) {
    do_recursion(&array[i], &array[i - SFMT_N], &array[i + SFMT_POS1 - SFMT_N],
                 r1, r2);
    r1 = r2;
    r2 = &array[i];
  }
  for (j = 0; j < 2 * SFMT_N - size; j++) {
    sfmt->state[j] = array[j + size - SFMT_N];
  }
  for (; i < size; i++, j++) {
    do_recursion(&array[i], &array[i - SFMT_N], &array[i + SFMT_POS1 - SFMT_N],
                 r1, r2);
    r1 = r2;
    r2 = &array[i];
    sfmt->state[j] = array[i];
  }
}
#endif

#if defined(BIG_ENDIAN64) && !defined(ONLY64) && !defined(HAVE_ALTIVEC)
inline static void swap(w128_t *array, int size) {
  int i;
  uint32_t x, y;

  for (i = 0; i < size; i++) {
    x = array[i].u[0];
    y = array[i].u[2];
    array[i].u[0] = array[i].u[1];
    array[i].u[2] = array[i].u[3];
    array[i].u[1] = x;
    array[i].u[3] = y;
  }
}
#endif
/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
static uint32_t func1(uint32_t x) {
  return (x ^ (x >> 27)) * (uint32_t)1664525UL;
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
static uint32_t func2(uint32_t x) {
  return (x ^ (x >> 27)) * (uint32_t)1566083941UL;
}

/**
 * This function certificate the period of 2^{MEXP}
 * @param sfmt SFMT internal state
 */
static void period_certification(sfmt_t *sfmt) {
  uint32_t inner = 0;
  int i, j;
  uint32_t work;
  uint32_t *psfmt32 = &sfmt->state[0].u[0];
  const uint32_t parity[4] = {SFMT_PARITY1, SFMT_PARITY2, SFMT_PARITY3,
                              SFMT_PARITY4};

  for (i = 0; i < 4; i++) {
    inner ^= psfmt32[idxof(i)] & parity[i];
  }
  for (i = 16; i > 0; i >>= 1) {
    inner ^= inner >> i;
  }
  inner &= 1;
  /* check OK */
  if (inner == 1) {
    return;
  }
  /* check NG, and modification */
  for (i = 0; i < 4; i++) {
    work = 1;
    for (j = 0; j < 32; j++) {
      if ((work & parity[i]) != 0) {
        psfmt32[idxof(i)] ^= work;
        return;
      }
      work = work << 1;
    }
  }
}

/*----------------
  PUBLIC FUNCTIONS
  ----------------*/
/**
 * This function returns the identification string.
 * The string shows the word size, the Mersenne exponent,
 * and all parameters of this generator.
 * @param sfmt SFMT internal state
 */
static const char *sfmt_get_idstring(UNUSED_PARAM sfmt_t *sfmt) {
  return SFMT_IDSTR;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array32() function.
 * @param sfmt SFMT internal state
 * @return minimum size of array used for fill_array32() function.
 */
static int sfmt_get_min_array_size32(UNUSED_PARAM sfmt_t *sfmt) {
  return SFMT_N32;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array64() function.
 * @param sfmt SFMT internal state
 * @return minimum size of array used for fill_array64() function.
 */
static int sfmt_get_min_array_size64(UNUSED_PARAM sfmt_t *sfmt) {
  return SFMT_N64;
}

#if !defined(HAVE_SSE2) && !defined(HAVE_ALTIVEC) && !defined(HAVE_NEON)
/**
 * This function fills the internal state array with pseudorandom
 * integers.
 * @param sfmt SFMT internal state
 */
static void sfmt_gen_rand_all(sfmt_t *sfmt) {
  int i;
  w128_t *r1, *r2;

  r1 = &sfmt->state[SFMT_N - 2];
  r2 = &sfmt->state[SFMT_N - 1];
  for (i = 0; i < SFMT_N - SFMT_POS1; i++) {
    do_recursion(&sfmt->state[i], &sfmt->state[i], &sfmt->state[i + SFMT_POS1],
                 r1, r2);
    r1 = r2;
    r2 = &sfmt->state[i];
  }
  for (; i < SFMT_N; i++) {
    do_recursion(&sfmt->state[i], &sfmt->state[i],
                 &sfmt->state[i + SFMT_POS1 - SFMT_N], r1, r2);
    r1 = r2;
    r2 = &sfmt->state[i];
  }
}
#endif

#ifndef ONLY64
/**
 * This function generates pseudorandom 32-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 624 and a
 * multiple of four.  The generation by this function is much faster
 * than the following gen_rand function.
 *
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling gen_rand function, without initialization.
 *
 * @param sfmt SFMT internal state
 * @param array an array where pseudorandom 32-bit integers are filled
 * by this function.  The pointer to the array must be \b "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 32-bit pseudorandom integers to be
 * generated.  size must be a multiple of 4, and greater than or equal
 * to (MEXP / 128 + 1) * 4.
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void sfmt_fill_array32(sfmt_t *sfmt, uint32_t *array, int size) {
  assert(sfmt->idx == SFMT_N32);
  assert(size % 4 == 0);
  assert(size >= SFMT_N32);

  gen_rand_array(sfmt, (w128_t *)array, size / 4);
  sfmt->idx = SFMT_N32;
}
#endif

/**
 * This function generates pseudorandom 64-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 312 and a
 * multiple of two.  The generation by this function is much faster
 * than the following gen_rand function.
 *
 * @param sfmt SFMT internal state
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling gen_rand function, without initialization.
 *
 * @param array an array where pseudorandom 64-bit integers are filled
 * by this function.  The pointer to the array must be "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 64-bit pseudorandom integers to be
 * generated.  size must be a multiple of 2, and greater than or equal
 * to (MEXP / 128 + 1) * 2
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
static void sfmt_fill_array64(sfmt_t *sfmt, uint64_t *array, int size) {
  //assert(sfmt->idx == SFMT_N32);
  assert(size % 2 == 0);
  assert(size >= SFMT_N64);

  gen_rand_array(sfmt, (w128_t *)array, size / 2);
  sfmt->idx = SFMT_N32;

#if defined(BIG_ENDIAN64) && !defined(ONLY64)
  swap((w128_t *)array, size / 2);
#endif
}

/**
 * This function initializes the internal state array with a 32-bit
 * integer seed.
 *
 * @param sfmt SFMT internal state
 * @param seed a 32-bit integer used as the seed.
 */
static void sfmt_init_gen_rand(sfmt_t *sfmt, uint32_t seed) {
  int i;

  uint32_t *psfmt32 = &sfmt->state[0].u[0];

  psfmt32[idxof(0)] = seed;
  for (i = 1; i < SFMT_N32; i++) {
    psfmt32[idxof(i)] =
        1812433253UL * (psfmt32[idxof(i - 1)] ^ (psfmt32[idxof(i - 1)] >> 30)) +
        i;
  }
  sfmt->idx = SFMT_N32;
  period_certification(sfmt);
}

/**
 * This function initializes the internal state array,
 * with an array of 32-bit integers used as the seeds
 * @param sfmt SFMT internal state
 * @param init_key the array of 32-bit integers, used as a seed.
 * @param key_length the length of init_key.
 */
static void sfmt_init_by_array(sfmt_t *sfmt, uint32_t *init_key, int key_length) {
  int i, j, count;
  uint32_t r;
  int lag;
  int mid;
  int size = SFMT_N * 4;
  uint32_t *psfmt32 = &sfmt->state[0].u[0];

  if (size >= 623) {
    lag = 11;
  } else if (size >= 68) {
    lag = 7;
  } else if (size >= 39) {
    lag = 5;
  } else {
    lag = 3;
  }
  mid = (size - lag) / 2;

  memset(sfmt, 0x8b, sizeof(sfmt_t));
  if (key_length + 1 > SFMT_N32) {
    count = key_length + 1;
  } else {
    count = SFMT_N32;
  }
  r = func1(psfmt32[idxof(0)] ^ psfmt32[idxof(mid)] ^
            psfmt32[idxof(SFMT_N32 - 1)]);
  psfmt32[idxof(mid)] += r;
  r += key_length;
  psfmt32[idxof(mid + lag)] += r;
  psfmt32[idxof(0)] = r;

  count--;
  for (i = 1, j = 0; (j < count) && (j < key_length); j++) {
    r = func1(psfmt32[idxof(i)] ^ psfmt32[idxof((i + mid) % SFMT_N32)] ^
              psfmt32[idxof((i + SFMT_N32 - 1) % SFMT_N32)]);
    psfmt32[idxof((i + mid) % SFMT_N32)] += r;
    r += init_key[j] + i;
    psfmt32[idxof((i + mid + lag) % SFMT_N32)] += r;
    psfmt32[idxof(i)] = r;
    i = (i + 1) % SFMT_N32;
  }
  for (; j < count; j++) {
    r = func1(psfmt32[idxof(i)] ^ psfmt32[idxof((i + mid) % SFMT_N32)] ^
              psfmt32[idxof((i + SFMT_N32 - 1) % SFMT_N32)]);
    psfmt32[idxof((i + mid) % SFMT_N32)] += r;
    r += i;
    psfmt32[idxof((i + mid + lag) % SFMT_N32)] += r;
    psfmt32[idxof(i)] = r;
    i = (i + 1) % SFMT_N32;
  }
  for (j = 0; j < SFMT_N32; j++) {
    r = func2(psfmt32[idxof(i)] + psfmt32[idxof((i + mid) % SFMT_N32)] +
              psfmt32[idxof((i + SFMT_N32 - 1) % SFMT_N32)]);
    psfmt32[idxof((i + mid) % SFMT_N32)] ^= r;
    r -= i;
    psfmt32[idxof((i + mid + lag) % SFMT_N32)] ^= r;
    psfmt32[idxof(i)] = r;
    i = (i + 1) % SFMT_N32;
  }

  sfmt->idx = SFMT_N32;
  period_certification(sfmt);
}

//unused
//static void sfmt_jump(sfmt_state_t *state) { SFMT_jump(state->state, poly_128); };

static void sfmt_jump_n(sfmt_state_t *state, long count) {
  /* poly_xxx is 2**xxx ahead */
  long remaining = count;
  while (remaining > 0) {
    if (remaining >= 65536) {
      SFMT_jump(state->state, poly_144);
      remaining -= 65536;
    } else if (remaining >= 4096) {
      SFMT_jump(state->state, poly_140);
      remaining -= 4096;
    } else if (remaining >= 256) {
      SFMT_jump(state->state, poly_136);
      remaining -= 256;
    } else if (remaining >= 16) {
      SFMT_jump(state->state, poly_132);
      remaining -= 16;
    } else {
      SFMT_jump(state->state, poly_128);
      remaining -= 1;
    }
  }
}

#if defined(__cplusplus)
}
#endif

// ===================================================

static void sfmt_set(void *vstate, unsigned long int seed)
{
  sfmt_state_t* state = (sfmt_state_t*) vstate;
  if (!state->state) {
    state->state = calloc (1, sizeof(sfmt_t)); // leak!
    state->buffered_uint64 = calloc (SFMT_N64, sizeof(uint64_t)); // leak!
  }
  sfmt_init_gen_rand (state->state, (uint32_t)seed);
}

static unsigned long int sfmt_get(void *vstate)
{
  sfmt_state_t* state = (sfmt_state_t*) vstate;  
  return (unsigned long int)sfmt_next32(state);
}

static double sfmt_get_double (void *vstate)
{
  sfmt_state_t* state = (sfmt_state_t*) vstate;
  return sfmt_genrand_res53_mix(state->state);
}

static const gsl_rng_type sfmt_type =
{"sfmt",                        /* name */
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (sfmt_state_t),
 &sfmt_set,
 &sfmt_get,
 &sfmt_get_double};

const gsl_rng_type *gsl_rng_sfmt = &sfmt_type;
