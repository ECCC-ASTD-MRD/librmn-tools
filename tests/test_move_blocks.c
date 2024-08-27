//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// test the memory block movers
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <rmn/test_helpers.h>
#include <rmn/move_blocks.h>

#undef QUIET_SIMD_
#define NO_SIMD__
#include <rmn/simd_functions.h>
// #if defined(WITH_SIMD)
// #warning "test_move_blocks : using Intel SIMD intrinsics, use -DNO_SIMD to use pure C code"
// #else
// #warning "test_move_blocks: NOT using Intel SIMD intrinsics"
// #endif

// #define MIN08(V,VMI0,V0,VZ) { V8I z=VEQ8(V,V0) ; V=ABS8I(V) ; VZ=ADD8I(VZ,z) ;  V=BLEND8(V,VMI0,z) ;   VMI0=MINU8(V,VMI0) ; }

// typedef struct{ int32_t v[8] ; } V8i ;
// typedef struct{ union{ int32_t i[8] ; uint32_t u[8] ; float    f[8] ; } ; } V8i ;
// typedef struct{ float v[8] ; } V8f ;
typedef struct{ union{ float   f[8] ; int32_t  i[8] ; uint32_t u[8] ; } ; } V8f ;
// typedef struct{ union{ float   f[8] ; int32_t  i[8] ; uint32_t u[8] ; } ; } V8i ;
typedef V8f V8i ;


V8f _mm256_blendv_ps_(V8f A, V8f B, V8f MASK){
  int i ;
  V8f R ;
  for(i=0 ; i<8 ; i++) R.i[i] = (MASK.i[i] < 0) ? B.i[i] : A.i[i] ;
  return R ;
}
static inline V8i _mm256_xor_si256_( V8i A,  V8i B){
  int i ;
  V8i R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] ^ B.i[i] ;
  return R ;
}
static inline V8f _mm256_castsi256_ps_( V8i A){
  int i ;
  V8f R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] ;
  return R ;
}

#define BLEND256(A,B,MASK)      (V8I)_mm256_blendv_ps_((V8F)A, (V8F)B, (V8F)MASK)


// syntax test
static void c_and_prop(/*void *s_, void *d_, uint32_t ni, block_properties *bp*/){
//   int32_t *si = (int32_t *) s_ , *s = si ;
//   int32_t *di = (int32_t *) d_ , *d = di ;
//   int n7 = ni & 7 ;
  V8i v0, v1, v2 ; __m256i vv0, vv1, vv2 ;
  V8f vf, vx, vy, vm ; __m256 vvf, vvx, vvy, vvm ;
  v0 = _mm256_xor_si256_(v0, v0) ;
  v0 = _mm256_xor_si256_(v1, v2) ;
  v0 = (V8i)_mm256_blendv_ps_((V8f)v0, (V8f)v1, (V8f)v2) ;
  v0 = (V8i)_mm256_blendv_ps_(v0, v1, v2) ;
  vv0 = (__m256i) _mm256_blendv_ps((__m256) vv0, vvy, vvm) ;
  vf = _mm256_blendv_ps_(vx, vy, vm) ;
  vf = _mm256_blendv_ps_(_mm256_castsi256_ps_(v0), _mm256_castsi256_ps_(v1), _mm256_castsi256_ps_(v2)) ;
  __m256i i256 ;
  __m256  f256 ;
  f256 = _mm256_castsi256_ps(i256) ;
  vf = _mm256_castsi256_ps_(v0) ;
  f256 =  _mm256_blendv_ps((__m256) i256, (__m256) i256, (__m256) i256) ;
//   V8i vi = (V8i) _mm256_xor_si256_((V8i)vf, (V8i)vf) ;
//   ZERO8(vf, V8F) ;
//   ZERO8I(v0) ;
//   ZERO8I(vzero) ;
//   MASK8I(vmask, n7) ;
//   SET8I(vmii, 0x7FFFFFFF) ;
//   SET8I(vmai, 0x80000000) ;
//   if(n7){
//     MOVE8I(v,s+ 0,d+ 0) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     vzero = AND8I(vmask,vzero) ;
//     ni -= n7 ;
//     s += n7 ;
//     d += n7 ;
//   }
//   while(ni > 32) {
//     MOVE8I(v,s+ 0,d+ 0) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     MOVE8I(v,s+ 8,d+ 8) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     MOVE8I(v,s+16,d+16) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     MOVE8I(v,s+24,d+24) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     ni -= 32 ;
//     s += 32 ;
//     d += 32 ;
//   };
//   uint32_t n4 = ni / 8 ;
//   s = s - (4 - n4) * 8 ;
//   d = d - (4 - n4) * 8 ;
//   switch(n4)
//   {
//     case 4 : MOVE8I(v,s+ 0,d+ 0) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     case 3 : MOVE8I(v,s+ 8,d+ 8) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     case 2 : MOVE8I(v,s+16,d+16) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     case 1 : MOVE8I(v,s+24,d+24) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
//     case 0 :
//     default : ;
//   }
//   FOLD8_IS(F4MAXI, vmai,  &(bp->maxs.i) ) ;    // fold 8 element vector into a scalar
//   FOLD8_IS(F4MINI, vmii,  &(bp->mins.i) ) ;
//   FOLD8_IS(F4MINU, vmiu,  &(bp->min0.u) ) ;
//   FOLD8_IS(F4ADDI, vzero, &(bp->zeros)) ;
}

