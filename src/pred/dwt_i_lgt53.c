/* 
 * Copyright (C) 2025  Recherche en Prevision Numerique
 *                     Environnement Canada
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 */
// see
//
// Cohen-Daubechies-Feauveau 5/3 wavelets
// Le GAll / Tabatabai transform
// (reversible, using integers, similar to JPEG 2000)
//
// https://en.wikipedia.org/wiki/Cohen%E2%80%93Daubechies%E2%80%93Feauveau_wavelet
// https://en.wikipedia.org/wiki/Discrete_wavelet_transform
//
// this code is using a lifting implementation
// https://en.wikipedia.org/wiki/Lifting_scheme
//
// 1 dimensional transform, "in place", "even/odd split", or "in place with even/odd split"
//   original data
//   +--------------------------------------------------------+
//   |                  n data                                |
//   +--------------------------------------------------------+
//
//   void fwd_1d_lgt53_asis(int *x, int n);
//   void inv_1d_lgt53_asis(int *x, int n);
//
//   transformed data (in place, no split, even number of data)
//   +--------------------------------------------------------+
//   |   n data, even/odd, even/odd, ..... , even/odd         +
//   +--------------------------------------------------------+
//
//   transformed data (in place, no split, odd number of data)
//   +--------------------------------------------------------+
//   |   n data, even/odd, even/odd, ..... , even/odd, even   +
//   +--------------------------------------------------------+
//
//   void fwd_1d_lgt53(int *x, int n);
//   void inv_1d_lgt53(int *x, int n);
//
//   transformed data, in place with even/odd split
//   +--------------------------------------------------------+
//   | (n+1)/2 even data            |    (n/2) odd data       |
//   +--------------------------------------------------------+
//
//   the process can be applied again to the even transformed part to achieve a multi level transform
//   void fwd_1d_lgt53_n(int *x, int n, int levels);
//   void inv_1d_lgt53_n(int *x, int n, int levels);
//
//   void fwd_1d_lgt53_split(int *x, int *e, int *o, int n);
//   void inv_1d_lgt53_split(int *x, int *e, int *o, int n);
//
//   original data                     transformed data (2 output arrays)
//   +------------------------------+  +-------------------+  +------------------+
//   |             n data           |  | (n+1)/2 even data |  |   n/2 odd data   |
//   +------------------------------+  +-------------------+  +------------------+
//
//   even data are the "approximation" terms ("low frequency" terms)
//   odd data are the "detail" terms         ("high frequency" terms)
//
//   void fwd_2d_lgt53(int *x, int lni, int ni, int nj);
//   void inv_2d_lgt53(int *x, int lni, int ni, int nj);
//
// 2 dimensional in place with 2 D split
//   original data                               transformed data (in same array)
//   +------------------------------------+--+      +-------------------+----------------+--+
//   |                  ^                 |  |      +                   |                |  |
//   |                  |                 |  |      +   even i/odd j    |  odd i/odd j   |  |
//   |                  |                 |  |      +                   |                |  |
//   |                  |                 |  |      +                   |                |  |
//   |                  |                 |  +      +-------------------+----------------+  +
//   |               NJ data              |  |      +                   |                |  |
//   |                  |                 |  |      +                   |                |  |
//   |                  |                 |  |      +   even i/even j   |  odd i/even j  |  |
//   |<----- NI data ---|---------------->|  |      +                   |                |  |
//   |                  v                 |  |      +                   |                |  |
//   +------------------------------------+--+      +-------------------+----------------+--+
//   <---------------- LNI data ------------->      <---------------- LNI data ------------->
//
//   the process can be applied again to the even/even transformed part to achieve a multi level transform
//   void fwd_2d_lgt53_n(int *x, int lni, int ni, int nj, int levels);
//   void inv_2d_lgt53_n(int *x, int lni, int ni, int nj, int levels);
//
#include <stdio.h>
#include <stdint.h>

#include <immintrin.h>

#include <rmn/dwt_i_lgt53.h>

// utility functions used by main functions
static inline int is_odd(int n) { return (n & 1) ; }

#define ROUND 1

// predict odd terms using even terms
static inline int predict(int o0, int e0, int e1){ return o0 - ((e0 + e1 + ROUND) >> 1) ; }
static inline int predict_edge(int o0, int e0   ){ return o0 - e0 ; }   // predict(o0, e0, e0)

// update even terms using odd terms
static inline int update(int e1, int o0, int o1){ return e1 + ((o0 + o1 + 2) >> 2) ; }
static inline int update_edge(int e1, int o0   ){ return e1 + ((o0 + 1) >> 1) ; }  // update(e1, o0, o0) ;

// inverse predict odd terms using even terms
static inline int un_predict(int o0, int e0, int e1){ return o0 + ((e0 + e1 + ROUND) >> 1) ; }
static inline int un_predict_edge(int o0, int e0   ){ return o0 + e0 ; }   // un_predict(o0, e0, e0)

// inverse update even terms using odd terms
static inline int un_update(int e1, int o0, int o1){ return e1 - ((o0 + o1 + 2) >> 2) ; }
static inline int un_update_edge(int e1, int o0   ){ return e1 - ((o0 + 1) >> 1) ; } // un_update(e1, o0, o0) ;

