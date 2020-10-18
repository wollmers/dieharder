/*
  rng_chacha.c
  Copyright (c) 2015 Orson Peters <orsonpeters@gmail.com>
  Copyright (c) 2020 Reini Urban. dieharder/gsl integration.

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software. If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  3. This notice may not be removed or altered from any source
     distribution.

  Taken from the implementation in randomgen.
*/

#include <dieharder/libdieharder.h>
#include "cpu_features.h"

#include <stdint.h>

#if defined(_WIN32) && defined(_MSC_VER)
#define M128I_CAST
#else
#define M128I_CAST (__m128i)
#endif

int DH_USE_SIMD;

typedef double * aligned_double_ptr ;

struct chacha_state {
    uint32_t block[16] __attribute__((aligned(16)));
    uint32_t keysetup[8];
    uint64_t ctr[2];
    int rounds;
} __attribute__((aligned(16)));

typedef struct chacha_state chacha_state_t;


#if defined(__SSE2__) && __SSE2__
// Get an efficient _mm_roti_epi32 based on enabled features.
#if !defined(__XOP__)
    #if defined(__SSSE3__) && __SSSE3__
        #undef _mm_roti_epi32 /* Silence warnings on some compiler */
        #define _mm_roti_epi32(r, c) (                              \
            ((c) == 8) ?                                            \
                _mm_shuffle_epi8((r), _mm_set_epi8(14, 13, 12, 15,  \
                                                   10,  9,  8, 11,  \
                                                    6,  5,  4,  7,  \
                                                    2,  1,  0,  3)) \
            : ((c) == 16) ?                                         \
                _mm_shuffle_epi8((r), _mm_set_epi8(13, 12, 15, 14,  \
                                                    9,  8, 11, 10,  \
                                                    5,  4,  7,  6,  \
                                                    1,  0,  3,  2)) \
            : ((c) == 24) ?                                         \
                _mm_shuffle_epi8((r), _mm_set_epi8(12, 15, 14, 13,  \
                                                    8, 11, 10,  9,  \
                                                    4,  7,  6,  5,  \
                                                    0,  3,  2,  1)) \
            :                                                       \
                _mm_xor_si128(_mm_slli_epi32((r), (c)),             \
                              _mm_srli_epi32((r), 32-(c)))          \
        )
    #else
        #undef _mm_roti_epi32 /* Silence warnings on some compiler */
        #define _mm_roti_epi32(r, c) _mm_xor_si128(_mm_slli_epi32((r), (c)), \
                                                   _mm_srli_epi32((r), 32-(c)))
    #endif
#else
    #include <xopintrin.h>
#endif

