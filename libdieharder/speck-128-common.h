#ifndef _SPECK_COMMON_H
#define _SPECK_COMMON_H 1

#undef VERSION
#include "config.h"

#define SPECK_UNROLL     12
#define SPECK_BUFFER_SZ  (8 * SPECK_UNROLL)
#define SPECK_ROUNDS     34
#define SPECK_MAX_ROUNDS 34 /* Only correct for 128x256 */
#define SPECK_CTR_SZ     (SPECK_UNROLL / 2)

union speck_u {
#if defined(__SSSE3__) && __SSSE3__
  __m128i m128;
#endif
  uint64_t u64[2];
};

typedef union speck_u speck_t;

struct speck_state {
  speck_t round_key[SPECK_MAX_ROUNDS];
  speck_t ctr[SPECK_CTR_SZ];
  uint8_t buffer[SPECK_BUFFER_SZ];

  int offset;
  int has_uint32;
  uint32_t uinteger;
};

typedef struct speck_state speck_state_t;

static inline void advance_counter(speck_state_t *state) {
  uint64_t low;
  int i;
  for (i = 0; i < SPECK_CTR_SZ; i++) {
    low = state->ctr[i].u64[0];
    state->ctr[i].u64[0] += SPECK_CTR_SZ;
    if (state->ctr[i].u64[0] < low)
      state->ctr[i].u64[1]++;
  }
}

#endif
