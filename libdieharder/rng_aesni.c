/*
* rng_aesni.c
* Copyright (C) 2019 Daniel Lemire.
* Copyright (C) 2020 Reini Urban gsl/dieharder.
*
* From https://github.com/lemire/testingRNG/blob/master/source/aesctr.h
* Apache License
*/

#undef VERSION
#include "config.h"
#include <dieharder/libdieharder.h>
#include <inttypes.h>
// aesni
#ifdef HAVE_WMMINTRIN_H
#include <wmmintrin.h>
#endif
#include "cpu_features.h"

#if defined(FORCE_SOFTAES) && FORCE_SOFTAES
#undef __AES__
#endif

#if defined(__AES__) && __AES__ && defined(HAVE__MM_AESENC_SI128)
#endif

//#include "softaes.h"
/* softaes.h. Derived from tiny AES */

// The number of columns comprising a state in AES. This is a constant in AES.
// Value=4
#define TINY_Nb 4
#define TINY_Nk 4  // The number of 32 bit words in a key.
#define TINY_Nr 10 // The number of rounds in AES Cipher.

typedef uint8_t state_t[4][4];

static const uint8_t sbox[256] = {
    // 0     1    2      3     4    5     6     7      8    9     A      B    C
    // D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

#define getSBoxValue(num) (sbox[(num)])

// The round constant word array, Rcon[i], contains the values given by
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field
// GF(2^8)
static const uint8_t Rcon[11] = {0x8d, 0x01, 0x02, 0x04, 0x08, 0x10,
                                 0x20, 0x40, 0x80, 0x1b, 0x36};

static inline void tinyaes_expand_key(uint8_t *RoundKey, const uint8_t *Key) {
  unsigned i, j, k;
  uint8_t tempa[4]; // Used for the column/row operations

  // The first round key is the key itself.
  for (i = 0; i < TINY_Nk; ++i) {
    RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
    RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
    RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
    RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
  }

  // All other round keys are found from the previous round keys.
  for (i = TINY_Nk; i < TINY_Nb * (TINY_Nr + 1); ++i) {
    {
      k = (i - 1) * 4;
      tempa[0] = RoundKey[k + 0];
      tempa[1] = RoundKey[k + 1];
      tempa[2] = RoundKey[k + 2];
      tempa[3] = RoundKey[k + 3];
    }

    if (i % TINY_Nk == 0) {
      // This function shifts the 4 bytes in a word to the left once.
      // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

      // Function RotWord()
      {
        const uint8_t u8tmp = tempa[0];
        tempa[0] = tempa[1];
        tempa[1] = tempa[2];
        tempa[2] = tempa[3];
        tempa[3] = u8tmp;
      }

      // SubWord() is a function that takes a four-byte input word and
      // applies the S-box to each of the four bytes to produce an output word.

      // Function Subword()
      {
        tempa[0] = getSBoxValue(tempa[0]);
        tempa[1] = getSBoxValue(tempa[1]);
        tempa[2] = getSBoxValue(tempa[2]);
        tempa[3] = getSBoxValue(tempa[3]);
      }

      tempa[0] = tempa[0] ^ Rcon[i / TINY_Nk];
    }

    j = i * 4;
    k = (i - TINY_Nk) * 4;
    RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
    RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
    RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
    RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
  }
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static inline void SubBytes(state_t *state) {
  uint8_t i, j;
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      (*state)[j][i] = getSBoxValue((*state)[j][i]);
    }
  }
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static inline void ShiftRows(state_t *state) {
  uint8_t temp;

  // Rotate first row 1 columns to left
  temp = (*state)[0][1];
  (*state)[0][1] = (*state)[1][1];
  (*state)[1][1] = (*state)[2][1];
  (*state)[2][1] = (*state)[3][1];
  (*state)[3][1] = temp;

  // Rotate second row 2 columns to left
  temp = (*state)[0][2];
  (*state)[0][2] = (*state)[2][2];
  (*state)[2][2] = temp;

  temp = (*state)[1][2];
  (*state)[1][2] = (*state)[3][2];
  (*state)[3][2] = temp;

  // Rotate third row 3 columns to left
  temp = (*state)[0][3];
  (*state)[0][3] = (*state)[3][3];
  (*state)[3][3] = (*state)[2][3];
  (*state)[2][3] = (*state)[1][3];
  (*state)[1][3] = temp;
}

static inline uint8_t xtime(uint8_t x) {
  return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

// MixColumns function mixes the columns of the state matrix
static inline void MixColumns(state_t *state) {
  uint8_t i;
  uint8_t Tmp, Tm, t;
  for (i = 0; i < 4; ++i) {
    t = (*state)[i][0];
    Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3];
    Tm = (*state)[i][0] ^ (*state)[i][1];
    Tm = xtime(Tm);
    (*state)[i][0] ^= Tm ^ Tmp;
    Tm = (*state)[i][1] ^ (*state)[i][2];
    Tm = xtime(Tm);
    (*state)[i][1] ^= Tm ^ Tmp;
    Tm = (*state)[i][2] ^ (*state)[i][3];
    Tm = xtime(Tm);
    (*state)[i][2] ^= Tm ^ Tmp;
    Tm = (*state)[i][3] ^ t;
    Tm = xtime(Tm);
    (*state)[i][3] ^= Tm ^ Tmp;
  }
}

// This function adds the round key to state.
// The round key is added to the state by an XOR function.
static inline void AddRoundKey(uint8_t round, state_t *state,
                               const uint8_t *RoundKey) {
  uint8_t i, j;
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < 4; ++j) {
      (*state)[i][j] ^= RoundKey[(round * TINY_Nb * 4) + (i * TINY_Nb) + j];
    }
  }
}