#if defined(__AVX2__)
// even/odd merge 4 even terms + 4 odd terms into 8 terms
static inline __m256i _mm256_merge_128(__m128i ve, __m128i vo){
  return _mm256_setr_m128i( _mm_unpacklo_epi32(ve, vo) , _mm_unpackhi_epi32(ve, vo) ) ;
}
// even/odd merge the low 4 even terms and the low 4 odd terms into 8 terms
static inline __m256i _mm256_merge_lo_128(__m256i ve, __m256i vo){
  __m256i v0 = _mm256_unpacklo_epi32(ve, vo) ;
  __m256i v1 = _mm256_unpackhi_epi32(ve, vo) ;
  return _mm256_permute2x128_si256(v0, v1, 0x20) ;
}
// even/odd merge the high 4 even terms and the high 4 odd terms into 8 terms
static inline __m256i _mm256_merge_hi_128(__m256i ve, __m256i vo){
  __m256i v0 = _mm256_unpacklo_epi32(ve, vo) ;
  __m256i v1 = _mm256_unpackhi_epi32(ve, vo) ;
  return _mm256_permute2x128_si256(v0, v1, 0x31) ;
}
// merge 8 even terms + 8 odd terms and store 16 terms
static inline void merge_store_256(uint32_t *s, __m256i ve, __m256i vo){
  _mm256_storeu_si256((__m256i *)(s  ), _mm256_merge_lo_128(ve, vo)) ;
  _mm256_storeu_si256((__m256i *)(s+8), _mm256_merge_hi_128(ve, vo)) ;
}
// merge 4 even terms + 4 odd terms and store 8 terms
static inline void merge_store_128(uint32_t *s, __m128i ve, __m128i vo){
  _mm256_storeu_si256((__m256i *)(s), _mm256_merge_128(ve, vo) ) ;
}
// merge (n+1)/2 even terms and n/2 odd terms into s[n]
void merge_even_odd_32_simd(uint32_t *s, uint32_t *e, uint32_t *o, int n){
  while(n>15){
    merge_store_256(s, _mm256_loadu_si256((__m256i *)e), _mm256_loadu_si256((__m256i *)o)) ;
    n-=16 ; s+=16 ; e+=8 ; o+=8 ;             // update pointers and count
  }
  if(n>7){
    merge_store_128(s, _mm_loadu_si128((__m128i *)e), _mm_loadu_si128((__m128i *)o)) ;
    n-=8 ; s+=8 ; e+=4 ; o+=4 ;               // update pointers and count
  }
  if(n>0){                                    // any leftovers ?
    __m128i ve, vo, v0 ;
    ve = _mm_loadu_si128((__m128i *)e) ;      // 4 even terms (3 possibly irrelevant)
    vo = _mm_loadu_si128((__m128i *)o) ;      // 4 odd terms (4 possibly irrelevant)
    v0 = _mm_unpacklo_epi32(ve, vo) ;         // merge first 2 pairs
    if(n > 3){
      _mm_storeu_si128((__m128i *)(s  ), v0) ; // store 2 even terms ,  2 odd terms
      v0 = _mm_unpackhi_epi32(ve, vo) ;       // merge next 2 pairs
      n-=4 ; s+=4 ; e+=2 ; o+=2 ;             // update pointers and count
    }
    if(n > 1){
      _mm_storeu_si64((void *)s, v0) ;        // store 1 even term, 1 odd term
      v0 = _mm_bsrli_si128(v0, 8) ;           // shift right by 64 bits
      n-=2 ; s+=2 ; e+=1 ; o+=1 ;             // update pointers and count
    }
    if(n > 0){
      _mm_storeu_si32((void *)s, v0) ;        // store last even term
    }
  }
}
#endif

typedef struct{
  uint32_t e ;
  uint32_t o ;
} even_odd_pair ;

// merge separate even and odd arrays into x array
static void merge_even_odd(void *s_, void *e_, void *o_, int n_){
  uint32_t *s = (uint32_t *) s_, *e = (uint32_t *) e_, *o = (uint32_t *) o_ ;
  int i, n = n_/2 ;
  even_odd_pair *t = (even_odd_pair *) s ;
  for(i=0 ; i<n ; i++) { t[i].e = e[i] ; t[i].o = o[i]; }
  if(n_ & 1) t[n].e = e[n] ;
}

// split array x into separate even and odd arrays
static void split_even_odd(void *s_, void *e_, void *o_, int n_){
  uint32_t *s = (uint32_t *) s_, *e = (uint32_t *) e_, *o = (uint32_t *) o_ ;
  int i, n = n_/2 ;
  even_odd_pair *t = (even_odd_pair *) s ;
  for(i=0 ; i<n ; i++) { e[i] = t[i].e ; o[i] = t[i].o ; }
  if(n_ & 1) e[n] = t[n].e ;
}

// ============================ FORWARD TRANSFORMS ============================

// forward Le Gall Tabatabai transform, in place, split layout
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
void fwd_1d_lgt53(int *x, int n){
  if(n < 2) return ;       // 1 item only, nothing to do

//   int o[nodd], *e = x ;
  int i, neven = (n+1) >> 1, nodd = n >> 1 ;
  int o[nodd], e[neven] ;
  fwd_1d_lgt53_split(x, e, o, n) ;     // use local arrays e and o
  for(i=0 ; i<nodd ; i++){ x[i] = e[i] ; x[neven+i] = o[i] ; }   // copy into x
  x[neven-1] = e[neven-1] ;

//   for(i=0; i<nodd-1 ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;  // predict odd and move to o
//   if(is_odd(n))
//     o[nodd-1] = predict(x[n-2], x[n-1], x[n-3]) ;   // last is even, normal predict
//   else
//     o[nodd-1] = predict_edge(x[n-1], x[n-2]) ;      // last is odd, edge predict
// 
//   e[0] = update_edge(x[0], o[0]) ;                  // update first even
//   for(i=1 ; i<neven-1 ; i++) e[i] = update(x[i+i], o[i-1], o[i]) ;
//   if(is_odd(n))
//     e[i] = update_edge(x[n-1], o[nodd-1]) ;
//   else
//     e[i] = update(x[n-2], o[nodd-1], o[nodd-2]) ;
// 
//   for(i=0 ; i<nodd ; i++) x[neven+i] = o[i] ;       // copy o back into x
}

// forward Le Gall Tabatabai transform, in place, split layout, multiple successive transforms
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
// nl   [IN} : number of successive transforms
void fwd_1d_lgt53_n(int *x, int n, int nl){
  fwd_1d_lgt53(x, n) ;
  if(nl > 0){
    fwd_1d_lgt53_n(x, (n+1)/2, nl -1) ;
  }
}