#define NITER 100
#define WITH_TIMING

#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
  float NaNoSeC = 0.0f ;
#endif

#define NI  128
#define NJ  128
#define LNI 129

int main(int argc, char **argv){
  uint32_t z[NJ*2][LNI], r[NJ*2][LNI] ;
  int i, j, ni, nj, errors ;
  float t0 ;
  TIME_LOOP_DATA ;

  if(cycles_overhead == 0) cycles_overhead = 1;
  if(argc >= 0){
    start_of_test(argv[0]);
  }
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<LNI ; i++){
      z[j][i] = (i << 8) + j ;
    }
  }
//   fprintf(stderr, "original array \n") ;
//   for(j=nj-1 ; j>=0 ; j--){
//     for(i=0 ; i<ni ; i++){
//       fprintf(stderr, "%4.4x ", z[j][i]) ;
//     }
//     fprintf(stderr, "\n") ;
//   }
//   fprintf(stderr, "\n") ;

  ni = 128 ; nj = 127 ;
  {
    uint32_t blk[nj][ni] ;
    for(j=0 ; j<1 ; j++){
      for(i=0 ; i<ni ; i++){
        blk[j][i] = 0xFFFF ;
      }
    }
    get_word_block(z, blk, ni, LNI, nj) ;
    errors = 0 ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        if(blk[j][i] != z[j][i]) {
          if(errors == 0) fprintf(stderr, "(%d,%d) expected %4.4x, got %4.4x\n", i, j, z[j][i], blk[j][i]) ;
          errors++ ;
        }
      }
    }
    fprintf(stderr, "get block errors = %d [%dx%d]\n", errors, ni, nj) ;

    put_word_block(r, blk, ni, LNI, nj) ;
    errors = 0 ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        if(blk[j][i] != r[j][i]) {
          if(errors == 0) fprintf(stderr, "(%d,%d) expected %4.4x, got %4.4x\n", i, j, z[j][i], blk[j][i]) ;
          errors++ ;
        }
      }
    }
    fprintf(stderr, "put block errors = %d [%dx%d]\n", errors, ni, nj) ;

    TIME_LOOP_EZ(NITER, ni*nj, get_word_block(z, blk, ni, LNI, nj) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;

    fprintf(stderr, "get block : %4.2f ns/word\n", t0) ;
    TIME_LOOP_EZ(NITER, ni*nj, put_word_block(z, blk, ni, LNI, nj) ) ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "put block : %4.2f ns/word\n", t0) ;
    if(errors > 0){
      for(j=nj-1 ; j>=0 ; j--){
        for(i=0 ; i<ni ; i++){
          fprintf(stderr, "%4.4x ", blk[j][i]) ;
        }
        fprintf(stderr, "\n") ;
      }
    }
  }
  return 0 ;
}
