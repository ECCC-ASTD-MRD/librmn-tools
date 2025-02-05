#include <stdio.h>
#include <rmn/dwt_i_lgt53.h>

static int nerr = 0 ;

static int errors(int *a, int *b, int n){
  int i ;
  for(i=0; i<n ; i++) {
    if(a[i] != b[i]){ nerr++ ;
      fprintf(stderr, "error at i = %d, expected %d, got %d\n", i, a[i], b[i]) ;
    }
  }
  return nerr ;
}

int main(int argc, char **argv){
  (void) argc ;
  (void) argv ;
  int i, j ;
  int tmp[16] = {-1, 11, 2, -23, 3, 32, 1, 0, 10, 1, -22, -3, 44, 5, -66, 8 } ;
  int ref[16] ;
  int sin[16] = {0, 1, 2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1 } ;
  int cos[16] = {2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0, -1, -2, -1, 0, 1 } ;
  int t2d[16][16] ;
  int r2d[16][16] ;
  int e[16], o[16] ;

  for(i=0; i<16 ; i++){ ref[i] = tmp[i] ; }
  for(j=0 ; j<16 ; j++){
    for(i=0; i<16 ; i++){
      t2d[j][i] = r2d[j][i] = sin[i] * cos[j] + 1 ;
//       t2d[j][i] = r2d[j][i] = tmp[j] + i ;
    }
  }

  fprintf(stderr, "original 1D data\n") ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");

  fprintf(stderr, "in place, 1 point\n") ;
  fwd_1d_lgt53_asis(tmp, 1) ;
  for(i=0; i<1 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_asis(tmp, 1) ;
  for(i=0; i<1 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 1));
  if(nerr) goto fail ;

  fprintf(stderr, "in place, 2 points\n") ;
  fwd_1d_lgt53_asis(tmp, 2) ;
  for(i=0; i<2 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_asis(tmp, 2) ;
  for(i=0; i<2 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 2));
  if(nerr) goto fail ;

  fprintf(stderr, "in place, 3 points\n") ;
  fwd_1d_lgt53_asis(tmp, 3) ;
  for(i=0; i<3 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_asis(tmp, 3) ;
  for(i=0; i<3 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 3));
  if(nerr) goto fail ;

  fprintf(stderr, "in place, 4 points\n") ;
  fwd_1d_lgt53_asis(tmp, 4) ;
  for(i=0; i<4 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_asis(tmp, 4) ;
  for(i=0; i<4 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 4));
  if(nerr) goto fail ;

  fprintf(stderr, "in place, even number of points\n") ;
  fwd_1d_lgt53_asis(tmp, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_asis(tmp, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

//   for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  fprintf(stderr, "in place, odd number of points\n") ;
  fwd_1d_lgt53_asis(tmp, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
//   for(i=0; i<7 ; i++){ fprintf(stderr, "%4d ", tmp[i+i+1]) ; } fprintf(stderr, "\n");
//   for(i=0; i<8 ; i++){ fprintf(stderr, "%4d ", tmp[i+i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_asis(tmp, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, 1 point\n") ;
  fwd_1d_lgt53_split(tmp, e, o, 1) ;
  fprintf(stderr, "%4d ", e[0]) ; fprintf(stderr, "\n");
  inv_1d_lgt53_split(tmp, e, o, 1) ;
  for(i=0; i<1 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, 3 points\n") ;
  fwd_1d_lgt53_split(tmp, e, o, 3) ;
  for(i=0; i<1 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } ; fprintf(stderr, "%4d ", e[1]) ; fprintf(stderr, "\n");
  inv_1d_lgt53_split(tmp, e, o, 3) ;
  for(i=0; i<3 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, odd number of points\n") ;
  fwd_1d_lgt53_split(tmp, e, o, 15) ;
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } ; fprintf(stderr, "%4d ", e[7]) ; fprintf(stderr, "\n");
  inv_1d_lgt53_split(tmp, e, o, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, 2 points\n") ;
  for(i=0; i<8 ; i++){ e[i] = o[i] = 8888 ; }
  fwd_1d_lgt53_split(tmp, e, o, 2) ;
  for(i=0; i<1 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_split(tmp, e, o, 2) ;
  for(i=0; i<2 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, 4 points\n") ;
  for(i=0; i<8 ; i++){ e[i] = o[i] = 8888 ; }
  fwd_1d_lgt53_split(tmp, e, o, 4) ;
  for(i=0; i<2 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_split(tmp, e, o, 4) ;
  for(i=0; i<4 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, even number of points\n") ;
  for(i=0; i<8 ; i++){ e[i] = o[i] = 8888 ; }
  fwd_1d_lgt53_split(tmp, e, o, 16) ;
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } fprintf(stderr, "\n");
  inv_1d_lgt53_split(tmp, e, o, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, in place, odd number of points\n") ;
//   fwd_1d_lgt53(tmp, 15);
  fwd_2d_lgt53(tmp, 15, 15, 1);  // 2D transform with nj == 1
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "%4d\n", tmp[7]);
//   inv_1d_lgt53(tmp, 15);
  inv_2d_lgt53(tmp, 15, 15, 1);  // 2D transform with nj == 1
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, in place, even number of points\n") ;
//   fwd_1d_lgt53(tmp, 16);
  fwd_2d_lgt53(tmp, 16, 16, 1);  // 2D transform with nj == 1
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "\n");
//   inv_1d_lgt53(tmp, 16);
  inv_2d_lgt53(tmp, 16, 16, 1);  // 2D transform with nj == 1
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, even number of points along j, ni == 1\n") ;
  fwd_2d_lgt53(tmp, 1, 1, 16);
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "\n");
  inv_2d_lgt53(tmp, 1, 1, 16);
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, odd number of points along j, ni == 1\n") ;
  fwd_2d_lgt53(tmp, 1, 1, 15);
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "%4d\n", tmp[7]);
  inv_2d_lgt53(tmp, 1, 1, 15);
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, in place, even number of points, 2 levels\n") ;
//   fwd_1d_lgt53_n(tmp, 16, 2);
  fwd_2d_lgt53_n(tmp, 16, 16, 1, 2);  // 2D transform with nj == 1
//   inv_1d_lgt53_n(tmp, 16, 2);
  inv_2d_lgt53_n(tmp, 16, 16, 1, 2);  // 2D transform with nj == 1
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "split, in place, odd number of points, 2 levels\n") ;
//   fwd_1d_lgt53_n(tmp, 15, 2);
  fwd_2d_lgt53_n(tmp, 15, 15, 1, 2);  // 2D transform with nj == 1
//   inv_1d_lgt53_n(tmp, 15, 2);
  inv_2d_lgt53_n(tmp, 15, 15, 1, 2);  // 2D transform with nj == 1
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, even number of points along j, ni == 1, 3 levels\n") ;
  fwd_2d_lgt53_n(tmp, 1, 1, 16, 3);
  inv_2d_lgt53_n(tmp, 1, 1, 16, 3);
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, even number of points along j, ni == 3") ;
  fwd_2d_lgt53((void *)t2d, 16, 3, 16);
  inv_2d_lgt53((void *)t2d, 16, 3, 16);
  fprintf(stderr, " : errors = %d\n\n", errors((void *)r2d, (void *)t2d, 16*16));
  if(nerr) goto fail ;

int nrows = 2, ncols = 2 ;
  fprintf(stderr, "2D, %d points along j, ni == %d", nrows, ncols) ;
// for(j=nrows ; j>=0 ; j--){
//   for(i=0 ; i<8 ; i++){
//     fprintf(stderr, "%4d ",t2d[j][i]);
//   }
//   fprintf(stderr, "\n");
// }
// fprintf(stderr, "\n");
  fwd_2d_lgt53((void *)t2d, 16, ncols, nrows);
// for(j=nrows ; j>=0 ; j--){
//   for(i=0 ; i<8 ; i++){
//     fprintf(stderr, "%4d ",t2d[j][i]);
//   }
//   fprintf(stderr, "\n");
// }
// fprintf(stderr, "\n");
  inv_2d_lgt53((void *)t2d, 16, ncols, nrows);
// for(j=nrows ; j>=0 ; j--){
//   for(i=0 ; i<8 ; i++){
//     fprintf(stderr, "%4d ",t2d[j][i]);
//   }
//   fprintf(stderr, "\n");
// }
  fprintf(stderr, " : errors = %d\n\n", errors((void *)r2d, (void *)t2d, 16*16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, even number of points along i and j") ;
  fwd_2d_lgt53((void *)t2d, 16, 16, 16) ;
  inv_2d_lgt53((void *)t2d, 16, 16, 16) ;
  fprintf(stderr, " : errors = %d\n\n", errors((void *)r2d, (void *)t2d, 16*16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, odd number of points along j, ni == 2") ;
  fwd_2d_lgt53((void *)t2d, 16, 2, 15);
  inv_2d_lgt53((void *)t2d, 16, 2, 15);
  fprintf(stderr, " : errors = %d\n\n", errors((void *)r2d, (void *)t2d, 16*16));
  if(nerr) goto fail ;

  fprintf(stderr, "2D, odd number of points along i and j") ;
  fwd_2d_lgt53((void *)t2d, 16, 15, 15) ;
  inv_2d_lgt53((void *)t2d, 16, 15, 15) ;
  fprintf(stderr, " : errors = %d\n\n", errors((void *)r2d, (void *)t2d, 16*16));
  if(nerr) goto fail ;

  fprintf(stderr, "SUCCESS\n");

//   int bench[64][64] ;
//   for(j=0 ; j<64 ; j++){
//     for(i=0; i<64 ; i++){
//       bench[j][i] = sin[i&15] * cos[j&15] + 1 ;
//     }
//   }

  return 0 ;

fail:
  fprintf(stderr, "FAILED\n");
  return 1 ;
}
