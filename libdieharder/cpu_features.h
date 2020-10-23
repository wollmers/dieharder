#ifndef _CPU_FEATURES_H_
#define _CPU_FEATURES_H_

#undef VERSION
#include "config.h"
#include <inttypes.h>

// intel intrinsics. _rdrand64_step
#ifdef HAVE_IMMINTRIN_H
#include <immintrin.h>
#endif
// sse2
#ifdef HAVE_EMMINTRIN_H
#include <emmintrin.h>
#endif
// ssse3: _mm_shuffle_epi8
#ifdef HAVE_TMMINTRIN_H
#include <tmmintrin.h>
#endif
#if defined(_MSC_VER) || defined(HAVE_INTRIN_H)
#include <intrin.h>
#endif

#ifdef __GNUC__
#define ALIGN_GCC_CLANG __attribute__((aligned(16)))
#else
#define ALIGN_GCC_CLANG
#endif
#define ALIGN_WINDOWS

#define DH_EAX 0
#define DH_EBX 1
#define DH_ECX 2
#define DH_EDX 3

#undef HAVE_CPUID
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
#if defined(_MSC_VER) && defined(_WIN32)
#if _MSC_VER >= 1500
#define HAVE_CPUID 1
#endif
#elif defined HAVE_CPUID_H
#define HAVE_CPUID 1
#include <cpuid.h>
#endif
#endif

/* returns
   1 if vendorid is Genuine Intel,
   0 if AMD or other, or
  -1 if unknown, no CPUID insn */
extern int is_genuine_intel();
uint32_t x86_feature_flags(int major);
#define x86_feature_set(flags, i) ((flags) & (1 << (i)))

/* need bswap_64 */

#if defined(__APPLE__)
// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)
#include <sys/byteorder.h>
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)
#include <sys/endian.h>
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)
#include <sys/types.h>
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_64(x) bswap64(x)
#endif

#elif defined (HAVE_BYTESWAP_H)

#include <byteswap.h>

#endif

#endif /* _CPU_FEATURES_H */