// Cipher is the main function that encrypts the PlainText.
static void tiny_encrypt(state_t *state, const uint8_t *RoundKey) {
  uint8_t round = 0;

  // Add the First round key to the state before starting the rounds.
  AddRoundKey(0, state, RoundKey);

  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  for (round = 1; round < TINY_Nr; ++round) {
    SubBytes(state);
    ShiftRows(state);
    MixColumns(state);
    AddRoundKey(round, state, RoundKey);
  }

  // The last round is given below.
  // The MixColumns function is not here in the last round.
  SubBytes(state);
  ShiftRows(state);
  AddRoundKey(TINY_Nr, state, RoundKey);
}


#define AESCTR_UNROLL 4
#define AESCTR_ROUNDS 10

int g_use_aesni;

union aes128_u {
#if defined(__AES__) && __AES__
  __m128i m128;
#endif
  uint64_t u64[2];
  uint32_t u32[4];
  uint8_t u8[16];
};

typedef union aes128_u aes128_t;

struct aesctr_state {
  ALIGN_WINDOWS aes128_t ctr[AESCTR_UNROLL] ALIGN_GCC_CLANG;
  ALIGN_WINDOWS aes128_t seed[AESCTR_ROUNDS + 1] ALIGN_GCC_CLANG;
  ALIGN_WINDOWS uint8_t state[16 * AESCTR_UNROLL] ALIGN_GCC_CLANG;
  size_t offset;
};

typedef struct aesctr_state aesctr_state_t;

static inline uint64_t aesctr_r(aesctr_state_t *state) {
  uint64_t output;
  if (UNLIKELY(state->offset >= 16 * AESCTR_UNROLL)) {
    if (g_use_aesni) {
#if defined(__AES__) && __AES__
      __m128i work[AESCTR_UNROLL];
      for (int i = 0; i < AESCTR_UNROLL; ++i) {
        work[i] = _mm_xor_si128(state->ctr[i].m128, state->seed[0].m128);
      }
      for (int r = 1; r <= AESCTR_ROUNDS - 1; ++r) {
        const __m128i subkey = state->seed[r].m128;
        for (int i = 0; i < AESCTR_UNROLL; ++i) {
          work[i] = _mm_aesenc_si128(work[i], subkey);
        }
      }
      for (int i = 0; i < AESCTR_UNROLL; ++i) {
        state->ctr[i].m128 =
            _mm_add_epi64(state->ctr[i].m128, _mm_set_epi64x(0, AESCTR_UNROLL));
        if (UNLIKELY(state->ctr[i].u64[0] < AESCTR_UNROLL)) {
          /* rolled, add carry */
          state->ctr[i].m128 =
              _mm_add_epi64(state->ctr[i].m128, _mm_set_epi64x(1, 0));
        }
        _mm_storeu_si128(
            (__m128i *)&state->state[16 * i],
            _mm_aesenclast_si128(work[i], state->seed[AESCTR_ROUNDS].m128));
      }
      state->offset = 0;
#endif
    } else {
      int i;
      memcpy(&state->state, &state->ctr, sizeof(state->state));

#ifdef LITTLE_ENDIAN
      uint64_t *block = (uint64_t *)&state->state[0];
      for (i=0; i<(2 * AESCTR_UNROLL);i++){
        block[i] = bswap_64(block[i]);
      }
#endif
      for (i = 0; i < 4; i++) {
        /* On BE, the encrypted data has LE order*/
        tiny_encrypt((state_t *)&state->state[16 * i], (uint8_t *)&state->seed);
      }

      for (i = 0; i < 4; i++) {
        state->ctr[i].u64[0] += AESCTR_UNROLL;
        /* Rolled if less than AESCTR_UNROLL */
        state->ctr[i].u64[1] += (state->ctr[i].u64[0] < AESCTR_UNROLL);
      }
      state->offset = 0;
    }
  }
  output = 0;
  memcpy(&output, &state->state[state->offset], sizeof(output));
  state->offset += sizeof(output);
#ifdef LITTLE_ENDIAN
  /* On BE, the encrypted data has LE order*/
  output = bswap_64(output);
#endif

  return output;
}

