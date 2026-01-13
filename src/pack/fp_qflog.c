// Hopefully useful code for C
// Copyright (C) 2025  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025

// 32 bit float <--> 32 bit integer fake log

#include <rmn/fp_qflog.h>

// ================================== transform ==================================

// sign magnitude float to rounded and scaled signed integer, order preserving
// both 0.0 and -0.0 come back as 0
// f     : 32 bit float to transform
// nbits : number of significant mantissa bits to keep (0 <= nbits <= 23)
// return fake integer
static inline int32_t fp_to_flog_(float f, int nbits){
  union{ int32_t i ; float f ; } r ;
  int32_t round, sign ;

  r.f = f ;
  round = (0x400000 >> nbits) ;   // rounding term (0 if nbits == 23)
  sign = (r.i >> 31) ;            // 0 or 0xFFFFFFFF (extended sign)
  r.i &= 0x7FFFFFFF ;             // suppress sign
  r.i = (r.i + round) ;           // add rounding term to absolute value
  r.i >>= (23 - nbits) ;          // scale the absolute value, then apply sign
  r.i ^= sign ;                   // no-op if s == 0, negate if s == 0xFFFFFFFF
  r.i -= sign ;                   // complement and add 1 is 2's complement negate
  return r.i ;                    // float represented as a signed integer
}
// int32_t fp_to_flog_1(float f, int nbits){
//   return fp_to_flog_(f, nbits) ;
// }

// convert sign magnitude float to rounded and scaled signed integer, order preserving
// both 0.0 and -0.0 come back as 0
// z      [IN] : float values
// q     [OUT] : transformed integer values
// n      [IN] : number of values
// nbits  [IN] : number of desired significant mantissa bits ( forcing 0 <= nbits <= 23 )
void fp_to_flog(float *z, int32_t *q, int n, int32_t nbits){
  int32_t i ;

  nbits = (nbits <  0) ?  0 : nbits ;         // number of mantissa bits to keep
  nbits = (nbits > 23) ? 23 : nbits ;         // 0 <= nbits <= 23
  for(i=0 ; i<n ; i++){
    q[i] = fp_to_flog_(z[i], nbits) ;
  }
}

// sign magnitude float to rounded and scaled signed integer, order preserving
// both 0.0 and -0.0 come back as 0
// f     : 32 bit float to transform
// nbits : number of less significant bits to eliminate (0 <= nbits <= 23)
// minabs: smallest signicant absolute value
// zval  : any absolute value < minabs gets replaced with zval
// return fake integer
// if the absolute value of an input float is <= minabs, it is forced to zval
// (zval would normally be equal to minabs)
static inline int32_t fp_to_qlog_(float f, int nbits, float minabs, float zval){
  union{ int32_t i ; float f ; } r, m, z ;

  r.f = f ;                       // value to process
  m.f = minabs ;
  m.i &= 0x7F800000 ;             // truncate to power of 2 <= |value|
  z.f = zval   ;
  z.i &= 0x7F800000 ;             // truncate to power of 2 <= |value|
  int32_t round = (1 << nbits) ;  // rounding term (0 if nbits == 0)
  round >>= 1 ;
  int32_t s = (r.i >> 31) ;       // 0 or 0xFFFFFFFF (extended sign)
  r.i &= 0x7FFFFFFF ;             // absolute value of f
  r.i = (r.i + round) ;           // apply rounding term to absolute value
  r.i = (r.i < m.i) ? z.i : r.i ; // replace values below minimum significant value
  r.i >>= nbits ;                 // scale the absolute value, then apply saved sign
  r.i ^= s ;                      // no-op if s == 0, negate if s == 0xFFFFFFFF
  r.i -= s ;                      // complement and add 1 is 2's complement negate
  return r.i ;                    // float represented as a signed integer
}
// int32_t fp_to_qlog_1(float f, int nbits, float minabs, float zval){
//   return fp_to_qlog_(f, nbits, minabs, zval) ;
// }

// convert sign magnitude float to rounded and scaled signed integer, order preserving
// both 0.0 and -0.0 come back as 0
// z      [IN] : float values
// q     [OUT] : transformed integer values
// n      [IN] : number of values
// nbits  [IN] : number of desired significant mantissa bits ( forcing 0 <= nbits <= 23 )
// minabs [IN] : smallest signicant absolute value (will be truncated to power of 2 <= minabs)
// zval   [IN] : replace absolute value < minabs with zval (truncated to power of 2 <= zval)
void fp_to_qlog(float *z, int32_t *q, int n, int32_t nbits, float minabs, float zval){
  int32_t i ;

  if(minabs == 0.0f){
    fp_to_flog(z, q, n, nbits) ;
    return ;
  }

  nbits = (nbits < 0) ? 0 : nbits ;
  nbits = 23 - nbits ;                      // number of mantissa bits to eliminate
  nbits = (nbits < 0) ? 0 : nbits ;
  for(i=0 ; i<n ; i++){
    q[i] = fp_to_qlog_(z[i], nbits, minabs, zval) ;
  }
}

// ================================== restore ==================================

