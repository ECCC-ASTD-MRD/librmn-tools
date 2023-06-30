#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rmn/tools_types.h>
#include <rmn/test_helpers.h>
#include <rmn/ieee_quantize.h>
#include <rmn/timers.h>

float max_rel_err(float *ref, float *new, int n){
  int i, i0, np ;
  float maxerr, t, avgerr, avgerr2 ;
  maxerr = 0.0f ; avgerr = 0.0f ; avgerr2 = 0.0f ; np = 0 ;
  for(i=0 ; i<n ; i++){
    if(ref[i] != 0){
      np ++ ;
      t = (new[i] - ref[i]) / ref[i] ;
      t = (t < 0) ? -t : t ;
      avgerr += t ;
      if(t != 0.0f) avgerr2 += (1.0f / t) ;
      if(t > maxerr){
        maxerr = t ;
        i0 = i ;
      }
    }
  }
  t = 1.0f / maxerr ;
  avgerr /= np ;
  avgerr2 /= np ;
// fprintf(stderr, "max rel err at i0 = %d, avg rel err = %f (%f)\n", i0, 1.0f / avgerr, avgerr2) ;
fprintf(stderr, "max rel err at i0 = %d, avg rel err = 1 part in %.0f\n", i0, 1.0f / avgerr) ;
  return t ;
}

void scale_float_test_data_1D(float *f, int n, float scale, float offset){
  int i ;
  for(i=0 ; i<n ; i++){
    f[i] = f[i] * scale + offset ;
  }
}

void ramp_float_test_data_1D(float *f, int n, float start, float alpha){
  int i ;
  f[0] = start ;
  for(i=1 ; i<n ; i++){
    f[i] = f[i-1] * alpha ;
  }
}

#define NPTS 4095

int main(int argc, char **argv){
  float x[NPTS], q[NPTS], r[NPTS], qzero, maxerr ;
  uint32_t *qi = (uint32_t *) q ;
  int i, nbits ;
  AnyType32 my_float ;
  uint64_t h64 ;
  TIME_LOOP_DATA ;
  int emin, emax ;

  start_of_test(argv[0]);

  my_float.f = -1.0000001f ;
  fprintf(stderr,"float = %12.7f, u = %8.8x, sign = %d, exponent = %d, mantissa = %8.8x\n", 
          my_float.f, my_float.u, my_float.ie32.s, my_float.ie32.e, my_float.ie32.m) ;

  nbits = 16 ; 
  qzero = 1.0f/512 ;
//   qzero = 1.0f ;
  fprintf(stderr,"nbits = %d\n", nbits) ;
//   x[0] = 1.00001f ;
//   for(i=1 ; i<NPTS ; i++) x[i] = 1.00135f * x[i-1] ;
  ramp_float_test_data_1D(x, NPTS, 1.00001f, 1.00135f) ;
  scale_float_test_data_1D(x, NPTS, 1.0f/4, 0.0f) ;
  h64 = IEEE32_limits(x, NPTS) ;
  IEEE32_exp_limits(h64, &emin, &emax) ;
  fprintf(stderr,"exponent range : %d -> %d (%d)\n", emin, emax, emax-emin+1) ;
//   for(i=0 ; i<NPTS ; i++) x[i] /= 16.0f ;
//   for(i=0 ; i<NPTS ; i++) x[i] = (x[i] < qzero) ? qzero : x[i] ;
//   for(i=0 ; i<NPTS ; i++) x[i] = -x[i] ;
//   x[NPTS-1] = -x[NPTS-1] ;

  fprintf(stderr, "%12f ", x[0]) ;
  for(i=1 ; i<NPTS ; i+=511) fprintf(stderr, "%12f ", x[i]) ;
  fprintf(stderr, "%12f\n", x[NPTS-1]) ;
  fprintf(stderr, "\n") ;

  h64 = IEEE32_fakelog_quantize_0(x, NPTS, nbits, qzero, qi);
  fprintf(stderr, "%12d ", qi[0]) ;
  for(i=1 ; i<NPTS ; i+=511) fprintf(stderr, "%12d ", qi[i]) ;
  fprintf(stderr, "%12d\n", qi[NPTS-1]) ;
  fprintf(stderr, "\n") ;

  TIME_LOOP_EZ(1000, NPTS, h64 = IEEE32_fakelog_quantize_0(x, NPTS, nbits, qzero, qi)) ;
  fprintf(stderr, "IEEE32_fakelog_quantize_0    : %s\n\n",timer_msg);

  IEEE32_fakelog_unquantize_0(r, h64, NPTS, qi);
  fprintf(stderr, "%12f ", r[0]) ;
  for(i=1 ; i<NPTS ; i+=511) fprintf(stderr, "%12f ", r[i]) ;
  fprintf(stderr, "%12f\n", r[NPTS-1]) ;
  fprintf(stderr, "\n") ;

  TIME_LOOP_EZ(1000, NPTS, IEEE32_fakelog_unquantize_0(r, h64, NPTS, qi)) ;
  fprintf(stderr, "IEEE32_fakelog_unquantize_0    : %s\n\n",timer_msg);

  fprintf(stderr, "%12.0f ", x[0]/(x[0]-r[0])) ;
  for(i=1 ; i<NPTS ; i+=511) fprintf(stderr, "%12.0f ", x[i]/(x[i]-r[i])) ;
  fprintf(stderr, "%12.0f\n", x[NPTS-1]/(x[NPTS-1]-r[NPTS-1])) ;
  maxerr = max_rel_err(x, r, NPTS) ;
  fprintf(stderr, "max rel err = 1 part in %12.0f\n", maxerr) ;
  fprintf(stderr, "\n") ;

}