static inline void chacha_core_ssse3(chacha_state_t *state) {
    // ROTVn rotates the elements in the given vector n places to the left.
    int i;

    #define CHACHA_ROTV1(x) _mm_shuffle_epi32(M128I_CAST x, 0x39)
    #define CHACHA_ROTV2(x) _mm_shuffle_epi32(M128I_CAST x, 0x4e)
    #define CHACHA_ROTV3(x) _mm_shuffle_epi32(M128I_CAST x, 0x93)

    __m128i a = _mm_load_si128((__m128i*) (&state->block[0]));
    __m128i b = _mm_load_si128((__m128i*) (&state->block[4]));
    __m128i c = _mm_load_si128((__m128i*) (&state->block[8]));
    __m128i d = _mm_load_si128((__m128i*) (&state->block[12]));

    for (i = 0; i < state->rounds; i += 2) {
        a = _mm_add_epi32(a, b);
        d = _mm_xor_si128(d, a);
        d = _mm_roti_epi32(d, 16);
        c = _mm_add_epi32(c, d);
        b = _mm_xor_si128(b, c);
        b = _mm_roti_epi32(b, 12);
        a = _mm_add_epi32(a, b);
        d = _mm_xor_si128(d, a);
        d = _mm_roti_epi32(d, 8);
        c = _mm_add_epi32(c, d);
        b = _mm_xor_si128(b, c);
        b = _mm_roti_epi32(b, 7);

        b = CHACHA_ROTV1(b);
        c = CHACHA_ROTV2(c);
        d = CHACHA_ROTV3(d);

        a = _mm_add_epi32(a, b);
        d = _mm_xor_si128(d, a);
        d = _mm_roti_epi32(d, 16);
        c = _mm_add_epi32(c, d);
        b = _mm_xor_si128(b, c);
        b = _mm_roti_epi32(b, 12);
        a = _mm_add_epi32(a, b);
        d = _mm_xor_si128(d, a);
        d = _mm_roti_epi32(d, 8);
        c = _mm_add_epi32(c, d);
        b = _mm_xor_si128(b, c);
        b = _mm_roti_epi32(b, 7);

        b = CHACHA_ROTV3(b);
        c = CHACHA_ROTV2(c);
        d = CHACHA_ROTV1(d);
    }

    _mm_store_si128((__m128i*) (&state->block[0]), a);
    _mm_store_si128((__m128i*) (&state->block[4]), b);
    _mm_store_si128((__m128i*) (&state->block[8]), c);
    _mm_store_si128((__m128i*) (&state->block[12]), d);

    #undef CHACHA_ROTV3
    #undef CHACHA_ROTV2
    #undef CHACHA_ROTV1
}
#endif

static inline void chacha_core(chacha_state_t *state) {
    int i;
    #define CHACHA_ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

    #define CHACHA_QUARTERROUND(x, a, b, c, d) \
        x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = CHACHA_ROTL32(x[d], 16); \
        x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = CHACHA_ROTL32(x[b], 12); \
        x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = CHACHA_ROTL32(x[d],  8); \
        x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = CHACHA_ROTL32(x[b],  7)

    for (i = 0; i < state->rounds ; i += 2) {
        CHACHA_QUARTERROUND(state->block, 0, 4,  8, 12);
        CHACHA_QUARTERROUND(state->block, 1, 5,  9, 13);
        CHACHA_QUARTERROUND(state->block, 2, 6, 10, 14);
        CHACHA_QUARTERROUND(state->block, 3, 7, 11, 15);
        CHACHA_QUARTERROUND(state->block, 0, 5, 10, 15);
        CHACHA_QUARTERROUND(state->block, 1, 6, 11, 12);
        CHACHA_QUARTERROUND(state->block, 2, 7,  8, 13);
        CHACHA_QUARTERROUND(state->block, 3, 4,  9, 14);
    }

    #undef CHACHA_QUARTERROUND
    #undef CHACHA_ROTL32
}

static inline void generate_block(chacha_state_t *state) {
    int i;
    uint32_t constants[4] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};

    uint32_t input[16];
    for (i = 0; i < 4; ++i) input[i] = constants[i];
    for (i = 0; i < 8; ++i) input[4 + i] = state->keysetup[i];
    // Using a 128-bit counter.
    input[12] = (state->ctr[0] / 16) & 0xffffffffu;
    // Carry from the top part of ctr
    input[13] = (((uint32_t)(state->ctr[1]) % 16) << 28) | ((state->ctr[0] / 16) >> 32);
    input[14] = (state->ctr[1] / 16) & 0xffffffffu;
    input[15] = (state->ctr[1] / 16) >> 32;

    for (i = 0; i < 16; ++i) state->block[i] = input[i];
#if defined(__SSE2__) && __SSE2__
    if LIKELY(DH_USE_SIMD > 0) {
        chacha_core_ssse3(state);
    } else {
#endif
        chacha_core(state);
#if defined(__SSE2__) && __SSE2__
    }
#endif
    for (i = 0; i < 16; ++i) state->block[i] += input[i];
}

static inline uint32_t chacha_next32(chacha_state_t *state){
    int idx = state->ctr[0] % 16;
    if UNLIKELY(idx == 0) generate_block(state);
    ++state->ctr[0];
    if (state->ctr[0] == 0) ++state->ctr[1];

    return state->block[idx];
}

