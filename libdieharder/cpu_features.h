#ifndef _CPU_FEATURES_H_
#define _CPU_FEATURES_H_

#undef VERSION
#include "config.h"

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
#if defined(_MSC_VER)
#include <intrin.h>
#endif

#ifdef _WIN32
#define UNLIKELY(x) ((x))
#define LIKELY(x) ((x))
#else
#define UNLIKELY(x) (__builtin_expect((x), 0))
#define LIKELY(x) (__builtin_expect((x), 1))
#endif

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

uint32_t x86_feature_flags(int major);
#define x86_feature_set(flags, i) ((flags) & (1 << (i)))

#endif /* _CPU_FEATURES_H */
