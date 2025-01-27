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

// split array x into even and odd indexes
void split_even_odd(int *x, int *e, int *o, int n){
  int i, neven = (n+1) >> 1, nodd = n >> 1 ;
  for(i = 0 ; i < nodd ; i++){ e[i] = x[i+i] ; o[i] = x[i+i+1] ; }        // move from x
  if(is_odd(n)) e[neven-1] = x[n-1] ;
}

// recompose array x from even and odd indexes
void unsplit_even_odd(int *x, int *e, int *o, int n){
  int i, neven = (n+1) >> 1, nodd = n >> 1 ;
  for(i = 0 ; i < nodd ; i++){ x[i+i] = e[i] ; x[i+i+1] = o[i] ; }        // move to x
  if(is_odd(n)) x[n-1] = e[neven-1] ;
}

// forward LeGall transform, in place, split layout
void fwd_1d_cdf53_split_inplace(int *x, int n){
	if(n < 2) return ;       // nothing to do

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

// forward LeGall transform, in place, split layout, multiple levels
void fwd_1d_cdf53_split_inplace_n(int *x, int n, int levels){
int i ;
  fwd_1d_cdf53_split_inplace(x, n) ;
  if(levels > 0){
    fwd_1d_cdf53_split_inplace_n(x, (n+1)/2, levels -1) ;
  }
}

void fwd_1d_cdf53(int *x, int n){
	if(n < 2) return ;       // nothing to do

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

static void fwd_1d_cdf53_split_even(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = neven;

  for(i = 0 ; i < nodd-1 ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;  // predict odd terms
  o[nodd-1] = predict_edge(x[n-1], x[n-2]) ;

  e[0 ] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;           // update even terms
}
static void fwd_1d_cdf53_split_odd(int *x, int *e, int *o, int n){
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++) o[i] = predict(x[i+i+1], x[i+i], x[i+i+2]) ;       // predict odd terms

  e[0] = update_edge(x[0], o[0]) ;
  for(i = 1; i < neven-1 ; i++) e[i] = update(x[i+i], o[i], o[i-1]) ;            // update even terms
  e[neven-1] = update_edge(x[n-1], o[nodd-1]) ;
}

void fwd_1d_cdf53_split(int *x, int *e, int *o, int n){
  if(n < 3){
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

// predict row o0 using even rows e0 and e1, store in row o
static void row_predict(int *o, int *o0, int *e0, int *e1, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = predict(o0[i], e0[i], e1[i]) ; }
}
static void row_predict_edge(int *o, int *o0, int *e0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = predict_edge(o0[i], e0[i]) ; }
}

// update row e0 using odd rows o0 and o1, store in row e
static void row_update(int *e, int *e0, int *o0, int *o1, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = update(e0[i], o0[i], o1[i]) ; }
}
static void row_update_edge(int *e, int *e0, int *o0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = update_edge(e0[i], o0[i]) ; }
}

