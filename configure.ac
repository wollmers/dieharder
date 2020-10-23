#==================================================================
# Process this file with autoconf to produce a configure script.
# Make changes to the version number(s) in particular ONLY here.
#==================================================================

AC_PREREQ(2.59)

#==================================================================
# Be sure to set DIEHARDER_LT_VERSION below at the same time
# you change this:
#==================================================================
AC_INIT([dieharder], [3.31.2beta], [https://github.com/rurban/dieharder/issues/])
AM_INIT_AUTOMAKE([1.14 foreign dist-xz no-installinfo subdir-objects -Wall])
AC_CONFIG_SRCDIR([dieharder.pc.in])
RELEASE=1
AC_SUBST(RELEASE)

AC_CONFIG_HEADERS([config.h])
# AM_MAINTAINER_MODE

#==================================================================
# Library versioning (current:revision:age)
# See the libtool manual for an explanation of the numbers.
# NOTE WELL!  One MUST bump LT_VERSION along with AC_INIT,
# apparently.
#
# dieharder-1.0    libdieharder 0:0:0
#==================================================================
# This command "should" keep libtool up to date.
AC_CONFIG_MACRO_DIR([m4])
# Check if automake supports 'pretty' builds, if so enable them by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CHECK_TOOLS([AR], [gcc-ar ar])
AM_PROG_AR
AC_CHECK_TOOLS([RANLIB], [gcc-ranlib ranlib])
AC_CHECK_TOOLS([NM], [gcc-nm nm])
LT_INIT
AC_SUBST([LIBTOOL_DEPS])
DIEHARDER_LT_VERSION="3:31:2"
AC_SUBST(DIEHARDER_LT_VERSION)
DIEHARDER_LIB_VERSION=3.31.2
AC_SUBST(DIEHARDER_LIB_VERSION)

case "$VERSION" in
    *+)
	AC_DEFINE([RELEASED], [0], [If released or not])
	;;
    *beta)
	AC_DEFINE([RELEASED], [0], [If released or not])
	;;
    *)
	AC_DEFINE([RELEASED], [1], [If released or not])
	;;
esac
AC_SUBST(RELEASED)

#==================================================================
# Check for which system.
#==================================================================
AC_CANONICAL_HOST

#==================================================================
# Checks for programs.
#==================================================================
AC_LANG(C)
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_PROGS([LATEX], [latex])
AM_CONDITIONAL([HAVE_LATEX], [test -n "$LATEX"])

#==================================================================
# Disable unnecessary libtool tests for c++,fortran,java
#==================================================================
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])
define([AC_LIBTOOL_LANG_GCJ_CONFIG], [:])
AC_PROG_LIBTOOL

#==================================================================
# Check compiler features
#==================================================================
AC_TYPE_SIZE_T
AC_C_VOLATILE
AC_C_INLINE

#==================================================================
# Checks for libraries, and headers.  Test for dependency libraries
# FIRST in reverse order that you need -lwhatever to appear on
# compile line as it accumulates libraries to build e.g.
#   -lgsl -lgslcblas
# for the SECOND test, required (in that order) to succeed.
#==================================================================
AC_CHECK_HEADER([gsl/gsl_sf_gamma.h],,[AC_MSG_ERROR([Couldn't find GSL headers.  Please install the gsl-devel package.])])
AC_CHECK_LIB([gslcblas], [main],,[AC_MSG_ERROR([Couldn't find libgsl. Please install the gsl package.])])
AC_CHECK_LIB([gsl],[gsl_sf_gamma])


#==================================================================
# Check if we're a little-endian or a big-endian system, needed by
# brg_endian.h in the build of rng_threefish.  This is a very
# certain test, and therefore is checked FIRST in this header file.
#==================================================================
AC_DEFUN([AC_C_ENDIAN],
[AC_CACHE_CHECK(for endianness, ac_cv_c_endian,
[
  AC_RUN_IFELSE(
    [AC_LANG_PROGRAM([], [dnl
	long val = 1;
	char *c = (char *) &val;
	exit(*c == 1);
    ])
  ],[
    ac_cv_c_endian=big
  ],[
    ac_cv_c_endian=little
  ])
])
if test $ac_cv_c_endian = big; then
  AC_SUBST(LITTLE_ENDIAN,0)
fi
if test $ac_cv_c_endian = little; then
  AC_SUBST(LITTLE_ENDIAN,1)
fi
])

AC_C_ENDIAN

#==================================================================
# Checks for typedefs, structures, and compiler characteristics.
#==================================================================
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(long)
if test x"$ac_cv_sizeof_long" = x"4"; then
  AC_DEFINE([HAVE_32BITLONG], [1],
    [Define if we are on a long = 32bit system.])
  AC_MSG_WARN([skip some 64bit rngs on this long = 32bit system])
  AC_DEFINE([RNG64_MAX], [UINT32_MAX],
    [Define to UINT32_MAX or UINT64_MAX])
  AM_CONDITIONAL([HAVE_32BITLONG], [true])
else
  AC_DEFINE([RNG64_MAX], [UINT64_MAX],
    [Define to UINT32_MAX or UINT64_MAX])
  AM_CONDITIONAL([HAVE_32BITLONG], [false])
fi
AC_HEADER_TIME

AC_CHECK_HEADERS([cpuid.h       \
                  intrin.h      \
                  immintrin.h   \
                  xmmintrin.h   \
                  emmintrin.h   \
                  tmmintrin.h   \
                  wmmintrin.h   \
                  x86intrin.h   \
                  xopintrin.h   \
                  arm_neon.h    \
                  mmintrin.h    \
                  altivec.h     \
                  byteswap.h
                  ])
dnl FIXME arm
CFLAGS="$CFLAGS -march=native"
if test x"$ac_cv_sizeof_long" = x"8"; then
 AC_MSG_CHECKING([for working _rdrand64_step])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[
    #include <immintrin.h>
  ]],
  [[
    unsigned long long p = 0;
    /* Some CPU's might fake it. E.g. AMD Zen2 needs a BIOS patch */
    if (_rdrand64_step(&p) != 1 || p == 0 || p == (unsigned long long)(-1))
      exit(1);
  ]])],[
    ac_cv__rdrand64_step=1
    AC_MSG_RESULT([yes])
  ],[
    ac_cv__rdrand64_step=
    AC_MSG_RESULT([no])
  ])
 if test x$ac_cv__rdrand64_step = x1; then
  AC_DEFINE([HAVE__RDRAND64_STEP], [1],
    [Define if _rdrand64_step() is supported.])
 fi
 AC_MSG_CHECKING([for working _rdseed64_step])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[
    #include <immintrin.h>
  ]],
  [[
    unsigned long long p = 0;
    if (_rdseed64_step(&p) != 1 || p == 0 || p == (unsigned long long)(-1))
      exit(1);
  ]])],[
    ac_cv__rdseed64_step=1
    AC_MSG_RESULT([yes])
  ],[
    AC_MSG_RESULT([no])
    ac_cv__rdseed64_step=
  ])
 if test x$ac_cv__rdseed64_step = x1; then
  AC_DEFINE([HAVE__RDSEED64_STEP], [1],
    [Define if _rdseed64_step() is supported.])
 fi

