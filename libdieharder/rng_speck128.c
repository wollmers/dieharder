/*
  rng_speck128.c
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
#include "speck-128.h"

// ECX. See https://www.felixcloutier.com/x86/cpuid
#define SSE41_FEATURE_FLAG 19

int g_can_sse41;

static int speck_sse41_capable(void)
{
#if defined(__SSSE3__) && __SSSE3__
    uint32_t flags = x86_feature_flags(DH_ECX);
    g_can_sse41 = x86_feature_set (flags, SSE41_FEATURE_FLAG);
    return g_can_sse41;
#else
    g_can_sse41 = 0;
    return 0;
#endif
}
//static void speck_use_sse41(int val) { g_can_sse41 = val; }

static void speck_seed(speck_state_t *state, uint64_t seed[4])
{
    int i;
    speck_sse41_capable();
    speck_expandkey_128x256(seed, state->round_key);
    state->offset = SPECK_BUFFER_SZ;
    for (i = 0; i < SPECK_CTR_SZ; i++)
    {
        state->ctr[i].u64[0] = i;
        state->ctr[i].u64[1] = 0;
    }
}

static void speck_set_counter(speck_state_t *state, uint64_t *ctr)
{
    int carry, i;
    for (i = 0; i < SPECK_CTR_SZ; i++)
    {
        state->ctr[i].u64[0] = ctr[0] + i;
        carry = state->ctr[i].u64[0] < ctr[0];
        state->ctr[i].u64[1] = ctr[1] + carry;
    }
}

static void speck_advance(speck_state_t *state, uint64_t *step)
{
    uint64_t low;
    uint64_t adj_step[2];
    int new_offset;
    int i;
    if (state->offset == SPECK_BUFFER_SZ)
    {
        /* Force update and reset the offset to simplify */
        speck_next64(state);
        state->offset = 0;
    }
    /* Handle odd with buffer update */
    state->offset = state->offset + 8 * (step[0] % 2);
    adj_step[0] = (step[0] / 2) + ((step[1] % 2) << 63);
    adj_step[1] = (step[1] / 2) + ((step[2] % 2) << 63);
    /* Early return if no counter change */
    if ((adj_step[0] == 0) && (adj_step[1] == 0))
    {
        return;
    }
    /* Update the counters to new **next** values */
    for (i = 0; i < SPECK_CTR_SZ; i++)
    {
        /* Add with carry */
        low = state->ctr[i].u64[0];
        state->ctr[i].u64[0] += adj_step[0];
        state->ctr[i].u64[1] += adj_step[1] + (state->ctr[i].u64[0] < low);
        /* Now subtract to get the previous counter, with carry */
        low = state->ctr[i].u64[0];
        state->ctr[i].u64[0] -= SPECK_CTR_SZ;
        state->ctr[i].u64[1] -= (state->ctr[i].u64[0] > low);
    }
    /* Force update */
    new_offset = state->offset;
    state->offset = SPECK_BUFFER_SZ;
    speck_next64(state);
    /* Reset the offset */
    state->offset = new_offset;
}

static void speck_set(void *vstate, unsigned long int seed)
{
  speck_state_t* state = (speck_state_t*) vstate;
  // Using 256-bit seeds
  uint64_t iv[4];
  uint64_t tmp = (uint64_t)seed;
  for (int i = 0; i < 4; i++) {
    iv[i] = splitmix64_next(&tmp);
  }
  speck_seed(state, iv);
}

static unsigned long int speck_get(void *vstate)
{
  speck_state_t* state = (speck_state_t*) vstate;
  return (unsigned long int)speck_next64(state);
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double speck_get_double (void *vstate)
{
  speck_state_t* state = (speck_state_t*) vstate;
  return TO_DOUBLE(speck_next64(state));
}

static const gsl_rng_type speck_type =
{"speck-128",                       /* name */
 UINT64_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (speck_state_t),
 &speck_set,
 &speck_get,
 &speck_get_double};

const gsl_rng_type *gsl_rng_speck128 = &speck_type;