// rounded and scaled signed integer to sign magnitude float, order preserving
// i     : fake integer
// nbits : number of significant bits kept (0 <= nbits <= 23) 
//         (MUST be the same value previously used by fp_to_qlog_)
// return restored float value
// 0 comes back as 0.0f, -0.0f is never produced
static inline float flog_to_fp_(int32_t i, int nbits){
  union{ int32_t i ; float f ; } r ;

  int32_t s = (i >> 31) ;         // 0 or 0xFFFFFFFF (extended sign)
  r.i = i ;                       // absolute value of i
  r.i ^= s ;                      // no-op if s == 0, negate if s == 0xFFFFFFFF
  r.i -= s ;                      // complement and add 1 is 2's complement negate
  r.i <<= (23 - nbits) ;          // scale the absolute value
  r.i |= (s << 31) ;              // apply sign
  return r.f ;                    // restored float
}
float flog_to_fp_1(int32_t i, int nbits){
  return flog_to_fp_(i, nbits) ;
}

// rounded and scaled signed integer to sign magnitude float, order preserving
// z     [OUT] : restored float values
// q      [IN] : integer values
// n      [IN] : number of values
// nbits  [IN] : number of significant mantissa bits kept (MUST BE the same value used for fp_to_qlog)
void flog_to_fp(float *z, int32_t *q, int n, int32_t nbits){
  int32_t i ;

  nbits = (nbits < 0) ? 0 : nbits ;         // number of significant bits kept during quantization
  for(i=0 ; i<n ; i++){
    z[i] = flog_to_fp_(q[i], nbits) ;
  }
}

// rounded and scaled signed integer to sign magnitude float, order preserving
// i     : fake integer
// nbits : number of less significant bits eliminated (0 <= nbits <= 23) 
//         (MUST be the same value previously used by fp_to_qlog)
// minabs: smallest signicant absolute value (should match minabs/zval from fp_to_qlog_)
// return appropriate float value ( minabs with appropriate sign if |value| < minabs )
// 0 comes back as 0.0f, -0.0f is never produced
static inline float qlog_to_fp_(int32_t i, int nbits, float minabs){
  union{ int32_t i ; float f ; } r, m ;

  m.f = minabs ;
  m.i &= 0x7F800000 ;             // truncate to power of 2 <= value
  int32_t s = (i >> 31) ;         // 0 or 0xFFFFFFFF (extended sign)
  r.i = i ;                       // absolute value of i
  r.i ^= s ;                      // no-op if s == 0, negate if s == 0xFFFFFFFF
  r.i -= s ;                      // complement and add 1 is 2's complement negate
  r.i <<= nbits ;                 // unscale the absolute value
  r.i = (r.i < m.i) ? m.i : r.i ; // replace values with |value| < minimum significant value
  r.i |= (s << 31) ;              // restore the sign bit
  return r.f ;                    // restored float
}
// float qlog_to_fp_1(int32_t i, int nbits, float minabs){
//   return qlog_to_fp_(i, nbits, minabs) ;
// }

// rounded and scaled signed integer to sign magnitude float, order preserving
// z     [OUT] : restored float values
// q      [IN] : integer values
// n      [IN] : number of values
// nbits  [IN] : number of significant mantissa bits (MUST BE the same value used for fp2fsi_n)
// minabs [IN] : smallest signicant absolute value (should match minabs/zval from fp_to_qlog_n)
void qlog_to_fp(float *z, int32_t *q, int n, int32_t nbits, float minabs){
  int32_t i ;

  if(minabs == 0.0f){
    flog_to_fp(z, q, n, nbits) ;
    return ;
  }

  nbits = (nbits < 0) ? 0 : nbits ;
  nbits = 23 - nbits ;
  nbits = (nbits < 0) ? 0 : nbits ;         // number of bits eliminated during quantization
  for(i=0 ; i<n ; i++){
    z[i] = qlog_to_fp_(q[i], nbits, minabs) ;
  }
}

// ================================== difference test ==================================
// COMPILE_TEST_CODE is expected to be NOT DEFINED
#if defined(COMPILE_TEST_CODE)
float fp_to_from_qlog(float *f, int n, int nbits, float minabs, float zval){
  int32_t i, q[n] ;
  float maxerr, t, r[n] ;

  fp_to_qlog(f, q, n, nbits, minabs, zval) ;
  qlog_to_fp(r, q, n, nbits, minabs) ;
  maxerr = 0.0f ;
  for(i=0 ; i<n ; i++){
    if(f[i] != 0.0f){          // skip if original data was 0
      t = (r[i] - f[i]) / f[i] ;
      t = (t < 0.0f) ? (-t) : t ;
      maxerr = (t > maxerr) ? t : maxerr ;
    }
  }
  return maxerr ;   // return largest relative error
}

float fp_to_from_flog(float *f, int n, int nbits){
  int32_t i, q[n] ;
  float maxerr, t, r[n] ;

  fp_to_flog(f, q, n, nbits) ;
  flog_to_fp(r, q, n, nbits) ;
  maxerr = 0.0f ;
  for(i=0 ; i<n ; i++){
    if(f[i] != 0.0f){          // skip if original data was 0
      t = (r[i] - f[i]) / f[i] ;
      t = (t < 0.0f) ? (-t) : t ;
      maxerr = (t > maxerr) ? t : maxerr ;
    }
  }
  return maxerr ;   // return largest relative error
}
#endif