else

 AC_MSG_CHECKING([for working _rdrand32_step])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[
    #include <immintrin.h>
  ]],
  [[
    unsigned int p = 0;
    /* Some CPU's might fake it. E.g. AMD Zen2 needs a BIOS patch */
    if (_rdrand32_step(&p) != 1 || p == 0 || p == (unsigned)(-1))
      exit(1);
  ]])],[
    ac_cv__rdrand32_step=1
    AC_MSG_RESULT([yes])
  ],[
    ac_cv__rdrand32_step=
    AC_MSG_RESULT([no])
  ])
 if test x$ac_cv__rdrand32_step = x1; then
    AC_DEFINE([HAVE__RDRAND32_STEP], [1],
      [Define if _rdrand32_step() is supported.])
 fi

 AC_MSG_CHECKING([for working _rdseed32_step])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[
    #include <immintrin.h>
  ]],
  [[
    unsigned int p = 0;
    if (_rdseed32_step(&p) != 1 || p == 0 || p == (unsigned)(-1))
      exit(1);
  ]])],[
    ac_cv__rdseed32_step=1
    AC_MSG_RESULT([yes])
  ],[
    ac_cv__rdseed32_step=
    AC_MSG_RESULT([no])
  ])
  if test x$ac_cv__rdseed32_step = x1; then
    AC_DEFINE([HAVE__RDSEED32_STEP], [1],
      [Define if _rdseed32_step() is supported.])
  fi
fi

if test x"$ac_cv_sizeof_long" = x"8"; then
 AC_MSG_CHECKING([for working _mm_aesenc_si128])
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  [[
    #include <wmmintrin.h>
  ]],
  [[
    const __m128i k = _mm_set_epi64x(0UL, 0UL);
    const __m128i work = k, subkey = k;
    if (!__mm_aesenc_si128(work, subkey))
      exit(1);
  ]])],[
    ac_cv__mm_aesenc_si128=1
    AC_MSG_RESULT([yes])
  ],[
    ac_cv__mm_aesenc_si128=
    AC_MSG_RESULT([no])
  ])
 if test x$ac_cv__mm_aesenc_si128 = x1; then
  AC_DEFINE([HAVE__MM_AESENC_SI128], [1],
    [Define if _mm_aesenc_si128() is supported.])
 fi
fi

dnl FIXME GH #1 proper exports, -fno-common -fvisibility=hidden
CFLAGS="$CFLAGS -Wall -Wextra -pedantic -fcommon -fvisibility=default"
DIEHARDER_CFLAGS="-I$includedir"
dieharder_CFLAGS="-std=c99 -I$includedir"
libdieharder_lo_CFLAGS="-I$includedir"
DIEHARDER_LIBS="-L$libdir -ldieharder"
ACLOCAL_AMFLAGS="-I m4"

AC_SUBST(DIEHARDER_CFLAGS)
AC_SUBST(dieharder_CFLAGS)
AC_SUBST(libdieharder_lo_CFLAGS)
AC_SUBST(DIEHARDER_LIBS)
AC_SUBST(ACLOCAL_AMFLAGS)
AM_CONDITIONAL([HAVE_INSRCDIR], [test x"$srcdir" = x"."])

#==================================================================
# Files that configure needs to process with this so that they are
# all happy and macro-replaced.
#==================================================================
AC_CONFIG_FILES([
 dieharder_version.h
 dieharder.spec
 dieharder.html
 dieharder-config
 dieharder.pc
 dieharder/Makefile
 include/Makefile
 libdieharder/Makefile
 manual/Makefile
 manual/macros.tex
 Makefile])

AC_OUTPUT