static inline uint64_t aes_next64(aesctr_state_t *state) {
  return aesctr_r(state);
}

#define AES_FEATURE_FLAG 25

extern int aesni_capable(void)
{
#if defined(__AES__) && __AES__
    uint32_t flags = x86_feature_flags(DH_ECX);
    g_use_aesni = x86_feature_set(flags, AES_FEATURE_FLAG);
    return g_use_aesni;
#else
    g_use_aesni = 0;
    return 0;
#endif
}

#define AES_ROUND(rcon, index)                                                 \
    do                                                                         \
    {                                                                          \
        __m128i k2 = _mm_aeskeygenassist_si128(k, rcon);                       \
        k = _mm_xor_si128(k, _mm_slli_si128(k, 4));                            \
        k = _mm_xor_si128(k, _mm_slli_si128(k, 4));                            \
        k = _mm_xor_si128(k, _mm_slli_si128(k, 4));                            \
        k = _mm_xor_si128(k, _mm_shuffle_epi32(k2, _MM_SHUFFLE(3, 3, 3, 3)));  \
        state->seed[index].m128 = k;                                           \
    } while (0)

static void aesctr_seed_r(aesctr_state_t *state, uint64_t *seed)
{
    int i;
    /* Call to ensure  g_use_aesni is assigned*/
    aesni_capable();
    /*static const uint8_t rcon[] = {
        0x8d, 0x01, 0x02, 0x04,
        0x08, 0x10, 0x20, 0x40,
        0x80, 0x1b, 0x36
    };*/
    if (g_use_aesni)
    {
#if defined(__AES__) && __AES__
        __m128i k = _mm_set_epi64x(seed[1], seed[0]);
        state->seed[0].m128 = k;
        // D. Lemire manually unrolled following loop since
        // _mm_aeskeygenassist_si128 requires immediates
        /*for(int i = 1; i <= AESCTR_ROUNDS; ++i)
        {
            __m128i k2 = _mm_aeskeygenassist_si128(k, rcon[i]);
            k = _mm_xor_si128(k, _mm_slli_si128(k, 4));
            k = _mm_xor_si128(k, _mm_slli_si128(k, 4));
            k = _mm_xor_si128(k, _mm_slli_si128(k, 4));
            k = _mm_xor_si128(k, _mm_shuffle_epi32(k2, _MM_SHUFFLE(3,3,3,3)));
            state->seed[i] = k;
        }*/
        AES_ROUND(0x01, 1);
        AES_ROUND(0x02, 2);
        AES_ROUND(0x04, 3);
        AES_ROUND(0x08, 4);
        AES_ROUND(0x10, 5);
        AES_ROUND(0x20, 6);
        AES_ROUND(0x40, 7);
        AES_ROUND(0x80, 8);
        AES_ROUND(0x1b, 9);
        AES_ROUND(0x36, 10);
        for (i = 0; i < AESCTR_UNROLL; ++i)
        {
            state->ctr[i].m128 = _mm_set_epi64x(0, i);
        }
#endif
    }
    else
    {
        for (i = 0; i < AESCTR_UNROLL; ++i)
        {
        /* TODO: Counter setting need to be adjusted for BE:
            always store as LE, but do math in math in native
            **might be right, but needs check**
        */

            state->ctr[i].u64[0] = 0;
            state->ctr[i].u64[1] = 0;
            /* Always set first byte to deal with endianness */
            state->ctr[i].u8[0] = i;
        }
#ifdef LITTLE_ENDIAN
        /* Need to apply a byte swap on seed here so that the bytes are the same as LE
           128-bit seed
           We don't do in place to be careful
        */
        uint64_t bwap_seed[2] = {bswap_64(seed[0]), bswap_64(seed[1])};
        seed = (uint64_t *)&bwap_seed;
#endif
        tinyaes_expand_key((uint8_t *)&state->seed, (uint8_t *)seed);
    }
    state->offset = 16 * AESCTR_UNROLL;
}

