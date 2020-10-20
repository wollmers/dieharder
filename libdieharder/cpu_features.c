/*
 * Copyright (c) 2019 Kevin Sheppard.
 * Copyright (c) 2020 Reini Urban.
 *
**This software is dual-licensed under the The University of Illinois/NCSA
Open Source License (NCSA) and The 3-Clause BSD License**

# NCSA Open Source License
**Copyright (c) 2019 Kevin Sheppard. All rights reserved.**

Developed by: Kevin Sheppard (<kevin.sheppard@economics.ox.ac.uk>,
<kevin.k.sheppard@gmail.com>)
[http://www.kevinsheppard.com](http://www.kevinsheppard.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimers.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimers in the documentation and/or
other materials provided with the distribution.

Neither the names of Kevin Sheppard, nor the names of any contributors may be
used to endorse or promote products derived from this Software without specific
prior written permission.

**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.**
*/  

#include <stdint.h>
//#include <stdio.h>
#include "cpu_features.h"

uint32_t x86_feature_flags(int major)
{
    uint32_t flag;
#if defined(HAVE_CPUID) && HAVE_CPUID
#if defined(__clang__) || defined(__GNUC__)
    uint32_t num_ids = 0, reg  = 0, eax = 0, ebx = 0, ecx = 0, edx = 0;
    num_ids = __get_cpuid_max(0, &ebx);
    ebx = 0;
    if (num_ids >= 1)
    {
        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    }
#elif defined(_MSC_VER) && defined(_WIN32)
    int cpu_info[4] = {0};
    int num_ids, reg = 0, eax = 0, ebx = 0, ecx = 0, edx = 0;
    __cpuid(cpu_info, 0);
    num_ids = (int)cpu_info[0];
    if (num_ids >= 1)
    {
        __cpuidex(cpu_info, 1, 0);
        eax = cpu_info[0];
        ebx = cpu_info[1];
        ecx = cpu_info[2];
        edx = cpu_info[3];
    }
#endif
#else
    uint32_t eax, ebx, ecx, edx;
    eax = 0; ebx = 0; ecx = 0; edx = 0;
#endif
    switch(major){
    case 0:
      return eax;
    case 1:
      return ebx;
    case 2:
      return ecx;
    case 3:
      return edx;
    default:
      return 0;
    }
}

int is_genuine_intel()
{
#if defined(HAVE_CPUID) && HAVE_CPUID
#if defined(__clang__) || defined(__GNUC__)
    uint32_t num_ids = 0, reg  = 0, eax = 0, ebx = 0, ecx = 0, edx = 0;
    num_ids = __get_cpuid_max(0, &ebx);
    ebx = 0;
    if (num_ids >= 1)
    {
        __get_cpuid(0, &eax, &ebx, &ecx, &edx);

        // printf("cpuid: 0x%x 0x%x 0x%x\n", ebx, ecx, edx);
        // AMD:  0x68747541 0x444d4163 0x69746e65
        if (ebx == 0x756e6547 /*"Genu"*/ &&
            ecx == 0x49656e96 /*"ntel"*/ &&
            edx == 0x6c65746e /*"inel"*/)
          return 1;
    }
    return 0;
#elif defined(_MSC_VER) && defined(_WIN32)
    int cpu_info[4] = {0};
    int num_ids, reg = 0, eax = 0, ebx = 0, ecx = 0, edx = 0;
    __cpuid(cpu_info, 0);
    num_ids = (int)cpu_info[0];
    if (num_ids >= 1)
    {
        __cpuidex(cpu_info, 0, 0);
        eax = cpu_info[0];
        ebx = cpu_info[1];
        ecx = cpu_info[2];
        edx = cpu_info[3];

        if (ebx == 0x756e6547 /*"Genu"*/ &&
            ecx == 0x49656e96 /*"ntel"*/ &&
            edx == 0x6c65746e /*"inel"*/)
          return 1;
    }
    return 0;
#endif
#else
    return -1; // unknown
#endif
}