static inline uint64_t chacha_next64(chacha_state_t *state){
    uint64_t out =  chacha_next32(state) | ((uint64_t)chacha_next32(state) << 32);
    return out;
}

static inline double chacha_next_double(chacha_state_t *state){
    return (chacha_next64(state) >> 11) * (1.0/9007199254740992.0);
}

#if defined(__SSE2__) && __SSE2__
#if defined(__SSSE3__) && __SSSE3__
#define CHACHA_FEATURE_REG DH_ECX
#define CHACHA_FEATURE_FLAG 9
#else
#define CHACHA_FEATURE_REG DH_EDX
#define CHACHA_FEATURE_FLAG 26
#endif
#else
#define CHACHA_FEATURE_FLAG 0
#endif

extern int chacha_simd_capable(void) {
#if defined(__SSE2__) && __SSE2__
  int flags[32];
  x86_feature_flags(flags, CHACHA_FEATURE_REG);
  DH_USE_SIMD = flags[CHACHA_FEATURE_FLAG];
  return DH_USE_SIMD;
#else
  DH_USE_SIMD = 0;
  return 0;
#endif
}

extern void chacha_use_simd(int flag) { DH_USE_SIMD = flag; }

static void chacha_seed(chacha_state_t *state, uint64_t *seedval, uint64_t *stream,
                        uint64_t *ctr) {
  chacha_simd_capable();
  // Using a 128-bit seed.
  state->keysetup[0] = seedval[0] & 0xffffffffu;
  state->keysetup[1] = seedval[0] >> 32;
  state->keysetup[2] = seedval[1] & 0xffffffffu;
  state->keysetup[3] = seedval[1] >> 32;
  // Using a 128-bit stream.
  state->keysetup[4] = stream[0] & 0xffffffffu;
  state->keysetup[5] = stream[0] >> 32;
  state->keysetup[6] = stream[1] & 0xffffffffu;
  state->keysetup[7] = stream[1] >> 32;

  /* Ensure ctr[0] is at a node where a block would be generated */
  state->ctr[0] = ((ctr[0] >> 4) << 4);
  state->ctr[1] = ctr[1];
  generate_block(state);
  /* Store correct value of counter */
  state->ctr[0] = ctr[0];
}

static void chacha_advance(chacha_state_t *state, uint64_t *delta) {
  int carry, idx = state->ctr[0] % 16;
  uint64_t orig;
  orig = state->ctr[0];
  state->ctr[0] += delta[0];
  carry = state->ctr[0] < orig;
  state->ctr[1] += (delta[1] + carry);
  if ((idx + delta[0] >= 16 || delta[1]) && ((state->ctr[0] % 16) != 0)) {
    generate_block(state);
  }
}

static void chacha_set(void *vstate, unsigned long int seed)
{
  chacha_state_t* state = (chacha_state_t*) vstate;
  // Using 2x 128-bit seeds
  uint64_t iv[4];
  uint64_t ctr[2] = {0UL, 0UL};
  uint64_t tmp = (uint64_t)seed;
  for (int i = 0; i < 4; i++) {
    iv[i] = splitmix64_next(&tmp);
  }
  (void)chacha_seed(state, &iv[0], &iv[2], ctr);
}

static unsigned long int chacha_get(void *vstate)
{
  chacha_state_t* state = (chacha_state_t*) vstate;
  return (unsigned long int)chacha_next64(state);
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double chacha_get_double (void *vstate)
{
  chacha_state_t* state = (chacha_state_t*) vstate;
  return TO_DOUBLE(chacha_next64(state));
}

static const gsl_rng_type chacha_type =
{"chacha",                      /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (chacha_state_t),
 &chacha_set,
 &chacha_get,
 &chacha_get_double};

const gsl_rng_type *gsl_rng_chacha = &chacha_type;
