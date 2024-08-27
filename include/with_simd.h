// control usage of Intel SIMD intrinsics
// by default SIMD intrinsics and assembler code are used on x86_64 architectures
// use -DNO_SIMD to refrain from using Intel SIMD intrinsics on x86_64 architectures

// #if ! defined(QUIET_SIMD)
// #if defined(NO_SIMD)
// #warning "with_simd.h : WILL NOT BE using Intel SIMD intrinsics"
// #else
// #warning "with_simd.h : WILL use immintrin.h"
// #endif
// #endif

#if ! defined(WITH_SIMD_CONTROL_DONE)
#define WITH_SIMD_CONTROL_DONE

#if ! defined(NO_SIMD)

// Intel SIMD intrinsics
#if defined(__x86_64__)
#if defined(__AVX2__) || defined(__SSE2__)
#include <immintrin.h>
#endif
#endif
#define WITH_SIMD

#else    // NO_SIMD

#undef WITH_SIMD

#endif   // NO_SIMD

#endif   // WITH_SIMD_CONTROL_DONE

#if ! defined(QUIET_SIMD)
#if defined(WITH_SIMD)
#warning "with_simd.h : using Intel SIMD intrinsics, use -DNO_SIMD to use pure C code"
#else
#warning "with_simd.h : NOT using Intel SIMD intrinsics"
#endif
#endif
