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

#include <rmn/simd_compare.h>

#if defined(__AVX512F__)
#define VL 16
#elif defined(__AVX2__)
#define VL 8
#else
#define VL 4
#endif

// increment count[j] when z[i] < ref[j] (0 <= j < 6, 0 <= i < n)
// z        [IN] : array of values
// ref      [IN] : 6 reference values
// count [INOUT] : 6 counts to be incremented
// n        [IN] : number of values
void v_less_than(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
  int32_t vc0[VL], vc1[VL], vc2[VL], vc3[VL], vc4[VL], vc5[VL]  ;     // vector counts
  int32_t kr0[VL], kr1[VL], kr2[VL], kr3[VL], kr4[VL], kr5[VL]  ;     // vector copies of reference values
  int32_t t[VL] ;
  int i, nvl ;

  for(i=0 ; i<VL ; i++) {                           // ref scalar -> vector
    kr0[i] = ref[0] ;
    kr1[i] = ref[1] ;
    kr2[i] = ref[2] ;
    kr3[i] = ref[3] ;
    kr4[i] = ref[4] ;
    kr5[i] = ref[5] ;
  }

  nvl = (n & (VL-1)) ;
  for(i=0   ; i<VL ; i++) { t[i] = z[i] ; }         // first slice modulo(n, VL)
  if(nvl == 0) nvl = VL ;                           // modulo is 0, full slice
  for(i=nvl ; i<VL ; i++) { t[i] = 0x7FFFFFFF ; }   // pad to full vector length
  for(i=0 ; i<VL ; i++) {
    vc0[i] = ((t[i] - kr0[i]) >> 31) ;              // -1 if smaller that reference value
    vc1[i] = ((t[i] - kr1[i]) >> 31) ;
    vc2[i] = ((t[i] - kr2[i]) >> 31) ;
    vc3[i] = ((t[i] - kr3[i]) >> 31) ;
    vc4[i] = ((t[i] - kr4[i]) >> 31) ;
    vc5[i] = ((t[i] - kr5[i]) >> 31) ;
  }
  z += nvl ;                                        // next slice
  n -= nvl ;

  while(n > VL - 1){                                // subsequent slices
    for(i=0 ; i<VL ; i++) {
      vc0[i] = vc0[i] + ((z[i] - kr0[i]) >> 31) ;   // -1 if smaller that reference value
      vc1[i] = vc1[i] + ((z[i] - kr1[i]) >> 31) ;   // 0 otherwise
      vc2[i] = vc2[i] + ((z[i] - kr2[i]) >> 31) ;
      vc3[i] = vc3[i] + ((z[i] - kr3[i]) >> 31) ;
      vc4[i] = vc4[i] + ((z[i] - kr4[i]) >> 31) ;
      vc5[i] = vc5[i] + ((z[i] - kr5[i]) >> 31) ;
    }
    z += VL ;                                       // next slice
    n -= VL ;
  }
  // vector counts are negative, substract from count[]
  for(i=0 ; i<VL ; i++) {   // fold vector counts into count[]
    count[0] -= vc0[i] ;
    count[1] -= vc1[i] ;
    count[2] -= vc2[i] ;
    count[3] -= vc3[i] ;
    count[4] -= vc4[i] ;
    count[5] -= vc5[i] ;
  }
  return;
}
