/*
 * rng_hc128.c
 * Copyright (c) 2013 Lucas Clemente Vella
 * Copyright (c) 2019 Kevin Sheppard. (optimizations)
 * Source code placed into public domain.
 *
 * Copyright (c) 2020 Reini Urban dieharder/gsl adaption
 *
 * HC-128 is a final portfolio cipher of eSTREAM, of the European Network of
 * Excellence for Cryptology (ECRYPT, 2004-2008).
 * The document of HC-128 is available at:
 * 1) Hongjun Wu. ``The Stream Cipher HC-128.'' New Stream Cipher Designs --
 *    The eSTREAM Finalists, LNCS 4986, pp. 39-47, Springer-Verlag, 2008.
 * 2) eSTREAM website:  http://www.ecrypt.eu.org/stream/hcp3.html
 *
 * Taken from the implementation in randomgen, which took parts from
 * https://github.com/lvella/libestream/blob/master/hc-128.h
 */

#include <dieharder/libdieharder.h>

#define HC128_UNROLL 16

typedef struct hc128_state {
  uint32_t p[512];
  uint32_t q[512];
  uint32_t buffer[HC128_UNROLL];
  int hc_idx;
  int buffer_idx;
} hc128_state_t;

static inline uint32_t rotl(uint32_t x, int n) {
#if defined(_MSC_VER)
  return _rotl(x, n);
#else
  return (x << n) | (x >> (32 - n));
#endif
}

#define G1(x, y, z) (rotl(x, 22) ^ rotl(z, 9)) + rotl(y, 24)
#define G2(x, y, z) (rotl(x, 10) ^ rotl(z, 23)) + rotl(y, 8)
#define H(qp, x) qp[x & 0xFFu] + qp[256 + ((x >> 16) & 0xFFu)]

/* 511 mask, for mod 512 */
#define M512(x) (x & 0x1ff)

static inline uint32_t round_expression_pq(uint32_t *pq, const uint32_t *qp,
                                           uint16_t i) {
  pq[i] += G1(pq[M512((i - 3u))], pq[M512((i - 10u))], pq[M512((i + 1u))]);
  return pq[i] ^ (H(qp, pq[M512((i - 12u))]));
}

static inline uint32_t round_expression_qp(uint32_t *pq, const uint32_t *qp,
                                           uint16_t i) {
  pq[i] += G2(pq[M512((i - 3u))], pq[M512((i - 10u))], pq[M512((i + 1u))]);
  return pq[i] ^ (H(qp, pq[M512((i - 12u))]));
}

static inline void hc128_extract_unroll(hc128_state_t *state) {
  uint16_t i = state->hc_idx;
  if (i < 512) {
    state->buffer[0] = round_expression_pq(state->p, state->q, i);
    state->buffer[1] = round_expression_pq(state->p, state->q, i + 1);
    state->buffer[2] = round_expression_pq(state->p, state->q, i + 2);
    state->buffer[3] = round_expression_pq(state->p, state->q, i + 3);
    state->buffer[4] = round_expression_pq(state->p, state->q, i + 4);
    state->buffer[5] = round_expression_pq(state->p, state->q, i + 5);
    state->buffer[6] = round_expression_pq(state->p, state->q, i + 6);
    state->buffer[7] = round_expression_pq(state->p, state->q, i + 7);
    state->buffer[8] = round_expression_pq(state->p, state->q, i + 8);
    state->buffer[9] = round_expression_pq(state->p, state->q, i + 9);
    state->buffer[10] = round_expression_pq(state->p, state->q, i + 10);
    state->buffer[11] = round_expression_pq(state->p, state->q, i + 11);
    state->buffer[12] = round_expression_pq(state->p, state->q, i + 12);
    state->buffer[13] = round_expression_pq(state->p, state->q, i + 13);
    state->buffer[14] = round_expression_pq(state->p, state->q, i + 14);
    state->buffer[15] = round_expression_pq(state->p, state->q, i + 15);
  } else {
    state->buffer[0] = round_expression_qp(state->q, state->p, M512((i)));
    state->buffer[1] = round_expression_qp(state->q, state->p, M512((i + 1)));
    state->buffer[2] = round_expression_qp(state->q, state->p, M512((i + 2)));
    state->buffer[3] = round_expression_qp(state->q, state->p, M512((i + 3)));
    state->buffer[4] = round_expression_qp(state->q, state->p, M512((i + 4)));
    state->buffer[5] = round_expression_qp(state->q, state->p, M512((i + 5)));
    state->buffer[6] = round_expression_qp(state->q, state->p, M512((i + 6)));
    state->buffer[7] = round_expression_qp(state->q, state->p, M512((i + 7)));
    state->buffer[8] = round_expression_qp(state->q, state->p, M512((i + 8)));
    state->buffer[9] = round_expression_qp(state->q, state->p, M512((i + 9)));
    state->buffer[10] = round_expression_qp(state->q, state->p, M512((i + 10)));
    state->buffer[11] = round_expression_qp(state->q, state->p, M512((i + 11)));
    state->buffer[12] = round_expression_qp(state->q, state->p, M512((i + 12)));
    state->buffer[13] = round_expression_qp(state->q, state->p, M512((i + 13)));
    state->buffer[14] = round_expression_qp(state->q, state->p, M512((i + 14)));
    state->buffer[15] = round_expression_qp(state->q, state->p, M512((i + 15)));
  }
  state->buffer_idx = 0;
  state->hc_idx = (state->hc_idx + 16u) & 1023u;
}