// forward Le Gall Tabatabai transform, in place, even/odd pairs layout
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
void fwd_1d_lgt53_asis(int *x, int n){
  if(n < 2) return ;       // 1 item only, nothing to do

  for(int i=1; i<n-2+(n&1); i+=2){     // predict odd
    x[i] = predict(x[i], x[i-1], x[i+1]) ;
  }

  if(is_odd(n))
    x[n-1] = update_edge(x[n-1], x[n-2]) ;   // last is even, update
  else
    x[n-1] = predict_edge(x[n-1], x[n-2]) ;  // last is odd, predict

  x[0] = update_edge(x[0], x[1]) ;           // update first even
  for(int i=2; i<n-(n&1); i+=2){
    x[i] = update(x[i], x[i-1], x[i+1]) ;  // update even
  }
}

// 1 dimensional forward Le Gall Tabatabai transform, not in place, even/odd arrays
// x    [IN] : 1D array to transform
// e   [OUT] : 1D array of even terms
// o   [OUT] : 1D array of odd terms
// n    [IN] : dimension of x (assumed even)
void fwd_1d_lgt53_split(int *x, int *e, int *o, int n){
#if defined(__AVX2__)
  fwd_1d_lgt53_split_simd(x, e, o, n) ;
#else
  fwd_1d_lgt53_split_c(x, e, o, n) ;
#endif
}
#if defined(__AVX2__)
void fwd_1d_lgt53_split_simd(int *x_, int *e_, int *o_, int n){
  if(n & 15){          // not a multiple of 16, use C version
    fwd_1d_lgt53_split_c(x_, e_, o_, n) ;
    return ;
  }
  int *x = x_, *e = e_, *o = o_ ;
  int i = 0 ;
  __m256i vc1, vc2 ;
  __m256  ve0, ve1,vd0, vd1, vo0, vo1 ;
  vc1 = _mm256_set1_epi32(1) ;      // vector of 1
  vc2 = _mm256_set1_epi32(2) ;      // vector of 2
//   for(i=0 ; i<n-63 ; i+=64, o+=32, e+=32, x+=64){            // by 64 elements
  for( ; i<n-31 ; i+=32, o+=16, e+=16, x+=32){             // by 32 elements (16 odd/even pairs)
    vd0 = _mm256_loadu_ps((float *)(x   )) ;
    vd1 = _mm256_loadu_ps((float *)(x+ 8)) ;
    ve0 = _mm256_shuffle_ps(vd0, vd1, 136) ;                  // 0b10001000 [ 0 2 8 A 4 6 C E ]
    ve0 = (__m256)_mm256_permute4x64_pd((__m256d)ve0, 216) ;  // 0b11011000 [ 0 2 4 6 8 A C E ]  e[i]
    vd0 = _mm256_loadu_ps((float *)(x+ 1)) ;
    vd1 = _mm256_loadu_ps((float *)(x+ 9)) ;
    vo0 = _mm256_shuffle_ps(vd0, vd1, 136) ;                  // 0b10001000 [ 1 3 9 B 5 7 D F ]
    vo0 = (__m256)_mm256_permute4x64_pd((__m256d)vo0, 216) ;  // 0b11011000 [ 1 3 5 7 9 B D F ]  o[i]
    ve1 = _mm256_shuffle_ps(vd0, vd1, 221) ;                  // 0b10001000 [ 2 4 A C 6 8 E 10]
    ve1 = (__m256)_mm256_permute4x64_pd((__m256d)ve1, 216) ;  // 0b11011000 [ 2 4 6 8 A C E 10]  e[i+1]
    _mm256_storeu_ps((float *)(e   ), ve0) ;

    ve0 = (__m256)_mm256_add_epi32((__m256i)ve0, vc1) ;           // e[i] + 1
    ve0 = (__m256)_mm256_add_epi32((__m256i)ve0, (__m256i)ve1) ;  // e[i] + 1 + e[i+1]
    ve0 = (__m256)_mm256_srai_epi32((__m256i)ve0, 1) ;            // (e[i] + 1 + e[i+1]) >> 1
    vo0 = (__m256)_mm256_sub_epi32((__m256i)vo0, (__m256i)ve0) ;  // o[i] - ( (e[i] + 1 + e[i+1]) >> 1 )
    _mm256_storeu_ps((float *)(o   ), vo0) ;

    vd0 = _mm256_loadu_ps((float *)(x+16)) ;
    vd1 = _mm256_loadu_ps((float *)(x+24)) ;
    ve0 = _mm256_shuffle_ps(vd0, vd1, 136) ;                  // 0b10001000 [ 0 2 8 A 4 6 C E ]
    ve0 = (__m256)_mm256_permute4x64_pd((__m256d)ve0, 216) ;  // 0b11011000 [ 0 2 4 6 8 A C E ]  e[i]
    vd0 = _mm256_loadu_ps((float *)(x+17)) ;
    vd1 = _mm256_loadu_ps((float *)(x+25)) ;
    vo0 = _mm256_shuffle_ps(vd0, vd1, 136) ;                  // 0b10001000 [ 1 3 9 B 5 7 D F ]
    vo0 = (__m256)_mm256_permute4x64_pd((__m256d)vo0, 216) ;  // 0b11011000 [ 1 3 5 7 9 B D F ]  o[i]
    ve1 = _mm256_shuffle_ps(vd0, vd1, 221) ;                  // 0b10001000 [ 2 4 A C 6 8 E 10]
    ve1 = (__m256)_mm256_permute4x64_pd((__m256d)ve1, 216) ;  // 0b11011000 [ 2 4 6 8 A C E 10]  e[i+1]
    _mm256_storeu_ps((float *)(e+ 8), ve0) ;

    ve0 = (__m256)_mm256_add_epi32((__m256i)ve0, vc1) ;           // e[i] + 1
    ve0 = (__m256)_mm256_add_epi32((__m256i)ve0, (__m256i)ve1) ;  // e[i] + 1 + e[i+1]
    ve0 = (__m256)_mm256_srai_epi32((__m256i)ve0, 1) ;            // (e[i] + 1 + e[i+1]) >> 1
    vo0 = (__m256)_mm256_sub_epi32((__m256i)vo0, (__m256i)ve0) ;  // o[i] - ( (e[i] + 1 + e[i+1]) >> 1 )
    _mm256_storeu_ps((float *)(o+ 8), vo0) ;
  }
  for( ; i<n-15 ; i+=16, o+=8, e+=8, x+=16){             // by 16 elements
    vd0 = _mm256_loadu_ps((float *)(x   )) ;
    vd1 = _mm256_loadu_ps((float *)(x+ 8)) ;
    ve0 = _mm256_shuffle_ps(vd0, vd1, 136) ;                  // 0b10001000 [ 0 2 8 A 4 6 C E ]
    ve0 = (__m256)_mm256_permute4x64_pd((__m256d)ve0, 216) ;  // 0b11011000 [ 0 2 4 6 8 A C E ]  e[i]
    vd0 = _mm256_loadu_ps((float *)(x+ 1)) ;
    vd1 = _mm256_loadu_ps((float *)(x+ 9)) ;
    vo0 = _mm256_shuffle_ps(vd0, vd1, 136) ;                  // 0b10001000 [ 1 3 9 B 5 7 D F ]
    vo0 = (__m256)_mm256_permute4x64_pd((__m256d)vo0, 216) ;  // 0b11011000 [ 1 3 5 7 9 B D F ]  o[i]
    ve1 = _mm256_shuffle_ps(vd0, vd1, 221) ;                  // 0b10001000 [ 2 4 A C 6 8 E 10]
    ve1 = (__m256)_mm256_permute4x64_pd((__m256d)ve1, 216) ;  // 0b11011000 [ 2 4 6 8 A C E 10]  e[i+1]
    _mm256_storeu_ps((float *)(e   ), ve0) ;

    ve0 = (__m256)_mm256_add_epi32((__m256i)ve0, vc1) ;           // e[i] + 1
    ve0 = (__m256)_mm256_add_epi32((__m256i)ve0, (__m256i)ve1) ;  // e[i] + 1 + e[i+1]
    ve0 = (__m256)_mm256_srai_epi32((__m256i)ve0, 1) ;            // (e[i] + 1 + e[i+1]) >> 1
    vo0 = (__m256)_mm256_sub_epi32((__m256i)vo0, (__m256i)ve0) ;  // o[i] - ( (e[i] + 1 + e[i+1]) >> 1 )
    _mm256_storeu_ps((float *)(o   ), vo0) ;
  }
  e = e_ ; o = o_ ; x = x_ ;
  o[n/2-1] = x[n-1] - x[n-2] ;                        // predict last odd term

  int e00 = x[0] + ((o[0] + 1) >> 1) ;                // update first even term
//   for(i=0 ; i<n-63 ; i+=64, o+=32, e+=32){            // by 64 elements
  i = 0 ;
  for( ; i<n-31 ; i+=32, o+=16, e+=16){            // by 32 elements
     ve1 = _mm256_loadu_ps((float *)(e   )) ;
     vo0 = _mm256_loadu_ps((float *)(o- 1)) ;
     vo1 = _mm256_loadu_ps((float *)(o   )) ;
     vo0 = (__m256)_mm256_add_epi32((__m256i)vo0, vc2) ;           // o[i-1] + 2
     vo0 = (__m256)_mm256_add_epi32((__m256i)vo0, (__m256i)vo1) ;  // o[i] + o[i-1] + 2
     vo0 = (__m256)_mm256_srai_epi32((__m256i)vo0, 2) ;            // (o[i] + o[i-1] + 2) >> 2
     ve1 = (__m256)_mm256_add_epi32((__m256i)ve1, (__m256i)vo0) ;  // e[i] + ( (o[i] + o[i-1] + 2) >> 2 )
     _mm256_storeu_ps((float *)(e   ), ve1) ;

     ve1 = _mm256_loadu_ps((float *)(e+ 8)) ;
     vo0 = _mm256_loadu_ps((float *)(o+ 7)) ;
     vo1 = _mm256_loadu_ps((float *)(o+ 8)) ;
     vo0 = (__m256)_mm256_add_epi32((__m256i)vo0, vc2) ;           // o[i-1] + 2
     vo0 = (__m256)_mm256_add_epi32((__m256i)vo0, (__m256i)vo1) ;  // o[i] + o[i-1] + 2
     vo0 = (__m256)_mm256_srai_epi32((__m256i)vo0, 2) ;            // (o[i] + o[i-1] + 2) >> 2
     ve1 = (__m256)_mm256_add_epi32((__m256i)ve1, (__m256i)vo0) ;  // e[i] + ( (o[i] + o[i-1] + 2) >> 2 )
     _mm256_storeu_ps((float *)(e+ 8), ve1) ;
  }
  for( ; i<n-15 ; i+=16, o+=8, e+=8){            // by 16 elements
     ve1 = _mm256_loadu_ps((float *)(e   )) ;
     vo0 = _mm256_loadu_ps((float *)(o- 1)) ;
     vo1 = _mm256_loadu_ps((float *)(o   )) ;
     vo0 = (__m256)_mm256_add_epi32((__m256i)vo0, vc2) ;           // o[i-1] + 2
     vo0 = (__m256)_mm256_add_epi32((__m256i)vo0, (__m256i)vo1) ;  // o[i] + o[i-1] + 2
     vo0 = (__m256)_mm256_srai_epi32((__m256i)vo0, 2) ;            // (o[i] + o[i-1] + 2) >> 2
     ve1 = (__m256)_mm256_add_epi32((__m256i)ve1, (__m256i)vo0) ;  // e[i] + ( (o[i] + o[i-1] + 2) >> 2 )
     _mm256_storeu_ps((float *)(e   ), ve1) ;
  }
  e_[0] = e00 ;
}
#endif
void fwd_1d_lgt53_split_c(int *x, int *e, int *o, int n){
  int i ;
  int neven = (n + 1) >> 1, nodd  = n >> 1 ;

  if(n < 3){       // 1 or 2 items, special case
    e[0] = x[0];
    if(n == 2){
      o[0] = predict_edge(x[1], x[0]) ;
      e[0] = update_edge(x[0], o[0]) ;
    }
    return;
  }

  for(i = 0 ; i < nodd ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;  // predict odd terms
  if(neven == nodd) o[nodd-1] = predict_edge(x[n-1], x[n-2]) ;              // last term is odd

  e[0 ] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;         // update even terms
  if(neven != nodd) e[neven-1] = update_edge(x[n-1], o[nodd-1]) ;           // last term is even
}
#if 0
// forward Le Gall Tabatabai transform, not in place, even/odd arrays, odd number of terms
// x    [IN] : 1D array to transform
// e   [OUT] : 1D array of even terms
// o   [OUT] : 1D array of odd terms
// n    [IN] : dimension of x (assumed odd)
void fwd_1d_lgt53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;       // predict odd terms

  e[0] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven-1 ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;            // update even terms
  e[neven-1] = update_edge(x[n-1], o[nodd-1]) ;
}
#endif
#if 0
// forward Le Gall Tabatabai transform, not in place, even/odd arrays
// x    [IN] : 1D array to transform
// e   [OUT] : 1D array of even terms
// o   [OUT] : 1D array of odd terms
// n    [IN] : dimension of x (even or odd)
// if n == 1 explicit action is taken
void fwd_1d_lgt53_split(int *x, int *e, int *o, int n){
  if(n < 2){       // 1 item only, copy even term
    e[0] = x[0];
    return;
  }
  fwd_1d_lgt53_split_c(x, e, o, n);
//   if(n & 1){
//     fwd_1d_lgt53_split_odd(x, e, o, n);
//   }else{
//     fwd_1d_lgt53_split_even(x, e, o, n);
//   }
}
#endif
#if 0
void fwd_1d_lgt53_split_c(int *x, int *e, int *o, int n){
  if(n < 2){       // 1 item only, copy even term
    e[0] = x[0];
    return;
  }
  fwd_1d_lgt53_split_even_c(x, e, o, n);
//   if(n & 1){
//     fwd_1d_lgt53_split_odd(x, e, o, n);
//   }else{
//     fwd_1d_lgt53_split_even_c(x, e, o, n);
//   }
}
#endif
// internal functions used by 2D transform in the j direction
// predict row o0 using even rows e0 and e1, store in row o
// o  [OUT] : 1D array of predicted odd terms
// o0  [IN] : 1D array of odd terms
// e0  [IN] : 1D array of even terms used to predict odd termss
// e1  [IN] : 1D array of even terms used to predict odd termss
static inline void row_predict(int *o, int *o0, int *e0, int *e1, int ni){
  int i = 0 ;
  __m256i vc1 = _mm256_set1_epi32(1) ;
  while(i < ni-7){
    __m256i vro, vo0, ve0, ve1 ;
    vo0 = _mm256_loadu_si256((__m256i *)(o0+i)) ;
    ve0 = _mm256_loadu_si256((__m256i *)(e0+i)) ;
    ve1 = _mm256_loadu_si256((__m256i *)(e1+i)) ;
    ve0 = _mm256_add_epi32(ve0, vc1) ;      // e0[i] + 1
    ve0 = _mm256_add_epi32(ve0, ve1) ;      // e1[i] + e0[i] + 1
    ve0 = _mm256_srai_epi32(ve0, 1) ;       // (e1[i] + e0[i] + 1) >> 1
    vro = _mm256_sub_epi32(vo0, ve0) ;      // (o[i] - (e1[i] + e0[i] + 1) >> 1)
    _mm256_storeu_si256((__m256i *)(o+i), vro) ;
    i += 8 ;
  }
  for( ; i<ni ; i++){ o[i] = o0[i] - ((e0[i] + e1[i] + 1) >> 1) ; }
  if(i != ni){
    fprintf(stderr,"ni = %d, i= %d\n", ni, i);
    exit(1) ;
  }
//   for(i=0 ; i<ni ; i++){ o[i] = o0[i] - ((e0[i] + e1[i] + 1) >> 1) ; }
}
static inline void row_predict_edge(int *o, int *o0, int *e0, int ni){
  int i ;
//   for(i=0 ; i<ni ; i++){ o[i] = predict_edge(o0[i], e0[i]) ; }
  for(i=0 ; i<ni ; i++){ o[i] = o0[i] - e0[i] ; }
}

