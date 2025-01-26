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
//
// Cohen-Daubechies-Feauveau 5/3 wavelets
// https://en.wikipedia.org/wiki/Cohen%E2%80%93Daubechies%E2%80%93Feauveau_wavelet
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
//   transformed data, in place with even/odd split
//   +--------------------------------------------------------+
//   | (n+1)/2 even data            |    (n/2) odd data       |
//   +--------------------------------------------------------+
//
//   original data                     transformed data (2 output arrays)
//   +------------------------------+  +-------------------+  +------------------+
//   |             n data           |  | (n+1)/2 even data |  |   n/2 odd data   |
//   +------------------------------+  +-------------------+  +------------------+
//
//   even data are the "approximation" terms ("low frequency" terms)
//   odd data are the "detail" terms         ("high frequency" terms)
//
// 2 dimensional in place with 2 D split
//   original data                               transformed data (in same array)
//   +------------------------------------+      +-------------------+----------------+
//   |                  ^                 |      +                   |                |
//   |                  |                 |      +   even i/odd j    |  odd i/odd j   |
//   |                  |                 |      +                   |                |
//   |                  |                 |      +                   |                |
//   |                  |                 |      +-------------------+----------------+
//   |               NJ data              |      +                   |                |
//   |                  |                 |      +                   |                |
//   |                  |                 |      +   even i/even j   |  odd i/even j  |
//   |<----- NI data ---|---------------->|      +                   |                |
//   |                  v                 |      +                   |                |
//   +------------------------------------+      +-------------------+----------------+
//   the process can be applied again to the even/even transformed part to achieve a multi level transform
//
#include <stdio.h>

static inline int is_odd(int n) { return (n & 1) ; }

static inline int predict(int o0, int e0, int e1){ return o0 - ((e0 + e1 + 1) >> 1) ; }
static inline int predict_edge(int o0, int e0   ){ return o0 - e0 ; }

static inline int update(int e1, int o0, int o1){ return e1 + ((o0 + o1 + 2) >> 2) ; }
static inline int update_edge(int e1, int o0   ){ return e1 + ((o0 + 1) >> 1) ; }

static inline int un_predict(int o0, int e0, int e1){ return o0 + ((e0 + e1 + 1) >> 1) ; }
static inline int un_predict_edge(int o0, int e0   ){ return o0 + e0 ; }

static inline int un_update(int e1, int o0, int o1){ return e1 - ((o0 + o1 + 2) >> 2) ; }
static inline int un_update_edge(int e1, int o0   ){ return e1 - ((o0 + 1) >> 1) ; }

void fwd_1d_cdf53_split_inplace(int *tmp, int n){
}

void fwd_1d_cdf53(int *tmp, int n){
	if(n < 2) return ;       // fix for small n

	for(int i=1; i<n-2+(n&1); i+=2){     // predict odd
    tmp[i] = predict(tmp[i], tmp[i-1], tmp[i+1]) ;
  }

	if(is_odd(n))
    tmp[n-1] = update_edge(tmp[n-1], tmp[n-2]) ;   // last is even, update
	else
    tmp[n-1] = predict_edge(tmp[n-1], tmp[n-2]) ;  // last is odd, predict

	tmp[0] = update_edge(tmp[0], tmp[1]) ;           // update first even
	for(int i=2; i<n-(n&1); i+=2){
    tmp[i] = update(tmp[i], tmp[i-1], tmp[i+1]) ;  // update even
  }
}

void fwd_1d_cdf53_split_even(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = neven;

  for(i = 0 ; i < nodd-1 ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;  // predict odd terms
  o[nodd-1] = predict_edge(x[n-1], x[n-2]) ;

  e[0 ] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;           // update even terms
}
void fwd_1d_cdf53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;       // predict odd terms

  e[0] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven-1 ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;            // update even terms
  e[neven-1] = update_edge(x[n-1], o[nodd-1]) ;
}

void fwd_1d_cdf53_split(int *x, int *e, int *o, int n){
  if(n < 3) {
    if(n > 0) e[0] = x[0];
    if(n > 1) o[0] = x[1];
    return;
  }
  if(n & 1){
    fwd_1d_cdf53_split_odd(x, e, o, n);
  }else{
    fwd_1d_cdf53_split_even(x, e, o, n);
  }
}

void inv_1d_cdf53_split_inplace(int *tmp, int n){
}

void inv_1d_cdf53(int *tmp, int n){
	if(n < 2) return ;    // fix for small n

	for(int i=2; i<n-(n&1); i+=2){                      // unupdate even
    tmp[i] = un_update(tmp[i], tmp[i-1], tmp[i+1]) ;
  }
  tmp[0] = un_update_edge(tmp[0], tmp[1]) ;           // unupdate first even

	if(is_odd(n))
    tmp[n-1] = un_update_edge(tmp[n-1], tmp[n-2]) ;   // last is even, unupdate
	else
    tmp[n-1] = un_predict_edge(tmp[n-1], tmp[n-2]) ;  // last is odd, unpredict

	for(int i=1; i<n-2+(n&1); i+=2){                    // unpredict odd
    tmp[i] = un_predict(tmp[i], tmp[i-1], tmp[i+1]) ;
  }
}
void inv_1d_cdf53_split_even(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;

  for(i = 0 ; i < neven ; i++){ x[i+i] = e[i] ; x[i+i+1] = o[i] ; }        // move to x

  for (i = 2; i < n; i += 2) x[i] = un_update(x[i], x[i+1], x[i-1]) ;      // unupdate even terms
  x[0] = un_update_edge(x[0], x[1]) ;

  x[n-1] = un_predict_edge(x[n-1], x[n-2]) ;
  for (i = 1; i < n - 2; i += 2) x[i] = un_predict(x[i], x[i-1], x[i+1]) ; // unpredict odd terms
}
void inv_1d_cdf53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++){ x[i+i] = e[i] ; x[i+i+1] = o[i] ; }         // move to x
  x[n-1] = e[neven-1] ;

  x[0] = un_update_edge(x[0], x[1]) ;
  for (i = 2; i < n - 2; i += 2) x[i] = un_update(x[i], x[i+1], x[i-1]) ;  // unupdate even terms
  x[n-1] = un_update_edge(x[n-1], x[n-2]) ;

  for (i = 1; i < n - 1; i += 2) x[i] += ((x[i-1] + x[i+1] + 1) >> 1) ;  // unpredict odd terms
}
void inv_1d_cdf53_split(int *x, int *e, int *o, int n){
  if(n < 3) {   // 3 points minimum
    if(n > 0) x[0] = e[0] ;
    if(n > 1) x[1] = o[0] ;
    return;
  }
  if(n & 1){
    inv_1d_cdf53_split_odd(x, e, o, n);
  }else{
    inv_1d_cdf53_split_even(x, e, o, n);
  }
}