#undef AES_ROUND

extern void aesctr_use_aesni(int val) { g_use_aesni = val; }

static void aesctr_get_seed_counter(aesctr_state_t *state, uint64_t *seed,
                                    uint64_t *counter)
{
    memcpy(seed, &state->seed, (AESCTR_ROUNDS + 1) * sizeof(aes128_t));
    memcpy(counter, &state->ctr, AESCTR_UNROLL * sizeof(aes128_t));
#ifdef LITTLE_ENDIAN
    /* Need to apply a byte swap on seed here so that the bytes are the same as LE
       128-bit seed
       We don't do in place to be careful
    */
    for (int i=0;i<(2*(AESCTR_ROUNDS + 1));i++){
      seed[i] = bswap_64(seed[i]);
    }
#endif
}

static void aesctr_set_counter(aesctr_state_t *state, uint64_t *counter)
{
    memcpy(&state->ctr, counter, AESCTR_UNROLL * sizeof(aes128_t));
}

static void aesctr_set_seed_counter(aesctr_state_t *state, uint64_t *seed,
                                    uint64_t *counter)
{
#if defined(LITTLE_ENDIAN) && !(LITTLE_ENDIAN)
    /* Need to apply a byte swap on seed here so that the bytes are the same as LE
       128-bit seed
       We don't do in place to be careful
    */
    uint64_t bwap_seed[2*(AESCTR_ROUNDS + 1)];
    for (int i=0;i<(2*(AESCTR_ROUNDS + 1));i++){
      bwap_seed[i] = bswap_64(seed[i]);
    }
    seed = (uint64_t *)&bwap_seed;
#endif

    memcpy(&state->seed, seed, (AESCTR_ROUNDS + 1) * sizeof(aes128_t));
    aesctr_set_counter(state, counter);
}

#if 0
static void aesctr_advance(aesctr_state_t *state, uint64_t *step)
{
    uint64_t low;
    uint64_t temp[2];
    uint64_t adj_step[2];
    size_t new_offset;
    int i;
    if (state->offset == 64)
    {
        /* Force update and reset the offset to simplify */
        aesctr_r(state);
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
    /* TODO: These memcpy are not needed since can use .u64 directly */
    for (i = 0; i < AESCTR_UNROLL; i++)
    {
        memcpy(&temp, &state->ctr[i], sizeof(aes128_t));
        low = temp[0];
        temp[0] += adj_step[0];
        temp[1] += adj_step[1];
        if (temp[0] < low)
        {
            temp[1]++;
        };
        memcpy(&state->ctr[i].u64[0], &temp, sizeof(aes128_t));
    }
    /* Subtract 4 to get previous counter, and regenerate */
    for (i = 0; i < AESCTR_UNROLL; i++)
    {
        memcpy(&temp, &state->ctr[i], sizeof(aes128_t));
        low = temp[0];
        temp[0] -= 4;
        if (temp[0] > low)
        {
            temp[1]--;
        } /* Borrow 1 */
        memcpy(&state->ctr[i].u64[0], &temp, sizeof(aes128_t));
    }
    /* Force update */
    new_offset = state->offset;
    state->offset = 64;
    aesctr_r(state);
    /* Reset the offset */
    state->offset = new_offset;
}
#endif

static void aesni_set(void *vstate, unsigned long int s)
{
  aesctr_state_t* state = (aesctr_state_t*) vstate;
  uint64_t seed[2*(AESCTR_ROUNDS + 1)];
  memset (seed, 0, (AESCTR_ROUNDS + 1) * sizeof(aes128_t));
  memset (state, 0, sizeof(aesctr_state_t));
  seed[0] = (uint64_t)s;
  aesctr_seed_r(state, seed);
}

// aesni_capable checked earlier already
static unsigned long int aesni_get(void *vstate)
{
  aesctr_state_t* state = (aesctr_state_t*) vstate;
  return (unsigned long int)aesctr_r(state);
}

// 64bit only
#define TO_DOUBLE(x)  ((x) >> 11) * 0x1.0p-53
static double aesni_get_double (void *vstate)
{
  return TO_DOUBLE(aesni_get(vstate));
}

static const gsl_rng_type aesni_type =
{"aesni",                       /* name */
 UINT64_MAX,			/* RAND_MAX 64bit only */
 0,				/* RAND_MIN */
 sizeof (aesctr_state_t),
 &aesni_set,
 &aesni_get,
 &aesni_get_double};

const gsl_rng_type *gsl_rng_aesni = &aesni_type;
