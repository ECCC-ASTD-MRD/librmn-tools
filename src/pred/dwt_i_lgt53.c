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

// utility functions used by main functions
static inline int is_odd(int n) { return (n & 1) ; }

#define ROUND 1

// predict odd terms using even terms
static inline int predict(int o0, int e0, int e1){ return o0 - ((e0 + e1 + ROUND) >> 1) ; }
static inline int predict_edge(int o0, int e0   ){ return o0 - e0 ; }

// update even terms using odd terms
static inline int update(int e1, int o0, int o1){ return e1 + ((o0 + o1 + 2) >> 2) ; }
static inline int update_edge(int e1, int o0   ){ return e1 + ((o0 + 1) >> 1) ; }

// inverse predict odd terms using even terms
static inline int un_predict(int o0, int e0, int e1){ return o0 + ((e0 + e1 + ROUND) >> 1) ; }
static inline int un_predict_edge(int o0, int e0   ){ return o0 + e0 ; }

// inverse update even terms using odd terms
static inline int un_update(int e1, int o0, int o1){ return e1 - ((o0 + o1 + 2) >> 2) ; }
static inline int un_update_edge(int e1, int o0   ){ return e1 - ((o0 + 1) >> 1) ; }

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
  if(n & 1) t[i].e = e[n] ;
}

// split array x into separate even and odd arrays
static void split_even_odd(void *s_, void *e_, void *o_, int n_){
  uint32_t *s = (uint32_t *) s_, *e = (uint32_t *) e_, *o = (uint32_t *) o_ ;
  int i, n = n_/2 ;
  even_odd_pair *t = (even_odd_pair *) s ;
  for(i=0 ; i<n ; i++) { e[i] = t[i].e ; o[i] = t[i].o ; }
  if(n & 1) e[n] = t[i].e ;
}

// forward Le Gall Tabatabai transform, in place, split layout
// x [INOUT] : 1D array to transform
// n    [IN] : dimension of x
void fwd_1d_lgt53(int *x, int n){
  if(n < 2) return ;       // 1 item only, nothing to do

  int i, neven = (n+1) >> 1, nodd = n >> 1 ;
  int o[nodd], *e = x ;

  for(i=0; i<nodd-1 ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;  // predict odd and move to o
  if(is_odd(n))
    o[nodd-1] = predict(x[n-2], x[n-1], x[n-3]) ;   // last is even, normal predict
  else
    o[nodd-1] = predict_edge(x[n-1], x[n-2]) ;      // last is odd, edge predict

  e[0] = update_edge(x[0], o[0]) ;                  // update first even
  for(i=1 ; i<neven-1 ; i++) e[i] = update(x[i+i], o[i-1], o[i]) ;
  if(is_odd(n))
    e[i] = update_edge(x[n-1], o[nodd-1]) ;
  else
    e[i] = update(x[n-2], o[nodd-1], o[nodd-2]) ;

  for(i=0 ; i<nodd ; i++) x[neven+i] = o[i] ;       // copy o back into x
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

// forward Le Gall Tabatabai transform, not in place, even/odd arrays
// x    [IN] : 1D array to transform
// e   [OUT] : 1D array of even terms
// o   [OUT] : 1D array of odd terms
// n    [IN] : dimension of x (assumed even)
static void fwd_1d_lgt53_split_even(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = neven;

  for(i = 0 ; i < nodd-1 ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;  // predict odd terms
  o[nodd-1] = predict_edge(x[n-1], x[n-2]) ;

  e[0 ] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;           // update even terms
}

// forward Le Gall Tabatabai transform, not in place, even/odd arrays
// x    [IN] : 1D array to transform
// e   [OUT] : 1D array of even terms
// o   [OUT] : 1D array of odd terms
// n    [IN] : dimension of x (assumed odd)
static void fwd_1d_lgt53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;       // predict odd terms

  e[0] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven-1 ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;            // update even terms
  e[neven-1] = update_edge(x[n-1], o[nodd-1]) ;
}

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
  if(n & 1){
    fwd_1d_lgt53_split_odd(x, e, o, n);
  }else{
    fwd_1d_lgt53_split_even(x, e, o, n);
  }
}

// internal functions used by 2D transform in the j direction
// predict row o0 using even rows e0 and e1, store in row o
// o  [OUT] : 1D array of predicted odd terms
// o0  [IN] : 1D array of odd terms
// e0  [IN] : 1D array of even terms used to predict odd termss
// e1  [IN] : 1D array of even terms used to predict odd termss
static void row_predict(int *o, int *o0, int *e0, int *e1, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = predict(o0[i], e0[i], e1[i]) ; }
}
static void row_predict_edge(int *o, int *o0, int *e0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = predict_edge(o0[i], e0[i]) ; }
}

// update row e0 using odd rows o0 and o1, store in row e
// e  [OUT] : 1D array of updated even terms
// e0  [IN] : 1D array of even terms
// o0  [IN] : 1D array of edd terms used to update even terms
// o1  [IN] : 1D array of odd terms used to update even terms
static void row_update(int *e, int *e0, int *o0, int *o1, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = update(e0[i], o0[i], o1[i]) ; }
}
static void row_update_edge(int *e, int *e0, int *o0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = update_edge(e0[i], o0[i]) ; }
}

// used by fwd_2d_lgt53 (VLA form)
static void fwd_2d_lgt53_(int lni, int ni, int nj, int x[nj][lni]){
  int j, nie = (ni+1)/2 , njo = nj/2, nje = (nj+1)/2 ;
  int o[njo][ni] ;   // local temporary copy of odd terms

  if(nj == 1){   // 1 row only, perform 1d transform
    fwd_1d_lgt53(&x[0][0], ni) ;
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
  fwd_2d_lgt53_(lni, ni, nj, (void *)x) ;
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
  int i;

  merge_even_odd(x, e, o, n) ;                                           // move to x

  for (i = 2; i < n; i += 2) x[i] = un_update(x[i], x[i+1], x[i-1]) ;      // unupdate even terms
  x[0] = un_update_edge(x[0], x[1]) ;

  x[n-1] = un_predict_edge(x[n-1], x[n-2]) ;
  for (i = 1; i < n - 2; i += 2) x[i] = un_predict(x[i], x[i-1], x[i+1]) ; // unpredict odd terms
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
void inv_1d_lgt53_split(int *x, int *e, int *o, int n){
  if(n < 2) {   // 2 points minimum
    x[0] = e[0] ;
    return;
  }
  if(n & 1){
    inv_1d_lgt53_split_odd(x, e, o, n);
  }else{
    inv_1d_lgt53_split_even(x, e, o, n);
  }
}

// internal functions used by 2D inverse transform in the j direction
// unpredict row o0 using even rows e0 and e1, store in row o
// o  [OUT] : 1D array of unpredicted odd terms
// o0  [IN] : 1D array of odd terms
// e0  [IN] : 1D array of even terms used to predict odd termss
// e1  [IN] : 1D array of even terms used to predict odd termss
// ni  [IN] : row length
static void row_un_predict(int *o, int *o0, int *e0, int *e1, int ni){
  int i ;
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
  int i ;
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
    inv_1d_lgt53(&x[0][0], ni) ;
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