// update row e0 using odd rows o0 and o1, store in row e
// e  [OUT] : 1D array of updated even terms
// e0  [IN] : 1D array of even terms
// o0  [IN] : 1D array of edd terms used to update even terms
// o1  [IN] : 1D array of odd terms used to update even terms
static inline void row_update(int *e, int *e0, int *o0, int *o1, int ni){
  int i = 0 ;
  __m256i vc2 = _mm256_set1_epi32(2) ;
  while(i < ni-7){
    __m256i vre, ve0, vo0, vo1 ;
    ve0 = _mm256_loadu_si256((__m256i *)(e0+i)) ;
    vo0 = _mm256_loadu_si256((__m256i *)(o0+i)) ;
    vo1 = _mm256_loadu_si256((__m256i *)(o1+i)) ;
    vo0 = _mm256_add_epi32(vo0, vc2) ;      // o0[i] + 2
    vo0 = _mm256_add_epi32(vo0, vo1) ;      // o1[i] + o0[i] + 2
    vo0 = _mm256_srai_epi32(vo0, 2) ;       // (o1[i] + o0[i] + 2) >> 2
    vre = _mm256_add_epi32(ve0, vo0) ;      // (e[i] + (o1[i] + o0[i] + 2) >> 1)
    _mm256_storeu_si256((__m256i *)(e+i), vre) ;
    i += 8 ;
  }
  for( ; i<ni ; i++){ e[i] = e0[i] + ((o0[i] + o1[i] + 2) >> 2) ; }
  if(i != ni){
    fprintf(stderr,"ni = %d, i= %d\n", ni, i);
    exit(1) ;
  }
//   for(i=0 ; i<ni ; i++){ e[i] = e0[i] + ((o0[i] + o1[i] + 2) >> 2) ; }
}
static inline void row_update_edge(int *e, int *e0, int *o0, int ni){
  int i ;
//   for(i=0 ; i<ni ; i++){ e[i] = update_edge(e0[i], o0[i]) ; }
  for(i=0 ; i<ni ; i++){ e[i] = e0[i] + ((o0[i] + 1) >> 1) ; }
}