static void fwd_2d_cdf53_(int lni, int ni, int nj, int x[nj][lni]){
  int i, j, nio = ni/2, nie = (ni+1)/2 , njo = nj/2, nje = (nj+1)/2 ;
  int o[njo][ni] ;   // local temporary copy of odd terms

  // 1d transform in the i direction, move to temporary array o (x[j+j+1][] : odd rows)
  for(j=0 ; j<njo ; j++){ fwd_1d_cdf53_split(&x[j+j+1][0], &o[j][0], &o[j][nie], ni) ; }

  // 1d transform in the i direction, move to bottom part of array x (x[j+j][] : even rows)
  fwd_1d_cdf53_split_inplace(&x[0][0], ni) ;   // first even row has to be done in place
  for(j=1 ; j<nje ; j++){ fwd_1d_cdf53_split(&x[j+j][0], &x[j][0], &x[j][nie], ni) ; }

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

void fwd_2d_cdf53(int *x, int lni, int ni, int nj){
  fwd_2d_cdf53_(lni, ni, nj, (void *)x) ;
}

void fwd_2d_cdf53_n(int *x, int lni, int ni, int nj, int levels){
  fwd_2d_cdf53_(lni, ni, nj, (void *)x) ;
  if(levels > 0){
    fwd_2d_cdf53_n(x, lni, (ni + 1) / 2, (nj + 1) / 2, levels - 1) ;
  }
}

// inverse LeGall transform, in place, split layout
void inv_1d_cdf53_split_inplace(int *x, int n){
	if(n < 2) return ;    // nothing to do

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

// inverse LeGall transform, in place, split layout, multiple levels
void inv_1d_cdf53_split_inplace_n(int *x, int n, int levels){
  if(levels > 0){
    inv_1d_cdf53_split_inplace_n(x, (n+1)/2, levels-1) ;
  }
  inv_1d_cdf53_split_inplace(x, n) ;
}

void inv_1d_cdf53(int *x, int n){
	if(n < 2) return ;    // nothing to do

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
static void inv_1d_cdf53_split_even(int *x, int *e, int *o, int n){
  int i;

  unsplit_even_odd(x, e, o, n) ;                                           // move to x

  for (i = 2; i < n; i += 2) x[i] = un_update(x[i], x[i+1], x[i-1]) ;      // unupdate even terms
  x[0] = un_update_edge(x[0], x[1]) ;

  x[n-1] = un_predict_edge(x[n-1], x[n-2]) ;
  for (i = 1; i < n - 2; i += 2) x[i] = un_predict(x[i], x[i-1], x[i+1]) ; // unpredict odd terms
}
static void inv_1d_cdf53_split_odd(int *x, int *e, int *o, int n){
  int i;

  unsplit_even_odd(x, e, o, n) ;                                           // move to x

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

// unpredict row o0 using even rows e0 and e1, store in row o
static void row_un_predict(int *o, int *o0, int *e0, int *e1, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = un_predict(o0[i], e0[i], e1[i]) ; }
}
static void row_un_predict_edge(int *o, int *o0, int *e0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ o[i] = un_predict_edge(o0[i], e0[i]) ; }
}

// unupdate row e0 using odd rows o0 and o1, store in row e
static void row_un_update(int *e, int *e0, int *o0, int *o1, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = un_update(e0[i], o0[i], o1[i]) ; }
}
static void row_un_update_edge(int *e, int *e0, int *o0, int ni){
  int i ;
  for(i=0 ; i<ni ; i++){ e[i] = un_update_edge(e0[i], o0[i]) ; }
}

void row_move(int *d, int *s, int n){
  int i ;
  for(i=0 ; i<n ; i++){ d[i] = s[i] ; }
}

static void inv_2d_cdf53_(int lni, int ni, int nj, int x[nj][lni]){
  int i, j, nio = ni/2, nie = (ni+1)/2 , njo = nj/2, nje = (nj+1)/2 ;
  int e[nje][ni] ;   // local temporary copy of even terms

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
    for(j=0 ; j<njo ; j++){ inv_1d_cdf53_split(&x[j+j+1][0], &x[nje+j][0], &x[nje+j][nie], ni) ; }
    // even rows
    for(j=0 ; j<nje ; j++) { inv_1d_cdf53_split(&x[j+j][0], &e[j][0],  &e[j][nie], ni) ; }
  }else{             // last row is odd
    // odd rows
    for(j=0 ; j<njo-1 ; j++){ inv_1d_cdf53_split(&x[j+j+1][0], &x[nje+j][0], &x[nje+j][nie], ni) ; }
    inv_1d_cdf53_split_inplace(&x[nj-1][0], ni) ;    // last odd row
    // even rows
    for(j=0 ; j<nje ; j++) { inv_1d_cdf53_split(&x[j+j][0], &e[j][0],  &e[j][nie], ni) ; }
  }
}

void inv_2d_cdf53(int *x, int lni, int ni, int nj){
  inv_2d_cdf53_(lni, ni, nj, (void *)x) ;
}

void inv_2d_cdf53_n(int *x, int lni, int ni, int nj, int levels){
  if(levels > 0){
    inv_2d_cdf53_n(x, lni, (ni + 1) / 2, (nj + 1) / 2, levels-1) ;
  }
    inv_2d_cdf53_(lni, ni, nj, (void *)x) ;
}
