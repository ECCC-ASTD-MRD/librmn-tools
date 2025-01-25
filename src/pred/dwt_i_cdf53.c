#include <stdio.h>
int is_odd(int n) {
  return (n & 1) ;
}

void cdf53_f_i(int *tmp, int N){
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
void fwd_1d_cdf53_split_even(int *x, int *e, int *o, int n){    // InTc
  int i;
  int neven = (n+1) >> 1;
  int nodd  = neven;

  for(i = 0 ; i < nodd-1 ; i++) o[i] = x[i+i+1] - ((x[i+i] + x[i+i+2]) >> 1);  // predict odd terms
  o[nodd-1] = x[n-1] - x[n-2] ;

  e[0 ] = x[0] + ((o[0] + 1) >> 1) ;
  for(i = 1; i < neven ; i++) e[i] = x[i+i] + ((o[i] + o[i-1] + 2) >> 2) ;     // update even terms
}
void fwd_1d_cdf53_split_odd(int *x, int *e, int *o, int n){    // InTc
//****
  int i;
  int neven = (n+1) >> 1;
  int nodd  = n >> 1;

  for(i = 0 ; i < nodd ; i++) o[i] = x[i+i+1] - ((x[i+i] + x[i+i+2]) >> 1);    // predict odd terms

  e[0 ] = x[0] + ((o[0] + 1) >> 1) ;
  for(i = 1; i < neven-1 ; i++) e[i] = x[i+i] + ((o[i] + o[i-1] + 2) >> 2) ;   // update even terms
  e[neven-1] = x[n-1] + ((o[nodd-1] + 1) >> 1) ;
}

void cdf53_i_i(int *tmp, int N){
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
void inv_1d_cdf53_split_even(int *x, int *e, int *o, int n){    // InTc
  int i;
  int neven = (n+1) >> 1;
//   int nodd  = neven;

  for(i = 0 ; i < neven ; i++){ x[i+i] = e[i] ; x[i+i+1] = o[i] ; }  // move to x

  for (i = 2; i < n; i += 2) x[i] -= ((x[i+1] + x[i-1] + 2) >> 2);   // unupdate even terms
  x[0] -= ((x[1] + 1) >> 1) ;

  x[n - 1] += x[n - 2] ;
  for (i = 1; i < n - 2; i += 2) x[i] += ((x[i-1] + x[i+1]) >> 1) ;  // unpredict odd terms
}
void inv_1d_cdf53_split_odd(int *x, int *e, int *o, int n){    // InTc
//****
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

#if defined(SELF_TEST)
int main(int argc, char **argv){
  int i ;
  int tmp[16] = {-1, 11, 2, 23, 3, 32, 1, 0, 10, 1, 22, 3, 44, 5, 66, 8 } ;
  int e[16], o[16] ;

  fprintf(stderr, "original data\n") ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");
  fprintf(stderr, "even number of points\n") ;
  cdf53_f_i(tmp, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  cdf53_i_i(tmp, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");

//   for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
  fprintf(stderr, "odd number of points\n") ;
  cdf53_f_i(tmp, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n");
//   for(i=0; i<7 ; i++){ fprintf(stderr, "%4d ", tmp[i+i+1]) ; } fprintf(stderr, "\n");
//   for(i=0; i<8 ; i++){ fprintf(stderr, "%4d ", tmp[i+i]) ; } fprintf(stderr, "\n");
  cdf53_i_i(tmp, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");

  fprintf(stderr, "even number of points, split\n") ;
  for(i=0; i<8 ; i++){ e[i] = o[i] = 8888 ; }
  fwd_1d_cdf53_split_even(tmp, e, o, 16) ;
  for(i=0; i<8 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } fprintf(stderr, "\n");
  inv_1d_cdf53_split_even(tmp, e, o, 16) ;
  for(i=0; i<16 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");

  fprintf(stderr, "odd number of points, split\n") ;
  fwd_1d_cdf53_split_odd(tmp, e, o, 15) ;
  for(i=0; i<7 ; i++){ fprintf(stderr, "%4d %4d ", e[i], o[i]) ; } ; fprintf(stderr, "%4d ", e[7]) ; fprintf(stderr, "\n");
  inv_1d_cdf53_split_odd(tmp, e, o, 15) ;
  for(i=0; i<15 ; i++){ fprintf(stderr, "%4d ", tmp[i]) ; } fprintf(stderr, "\n\n");
}
#endif