// used by fwd_2d_lgt53 (VLA form)
static void fwd_2d_lgt53_(int lni, int ni, int nj, int x[nj][lni]){
  int j, nie = (ni+1)/2 , njo = nj/2, nje = (nj+1)/2 ;
  int o[njo][ni] ;   // local temporary copy of odd terms

  if(nj == 1){   // 1 row only, perform 1d transform
// fprintf(stderr,"fwd_2d_lgt53_ %d %d\n", x[0][0], x[0][1]);
    fwd_1d_lgt53(&x[0][0], ni) ;
// fprintf(stderr,"fwd_2d_lgt53_ %d %d\n", x[0][0], x[0][1]);
    return ;
  }

  // 1d transform in the i direction, move to temporary array o (x[j+j+1][] : odd rows)
  for(j=0 ; j<njo ; j++){ fwd_1d_lgt53_split(&x[j+j+1][0], &o[j][0], &o[j][nie], ni) ; }

  // 1d transform in the i direction, move to bottom part of array x (x[j+j][] : even rows)
  fwd_1d_lgt53(&x[0][0], ni) ;   // first even row has to be done in place
  for(j=1 ; j<nje ; j++){ fwd_1d_lgt53_split(&x[j+j][0], &x[j][0], &x[j][nie], ni) ; }

  if(is_odd(nj)){       // last row is even

    // predict odd rows
    for(j=0 ; j<njo   ; j++){ row_predict(&x[nje+j][0], &o[j][0], &x[j][0], &x[j+1][0], ni) ; }
    // update even rows
    row_update_edge(&x[0][0], &x[0][0], &x[nje][0], ni) ;          // first even row
    for(j=1 ; j<nje-1 ; j++){ row_update(&x[j][0], &x[j][0], &x[nje+j-1][0], &x[nje+j][0], ni) ; }
    row_update_edge(&x[j][0], &x[j][0], &x[nje+njo-1][0], ni) ;   // last even row

  }else{                // last row is odd

    // predict odd rows
    for(j=0 ; j<njo-1 ; j++){ row_predict(&x[nje+j][0], &o[j][0], &x[j][0], &x[j+1][0], ni) ; }
    row_predict_edge(&x[nje+j][0], &o[j][0], &x[j][0], ni) ;      // last odd row
    // update even rows
// fprintf(stderr, "row_update_edge\n");
    row_update_edge(&x[0][0], &x[0][0], &x[nje][0], ni) ;          // first even row
    for(j=1 ; j<nje ; j++){ row_update(&x[j][0], &x[j][0], &x[nje+j-1][0], &x[nje+j][0], ni) ; }

  }
}

