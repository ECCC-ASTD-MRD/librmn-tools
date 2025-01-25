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
//   |                  N data                                |
//   +--------------------------------------------------------+
//
//   transformed data (in place, no split, even number of data)
//   +--------------------------------------------------------+
//   |   N data, even/odd, even/odd, ..... , even/odd         +
//   +--------------------------------------------------------+
//
//   transformed data (in place, no split, odd number of data)
//   +--------------------------------------------------------+
//   |   N data, even/odd, even/odd, ..... , even/odd, even   +
//   +--------------------------------------------------------+
//
//   transformed data, in place with even/odd split
//   +--------------------------------------------------------+
//   | (N+1)/2 even data            |    (N/2) odd data       |
//   +--------------------------------------------------------+
//
//   original data                     transformed data (2 output arrays)
//   +------------------------------+  +-------------------+  +------------------+
//   |             N data           |  | (N+1)/2 even data |  |   N/2 odd data   |
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
int is_odd(int n) {
  return (n & 1) ;
}

void fwd_1d_cdf53(int *tmp, int N){
	// fix for small N
	if(N < 2) return;

	// predict 1 + update 1
	for(int i=1; i<N-2+(N&1); i+=2){     // predict odd
		tmp[i] -= (tmp[i-1] + tmp[i+1]) >> 1 ;
  }

	if(is_odd(N))
		tmp[N-1] += (tmp[N-2] + 1) >> 1 ;   // last is even, update
	else
		tmp[N-1] -= tmp[N-2];              // last is odd, predict

	tmp[0] += (tmp[1] + 1) >> 1;         // update even
	for(int i=2; i<N-(N&1); i+=2){
		tmp[i] += ( (tmp[i-1] + tmp[i+1]) + 2 ) >> 2;
  }
}
// #define A      (-0.5f)
// #define B      0.25f
void fwd_1d_cdf53_split_even(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = neven;

  for(i = 0 ; i < nodd-1 ; i++) o[i] = x[i+i+1] - ((x[i+i] + x[i+i+2]) >> 1);  // predict odd terms
  o[nodd-1] = x[n-1] - x[n-2] ;

  e[0 ] = x[0] + ((o[0] + 1) >> 1) ;
  for(i = 1; i < neven ; i++) e[i] = x[i+i] + ((o[i] + o[i-1] + 2) >> 2) ;     // update even terms
}
void fwd_1d_cdf53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++) o[i] = x[i+i+1] - ((x[i+i] + x[i+i+2]) >> 1);    // predict odd terms

  e[0 ] = x[0] + ((o[0] + 1) >> 1) ;
  for(i = 1; i < neven-1 ; i++) e[i] = x[i+i] + ((o[i] + o[i-1] + 2) >> 2) ;   // update even terms
  e[neven-1] = x[n-1] + ((o[nodd-1] + 1) >> 1) ;
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

void inv_1d_cdf53(int *tmp, int N){
	// fix for small N
	if(N < 2) return;

	// backward update 1 + backward predict 1
	for(int i=2; i<N-(N&1); i+=2){       // unupdate even
		tmp[i] -= ( (tmp[i-1] + tmp[i+1]) + 2 ) >> 2;
  }
	tmp[0] -= (tmp[1] + 1) >> 1;

	if(is_odd(N))
		tmp[N-1] -= (tmp[N-2] + 1) >> 1;   // last is even, unupdate
	else
		tmp[N-1] += tmp[N-2];              // last is odd, unpredict

	for(int i=1; i<N-2+(N&1); i+=2){     // unpredict odd
		tmp[i] += ( tmp[i-1] + tmp[i+1] ) >> 1;
  }
}
void inv_1d_cdf53_split_even(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;

  for(i = 0 ; i < neven ; i++){ x[i+i] = e[i] ; x[i+i+1] = o[i] ; }  // move to x

  for (i = 2; i < n; i += 2) x[i] -= ((x[i+1] + x[i-1] + 2) >> 2);   // unupdate even terms
  x[0] -= ((x[1] + 1) >> 1) ;

  x[n - 1] += x[n - 2] ;
  for (i = 1; i < n - 2; i += 2) x[i] += ((x[i-1] + x[i+1]) >> 1) ;  // unpredict odd terms
}
void inv_1d_cdf53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++){ x[i+i] = e[i] ; x[i+i+1] = o[i] ; }   // move to x
  x[n-1] = e[neven-1] ;

  x[0] -= ((x[1] + 1) >> 1) ;                                        // unupdate even terms
  for (i = 2; i < n - 2; i += 2) x[i] -= ((x[i+1] + x[i-1] + 2) >> 2) ;
  x[n - 1] -= ((x[n - 2] + 1) >> 1) ;

  for (i = 1; i < n - 1; i += 2) x[i] += ((x[i-1] + x[i+1]) >> 1) ;  // unpredict odd terms
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
