#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rmn/fp_qflog.h>
#include <rmn/timers.h>
#include <rmn/test_helpers.h>

float max_rel_err(float *ref, float *new, int n){
  int i, i0 = 0, np ;
  float maxerr, t, avgerr ;
  maxerr = 0.0f ; avgerr = 0.0f ; np = 0 ;
  for(i=0 ; i<n ; i++){
    if(ref[i] != 0){
      np ++ ;
      t = (new[i] - ref[i]) / ref[i] ;
      t = (t < 0) ? -t : t ;
      avgerr += t ;
//       if(t != 0.0f) avgerr2 += (1.0f / t) ;
      if(t > maxerr){
        maxerr = t ;
        i0 = i ;
      }
    }
  }
  t = 1.0f / maxerr ;
  avgerr /= np ;
//   avgerr2 /= np ;
// fprintf(stderr, "max rel err at i0 = %d, avg rel err = %f (%f)\n", i0, 1.0f / avgerr, avgerr2) ;
fprintf(stderr, "max rel err at i0 = %d, 1 part in %.0f, avg error = 1 part in %.0f\n", i0, 1.0f / maxerr, 1.0f / avgerr) ;
  return t ;
}


#define NPTS 4096
#define NITER 1000

int main(int argc, char **argv){
  (void)(argc) ;
  (void)(argv) ;
  float x[NPTS], r[NPTS], maxerr ;
  int32_t q[NPTS] ;
  int i, nbits ;
  float t0 ;
  TIME_LOOP_DATA ;

  start_of_test("qflog<->fp test");

  x[0] = 1.0000f ;
  for(i=1 ; i<NPTS ; i++) { x[i] = 1.021f * x[i-1] ; } ;
  for(i=1 ; i<NPTS ; i+=2) { x[i] = -x[i] ; } ;

  nbits = 16 ;
  fp_to_flog(x, q, NPTS, nbits) ;
  for(i=1 ; i<NPTS ; i++) { r[i] = 0 ; } ;
  flog_to_fp(r, q, NPTS, nbits) ;
  maxerr = max_rel_err(x, r, NPTS) ;
  fprintf(stderr, "i = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/7)){ fprintf(stderr, "%14d"  ,   i ) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "x = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/8)){ fprintf(stderr, "%14.6E", x[i]) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "q = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/8)){ fprintf(stderr, "%14.8x", q[i]) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "r = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/8)){ fprintf(stderr, "%14.6E", r[i]) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "max rel err : 1 part in %.0f, target = 1 part in %.0f\n", maxerr, 1.0f*(1 << nbits)) ;
  if(maxerr < (1 << nbits)) goto fail ;
  fprintf(stderr, "SUCCESS\n\n");

  for(i=0 ; i<NPTS ; i++) { x[i] = 1.0f/x[i] ; } ;
  fp_to_flog(x, q, NPTS, nbits) ;
  for(i=1 ; i<NPTS ; i++) { r[i] = 0 ; } ;
  flog_to_fp(r, q, NPTS, nbits) ;
  maxerr = max_rel_err(x, r, NPTS) ;
  fprintf(stderr, "i = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/7)){ fprintf(stderr, "%14d"  ,   i ) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "x = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/8)){ fprintf(stderr, "%14.6E", x[i]) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "q = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/8)){ fprintf(stderr, "%14.8x", q[i]) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "r = ") ; for(i=0 ; i<NPTS ; i+=(NPTS/8)){ fprintf(stderr, "%14.6E", r[i]) ; } ; fprintf(stderr, "\n") ;
  fprintf(stderr, "max rel err : 1 part in %.0f, target = 1 part in %.0f\n", maxerr, 1.0f*(1 << nbits)) ;
  if(maxerr < (1 << nbits)) goto fail ;
  fprintf(stderr, "SUCCESS\n\n");

  TIME_LOOP_EZ(NITER, NPTS, fp_to_flog(x, q, NPTS, nbits) ) ;
  if(timer_min == timer_max) timer_avg = timer_max ;
  t0 = timer_min * NaNoSeC / (NPTS) ;
  fprintf(stderr, "fp_to_flog  : %4.2f ns/float, %s\n", t0, timer_msg) ;

  TIME_LOOP_EZ(NITER, NPTS, fp_to_qlog(x, q, NPTS, nbits, 0.0f, 0.0f) ) ;
  if(timer_min == timer_max) timer_avg = timer_max ;
  t0 = timer_min * NaNoSeC / (NPTS) ;
  fprintf(stderr, "fp_to_qlog0 : %4.2f ns/float, %s\n", t0, timer_msg) ;

  TIME_LOOP_EZ(NITER, NPTS, fp_to_qlog(x, q, NPTS, nbits, 1.0E-34f, 0.0f) ) ;
  if(timer_min == timer_max) timer_avg = timer_max ;
  t0 = timer_min * NaNoSeC / (NPTS) ;
  fprintf(stderr, "fp_to_qlog  : %4.2f ns/float, %s\n\n", t0, timer_msg) ;

  TIME_LOOP_EZ(NITER, NPTS, flog_to_fp(r, q, NPTS, nbits) ) ;
  if(timer_min == timer_max) timer_avg = timer_max ;
  t0 = timer_min * NaNoSeC / (NPTS) ;
  fprintf(stderr, "flog_to_fp  : %4.2f ns/float, %s\n", t0, timer_msg) ;

  TIME_LOOP_EZ(NITER, NPTS, qlog_to_fp(r, q, NPTS, nbits, 0.0f, 0.0f) ) ;
  if(timer_min == timer_max) timer_avg = timer_max ;
  t0 = timer_min * NaNoSeC / (NPTS) ;
  fprintf(stderr, "qlog_to_fp0 : %4.2f ns/float, %s\n", t0, timer_msg) ;

  TIME_LOOP_EZ(NITER, NPTS, qlog_to_fp(r, q, NPTS, nbits, 1.0E-34f, 1.0E-34f) ) ;
  if(timer_min == timer_max) timer_avg = timer_max ;
  t0 = timer_min * NaNoSeC / (NPTS) ;
  fprintf(stderr, "qlog_to_fp  : %4.2f ns/float, %s\n", t0, timer_msg) ;

//   uint64_t t1, t2 ;
//   t1 = elapsed_cycles() ;
//   fp_to_flog(x, q, NPTS, nbits) ;
//   t2 = elapsed_cycles() ;
//   fprintf(stderr, "fp_to_flog : cycles = %ld\n", t2-t1) ;

// TODO : add test for e5m10_to_flog/flog_to_e5m10 and e8m7_to_flog/flog_to_e8m7

  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  fprintf(stderr, "FAILED\n") ;
  return 1 ;
}