// in place 2D forward Le Gall Tabatabai transform
// initially array : even/odd terms even/odd rows
// transformed array in quadrant form
// x  [INOUT] : 2D array to transform
// lni   [IN] : storage length of x rows
// ni    [IN] : length of x rows
// nj    [IN] : number of x rows
void fwd_2d_lgt53(int *x, int lni, int ni, int nj){
  fwd_2d_lgt53_(lni, ni, nj, (void *)x) ;  // VLA prototype
}

// in place 2D forward Le Gall Tabatabai multiple successive transform
// initially array : even/odd terms even/odd rows
// transformed array in quadrant form
// x  [INOUT] : 2D array to transform
// lni   [IN] : storage length of x rows
// ni    [IN] : length of x rows
// nj    [IN] : number of x rows
// nl    [IN] : number of successive transforms
void fwd_2d_lgt53_n(int *x, int lni, int ni, int nj, int nl){
  fwd_2d_lgt53_(lni, ni, nj, (void *)x) ;
  if(nl > 0){
    fwd_2d_lgt53_n(x, lni, (ni + 1) / 2, (nj + 1) / 2, nl - 1) ;
  }
}

// ============================ INVERSE TRANSFORMS ============================

// inverse Le Gall Tabatabai transform, in place, split layout
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
void inv_1d_lgt53(int *x, int n){
  if(n < 2) return ;    // 1 item only, nothing to do

  int i, neven = (n+1) >> 1, nodd = n >> 1 ;
  int *o = x+neven, e[neven] ;
  for(i=0 ; i<neven ; i++) e[i] = 777 ;

  e[0] = un_update_edge(x[0], o[0]) ;
  for(i=1 ; i<neven-1 ; i++) e[i] = un_update(x[i], o[i], o[i-1]) ;
  if(is_odd(n))
    e[neven-1] = un_update_edge(x[neven-1], o[nodd-1]) ;
  else
    e[neven-1] = un_update(x[neven-1], o[nodd-1], o[nodd-2]) ;

  for(i=0 ; i<nodd-1 ; i++) x[i+i+1] = un_predict(o[i], e[i], e[i+1]) ;
  if(is_odd(n))
    x[n-2] = un_predict(o[nodd-1], e[neven-1], e[neven-2]) ;
  else
    x[n-1] = un_predict_edge(o[nodd-1], e[neven-1]) ;

  for(i=0 ; i<neven ; i++) x[i+i] = e[i] ;       // copy o back into x
}

// inverse Le Gall Tabatabai transform, in place, split layout, multiple successive transforms
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
// nl   [IN} : number of successive transforms
void inv_1d_lgt53_n(int *x, int n, int nl){
  if(nl > 0){
    inv_1d_lgt53_n(x, (n+1)/2, nl-1) ;
  }
  inv_1d_lgt53(x, n) ;
}

// inverse Le Gall Tabatabai transform, in place, evn/odd pairs
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
void inv_1d_lgt53_asis(int *x, int n){
  if(n < 2) return ;    // 1 item only, nothing to do

  int i ;

  for(i=2; i<n-(n&1); i+=2){                      // unupdate even
    x[i] = un_update(x[i], x[i-1], x[i+1]) ;
  }
  x[0] = un_update_edge(x[0], x[1]) ;           // unupdate first even

  if(is_odd(n))
    x[n-1] = un_update_edge(x[n-1], x[n-2]) ;   // last is even, unupdate
  else
    x[n-1] = un_predict_edge(x[n-1], x[n-2]) ;  // last is odd, unpredict

  for(i=1; i<n-2+(n&1); i+=2){                    // unpredict odd
    x[i] = un_predict(x[i], x[i-1], x[i+1]) ;
  }
}

