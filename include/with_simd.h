// control usage of Intel SIMD intrinsics
// by default SIMD intrinsics and assembler code are used on x86_64 architectures
// use -DWITHOUT_SIMD to refrain from using Intel SIMD intrinsics on x86_64 architectures
#if ! defined(WITH_SIMD_CONTROL_DONE)
#define WITH_SIMD_CONTROL_DONE

#if ! defined(WIHOUT_SIMD)
#define WITH_SIMD

// Intel SIMD intrinsics
#if defined(__x86_64__)
#if defined(__AVX2__) || defined(__SSE2__)
#include <immintrin.h>
#endif
#endif

#if ! defined(QUIET_SIMD)
// #pragma message("NOTE: using SIMD intrinsics, use -DNO_SIMD to use pure C code")
#warning "NOTE: using Intel SIMD intrinsics, use -DNO_SIMD to use pure C code"
#endif   // QUIET_SIMD

#endif   // WIHOUT_SIMD

#endif   // WITH_SIMD_CONTROL_DONE
