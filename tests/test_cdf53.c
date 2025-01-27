#include <stdio.h>
#include <rmn/dwt_i_cdf53.h>

static int errors(int *a, int *b, int n){
  int nerr = 0 ;
  int i ;
  for(i=0; i<n ; i++) {
    if(a[i] != b[i]) nerr++ ;
  }
  return nerr ;
}

int main(int argc, char **argv){
  int i ;
  int tmp[16] = {-1, 11, 2, -23, 3, 32, 1, 0, 10, 1, -22, -3, 44, 5, -66, 8 } ;
  int ref[16] ;
  int e[16], o[16] ;

  for(i=0; i<16 ; i++){ ref[i] = tmp[i] ; }

  fprintf(stderr, "original data\n") ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");

  fprintf(stderr, "in place, even number of points\n") ;
  fwd_1d_cdf53(tmp, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  inv_1d_cdf53(tmp, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));


//   for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  fprintf(stderr, "in place, odd number of points\n") ;
  fwd_1d_cdf53(tmp, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
//   for(i=0; i<7 ; i++){ fprintf(stderr, "%4d ", tmp[i+i+1]) ; } fprintf(stderr, "\n");
//   for(i=0; i<8 ; i++){ fprintf(stderr, "%4d ", tmp[i+i]) ; } fprintf(stderr, "\n");
  inv_1d_cdf53(tmp, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 15));

  fprintf(stderr, "split, odd number of points\n") ;
  fwd_1d_cdf53_split(tmp, e, o, 15) ;
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } ; fprintf(stderr, "%4d ", e[7]) ; fprintf(stderr, "\n");
  inv_1d_cdf53_split(tmp, e, o, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 15));

  fprintf(stderr, "split, even number of points\n") ;
  for(i=0; i<8 ; i++){ e[i] = o[i] = 8888 ; }
  fwd_1d_cdf53_split(tmp, e, o, 16) ;
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } fprintf(stderr, "\n");
  inv_1d_cdf53_split(tmp, e, o, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));

  fprintf(stderr, "split, in place, odd number of points\n") ;
  fwd_1d_cdf53_split_inplace(tmp, 15);
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "%4d\n", tmp[7]);
  inv_1d_cdf53_split_inplace(tmp, 15);
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 15));

  fprintf(stderr, "split, in place, even number of points\n") ;
  fwd_1d_cdf53_split_inplace(tmp, 16);
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "\n");
  inv_1d_cdf53_split_inplace(tmp, 16);
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));

  fprintf(stderr, "2D, even number of points along j, ni == 1\n") ;
  fwd_2d_cdf53(tmp, 1, 1, 16);
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "\n");
  inv_2d_cdf53(tmp, 1, 1, 16);
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));

  fprintf(stderr, "2D, odd number of points along j, ni == 1\n") ;
  fwd_2d_cdf53(tmp, 1, 1, 15);
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", tmp[i], tmp[8+i]) ; } ; fprintf(stderr, "%4d\n", tmp[7]);
  inv_2d_cdf53(tmp, 1, 1, 15);
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 15));

  fprintf(stderr, "split, in place, even number of points, 3 levels\n") ;
  fwd_1d_cdf53_split_inplace_n(tmp, 16, 2);
  inv_1d_cdf53_split_inplace_n(tmp, 16, 2);
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));

  fprintf(stderr, "split, in place, odd number of points, 3 levels\n") ;
  fwd_1d_cdf53_split_inplace_n(tmp, 15, 2);
  inv_1d_cdf53_split_inplace_n(tmp, 15, 2);
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 15));

  fprintf(stderr, "2D, even number of points along j, ni == 1, 3 levels\n") ;
  fwd_2d_cdf53_n(tmp, 1, 1, 16, 3);
  inv_2d_cdf53_n(tmp, 1, 1, 16, 3);
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, ", errors = %d\n\n", errors(ref, tmp, 16));
}