// inverse Le Gall Tabatabai transform, not in place, even/odd arrays
// x   [OUT] : 1D array to receive transform
// e    [IN] : 1D array of even terms
// o    [IN] : 1D array of odd terms
// n    [IN] : dimension of x (assumed even)
static void inv_1d_lgt53_split_even(int *x, int *e, int *o, int n){
  int i, nodd = n >> 1;

  for (i = 1; i < nodd; i ++) x[i+i] = un_update(e[i], o[i], o[i-1]) ;         // unupdate even terms
  x[0] = un_update_edge(e[0], o[0]) ;

  x[n-1] = un_predict_edge(o[nodd-1], x[n-2]) ;
  for (i = 0; i < nodd-1; i++) x[i+i+1] = un_predict(o[i], x[i+i], x[i+i+2]) ; // unpredict odd terms
}

// inverse Le Gall Tabatabai transform, not in place, even/odd arrays
// x   [OUT] : 1D array to receive transform
// e    [IN] : 1D array of even terms
// o    [IN] : 1D array of odd terms
// n    [IN] : dimension of x (assumed odd)
static void inv_1d_lgt53_split_odd(int *x, int *e, int *o, int n){
  int i;

  merge_even_odd(x, e, o, n) ;                                           // move to x
  x[0] = un_update_edge(x[0], x[1]) ;
  for (i = 2; i < n - 2; i += 2) x[i] = un_update(x[i], x[i+1], x[i-1]) ;  // unupdate even terms
  x[n-1] = un_update_edge(x[n-1], x[n-2]) ;

  for (i = 1; i < n - 1; i += 2) x[i] += ((x[i-1] + x[i+1] + 1) >> 1) ;  // unpredict odd terms
}

// forward Le Gall Tabatabai transform, not in place, even/odd arrays
// x   [OUT] : 1D array to receive transform
// e    [IN] : 1D array of even terms
// o    [IN] : 1D array of odd terms
// n    [IN] : dimension of x (even or odd)
// if n == 1 explicit action is taken
#if defined(__AVX2__)
void inv_1d_lgt53_split_simd(int *x_, int *e_, int *o_, int n){
  int *x = x_, *e = e_, *o = o_ ;
  inv_1d_lgt53_split_c(x, e, o, n) ;   // while debugging the simd version
  if(n & 15) {   // not a multiple of 16
    inv_1d_lgt53_split_c(x, e, o, n) ;
    return;
  }
  int i = 0 ;
  __m256i vc1, vc2 ;
  __m256  ve0, ve1,vd0, vd1, vo0, vo1 ;
  vc1 = _mm256_set1_epi32(1) ;      // vector of 1
  vc2 = _mm256_set1_epi32(2) ;      // vector of 2
  int e00 = e[0] - ((o[0] + 1) >> 1) ;
//   for( ; i<n-31 ; i+=32, o+=16, e+=16, x+=32){             // by 32 elements (16 odd/even pairs)
//   }
  for( ; i<n-15 ; i+=16, o+=8, e+=8, x+=16){               // by 16 elements (8 odd/even pairs)
    ve0 = _mm256_loadu_ps((float *)(e   )) ;
    ve1 = _mm256_loadu_ps((float *)(e+ 8)) ;
    vo0 = _mm256_loadu_ps((float *)(o   )) ;
    ve1 = _mm256_loadu_ps((float *)(o+ 8)) ;
  }

  e = e_ ; o = o_ ; x = x_ ;

}
#endif
void inv_1d_lgt53_split_c(int *x, int *e, int *o, int n){
  if(n < 3) {   // 2 points minimum
    x[0] = e[0] ;
    if(n == 2){
      x[0] = un_update_edge(e[0], o[0]) ;
      x[1] = un_predict_edge(o[0], x[0]) ;
    }
    return;
  }
  if(n & 1){
    inv_1d_lgt53_split_odd(x, e, o, n);
  }else{
    inv_1d_lgt53_split_even(x, e, o, n);
  }
}
void inv_1d_lgt53_split(int *x, int *e, int *o, int n){
#if defined(__AVX2__)
  inv_1d_lgt53_split_simd(x, e, o, n) ;
#else
  inv_1d_lgt53_split_c(x, e, o, n) ;
#endif
}
// internal functions used by 2D inverse transform in the j direction
// unpredict row o0 using even rows e0 and e1, store in row o
// o  [OUT] : 1D array of unpredicted odd terms
// o0  [IN] : 1D array of odd terms
// e0  [IN] : 1D array of even terms used to predict odd termss
// e1  [IN] : 1D array of even terms used to predict odd termss
// ni  [IN] : row length
static void row_un_predict(int *o, int *o0, int *e0, int *e1, int ni){
  int i = 0 ;
//   __m256i vc1 = _mm256_set1_epi32(1) ;
//   while(i < ni-7){
//     __m256i vro, vo0, ve0, ve1 ;
//     vo0 = _mm256_loadu_si256((__m256i *)(o0+i)) ;
//     ve0 = _mm256_loadu_si256((__m256i *)(e0+i)) ;
//     ve1 = _mm256_loadu_si256((__m256i *)(e1+i)) ;
//     ve0 = _mm256_add_epi32(ve0, vc1) ;      // e0[i] + 1
//     ve0 = _mm256_add_epi32(ve0, ve1) ;      // e1[i] + e0[i] + 1
//     ve0 = _mm256_srai_epi32(ve0, 1) ;       // (e1[i] + e0[i] + 1) >> 1
//     vro = _mm256_add_epi32(vo0, ve0) ;      // (o[i] + (e1[i] + e0[i] + 1) >> 1)
//     _mm256_storeu_si256((__m256i *)(o+i), vro) ;
//     i += 8 ;
//   }
//   if(i != ni){
//     fprintf(stderr,"ni = %d, i= %d\n", ni, i);
//     exit(1) ;
//   }
  for(i=0 ; i<ni ; i++){ o[i] = un_predict(o0[i], e0[i], e1[i]) ; }
}
static void row_un_predict_edge(int *o, int *o0, int *e0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = un_predict_edge(o0[i], e0[i]) ; }
}