static inline uint32_t hc128_next32(hc128_state_t *state) {
  uint32_t out;
  if ((state->buffer_idx % 16) == 0) {
    hc128_extract_unroll(state);
  }
  out = state->buffer[state->buffer_idx];
  state->buffer_idx++;
  return out;
}

static inline uint64_t hc128_next64(hc128_state_t *state) {
  uint64_t out = hc128_next32(state) | ((uint64_t)hc128_next32(state) << 32);
  return out;
}

static inline double hc128_next_double(hc128_state_t *state) {
  return (hc128_next64(state) >> 11) * (1.0 / 9007199254740992.0);
}

static inline uint32_t pack_littleendian(const uint8_t *v)
{
    return *((uint32_t *)v);
}

static inline void unpack_littleendian(uint32_t value, uint8_t *v)
{
    *((uint32_t *)v) = value;
}

/* Init Only */
static inline uint32_t f1(uint32_t x) { return rotl(x, 25) ^ rotl(x, 14) ^ (x >> 3); }
static inline uint32_t f2(uint32_t x) { return rotl(x, 15) ^ rotl(x, 13) ^ (x >> 10); }

/** Initialize HC-128 state with key and IV.
 *
 * Contrary to the other implemented algorithms, the key and IV are taken
 * in a single function to initialize the state. This approach was chosen
 * here because of the nature of the algorithm, that keeps no intermediate
 * state between the key setting and the IV setting.
 *
 * Notice: an IV should never be reused.
 *
 * @param state The uninitialized state, it will be ready to
 * encryption/decryption afterwards.
 * @param key 16 bytes buffer containing the 128-bit key. The buffer must
 * be aligned to at least 4 bytes (depending on the platform it may or may
 * not work with unaligned memory).
 * @param iv 16 bytes buffer containing the IV.
 */
static void hc128_init(hc128_state_t *state, const uint8_t *key, const uint8_t *iv)
{
    unsigned int i;
    uint32_t w[1280];
    uint32_t *p, *q;
    for (i = 0; i < 4; ++i)
    {
        w[i] = w[i + 4] = pack_littleendian(key + 4 * i);
        w[i + 8] = w[i + 12] = pack_littleendian(iv + 4 * i);
    }

    for (i = 16; i < 1280; ++i)
    {
        w[i] = f2(w[i - 2]) + w[i - 7] + f1(w[i - 15]) + w[i - 16] + i;
    }

    p = state->p;
    q = state->q;

    for (i = 0; i < 512; ++i)
    {
        p[i] = w[i + 256];
        q[i] = w[i + 768];
    }

    for (i = 0; i < 512; ++i)
        p[i] = round_expression_pq(p, q, i);

    for (i = 0; i < 512; ++i)
        q[i] = round_expression_qp(q, p, i);

    state->hc_idx = 0;
}

// seed needs to be 16 bytes
static void hc128_seed(hc128_state_t *state, uint32_t *seed)
{
    hc128_init(state, (uint8_t *)&seed[0], (uint8_t *)&seed[4]);
}

static void hc128_set(void *vstate, unsigned long int seed)
{
  hc128_state_t* state = (hc128_state_t*) vstate;
  // Using 2x 128-bit seeds
  uint64_t iv[4];
  uint64_t tmp = (uint64_t)seed;
  for (int i = 0; i < 4; i++) {
    iv[i] = splitmix64_next(&tmp);
  }
  hc128_init(state, (uint8_t *)&iv[0], (uint8_t *)&iv[2]);
}

static unsigned long int hc128_get(void *vstate)
{
  hc128_state_t* state = (hc128_state_t*) vstate;
  return (unsigned long int)hc128_next32(state);
}

static double hc128_get_double (void *vstate)
{
  hc128_state_t* state = (hc128_state_t*) vstate;
  return hc128_next_double(state);
}

static const gsl_rng_type hc128_type =
{"hc-128",
 UINT32_MAX,			/* RAND_MAX */
 0,				/* RAND_MIN */
 sizeof (hc128_state_t),
 &hc128_set,
 &hc128_get,
 &hc128_get_double};

const gsl_rng_type *gsl_rng_hc128 = &hc128_type;