// unupdate row e0 using odd rows o0 and o1, store in row e
// e  [OUT] : 1D array of unupdated even terms
// e0  [IN] : 1D array of even terms
// o0  [IN] : 1D array of edd terms used to update even terms
// o1  [IN] : 1D array of odd terms used to update even terms
// ni  [IN] : row length
static void row_un_update(int *e, int *e0, int *o0, int *o1, int ni){
  int i = 0 ;
//   __m256i vc2 = _mm256_set1_epi32(2) ;
//   for(i=0 ; i<ni ; i++){ e[i] = un_update(e0[i], o0[i], o1[i]) ; }
//   while(i < ni-7){
//     __m256i vre, ve0, vo0, vo1 ;
//     ve0 = _mm256_loadu_si256((__m256i *)(e0+i)) ;
//     vo0 = _mm256_loadu_si256((__m256i *)(o0+i)) ;
//     vo1 = _mm256_loadu_si256((__m256i *)(o1+i)) ;
//     vo0 = _mm256_add_epi32(vo0, vc2) ;      // o0[i] + 2
//     vo0 = _mm256_add_epi32(vo0, vo1) ;      // o1[i] + o0[i] + 2
//     vo0 = _mm256_srai_epi32(vo0, 2) ;       // (o1[i] + o0[i] + 2) >> 2
//     vre = _mm256_sub_epi32(ve0, vo0) ;      // (e[i] - (o1[i] + o0[i] + 2) >> 1)
//     _mm256_storeu_si256((__m256i *)(e+i), vre) ;
//     i += 8 ;
//   }
//   if(i != ni){
//     fprintf(stderr,"ni = %d, i= %d\n", ni, i);
//     exit(1) ;
//   }
  for(i=0 ; i<ni ; i++){ e[i] = un_update(e0[i], o0[i], o1[i]) ; }
}
static void row_un_update_edge(int *e, int *e0, int *o0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = un_update_edge(e0[i], o0[i]) ; }
}

// used by inv_2d_lgt53 (VLA form)
static void inv_2d_lgt53_(int lni, int ni, int nj, int x[nj][lni]){
  int j, nie = (ni+1)/2 , njo = nj/2, nje = (nj+1)/2 ;
  int e[nje][ni] ;   // local temporary copy of even terms

  if(nj == 1){   // 1 row only, perform 1d inverse transform
// fprintf(stderr,"inv_2d_lgt53_ %d %d\n", x[0][0], x[0][1]);
    inv_1d_lgt53(&x[0][0], ni) ;
// fprintf(stderr,"inv_2d_lgt53_ %d %d\n", x[0][0], x[0][1]);
    return ;
  }
  // unupdate even rows, move to temporary array e
  row_un_update_edge(&e[0][0], &x[0][0], &x[nje][0], ni) ;  // first even row
  if(is_odd(nj)){   // last row is even, nje == njo+1
    // unupdate even rows
    for(j=1 ; j<nje-1 ; j++){ row_un_update(&e[j][0], &x[j][0], &x[nje+j-1][0], &x[nje+j][0], ni) ; }
    row_un_update_edge(&e[j][0], &x[j][0], &x[nje+njo-1][0], ni) ;   // last even row
    // unpredict odd rows
    for(j=0 ; j<njo ; j++) { row_un_predict(&x[nje+j][0], &x[nje+j][0], &e[j+1][0], &e[j][0], ni) ; }
  }else{            // last row is odd, nje == njo
    // unupdate even rows
    for(j=1 ; j<nje ; j++){ row_un_update(&e[j][0], &x[j][0], &x[nje+j-1][0], &x[nje+j][0], ni) ; }
    // unpredict odd rows
    for(j=0 ; j<njo-1 ; j++) { row_un_predict(&x[nje+j][0], &x[nje+j][0], &e[j+1][0], &e[j][0], ni) ; }
    row_un_predict_edge(&x[nje+j][0], &x[nje+j][0], &e[j][0], ni) ;
  }

  // 1d transform in the i direction, move to proper place
  if(is_odd(nj)){    // last row is even
    // odd rows
    for(j=0 ; j<njo ; j++){ inv_1d_lgt53_split(&x[j+j+1][0], &x[nje+j][0], &x[nje+j][nie], ni) ; }
    // even rows
    for(j=0 ; j<nje ; j++) { inv_1d_lgt53_split(&x[j+j][0], &e[j][0],  &e[j][nie], ni) ; }
  }else{             // last row is odd
    // odd rows
    for(j=0 ; j<njo-1 ; j++){ inv_1d_lgt53_split(&x[j+j+1][0], &x[nje+j][0], &x[nje+j][nie], ni) ; }
    inv_1d_lgt53(&x[nj-1][0], ni) ;    // last odd row
    // even rows
    for(j=0 ; j<nje ; j++) { inv_1d_lgt53_split(&x[j+j][0], &e[j][0],  &e[j][nie], ni) ; }
  }
}

// in place 2D inverse Le Gall Tabatabai transform, in place, quadrant layout
// initial array in quadrant form
// transformed array : even/odd terms even/odd rows
// x  [INOUT] : 2D array to transform
// lni   [IN] : storage length of x rows
// ni    [IN] : length of x rows
// nj    [IN] : number of x rows
void inv_2d_lgt53(int *x, int lni, int ni, int nj){
  inv_2d_lgt53_(lni, ni, nj, (void *)x) ;
}

// in place 2D inverse Le Gall Tabatabai transform, in place, quadrant layout,
// multiple successive transforms
// initial array in quadrant form
// transformed array : even/odd terms even/odd rows
// x  [INOUT] : 2D array to transform
// lni   [IN] : storage length of x rows
// ni    [IN] : length of x rows
// nj    [IN] : number of x rows
// nl    [IN] : number of successive transforms
void inv_2d_lgt53_n(int *x, int lni, int ni, int nj, int levels){
  if(levels > 0){
    inv_2d_lgt53_n(x, lni, (ni + 1) / 2, (nj + 1) / 2, levels-1) ;
  }
    inv_2d_lgt53_(lni, ni, nj, (void *)x) ;
}
