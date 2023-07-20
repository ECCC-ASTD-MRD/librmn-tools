#include <rmn/data_info.h>
#include <rmn/timers.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#define NP 16384
#define NP2 (NP*2+1)
#define NTIMES 30

int main(int argc, char **argv){
  float ff[NP2] ;
  int32_t fi[NP2] ;
  int i ;
  limits_f lf ;
  limits_i li ;
  uint64_t t[NTIMES] ;
  TIME_LOOP_DATA ;
  char *errmsg ;

  start_of_test("data_info test C");

  ff[NP] = 1.1f ;
  fi[NP] = 1 ;
  for(i=1 ; i<=NP ; i++){
    ff[NP-i] = ff[NP-i+1] * 1.001f ;
    fi[NP-i] = fi[NP-i+1] + 127 ;
    ff[NP+i] = -ff[NP-i] * 1.001f ;
    fi[NP+i] = -fi[NP-i] - 1 ;
  }
  if(NP < 9) {
    for(i=0 ; i<NP2 ; i++) TEE_FPRINTF(stderr,2, "%f ", ff[i]); TEE_FPRINTF(stderr,2, "\n") ;
  }
  // testing very short case (length 3)

  TEE_FPRINTF(stderr,2, "short array = ") ;
  for(i=NP-1 ; i<NP+2 ; i++) TEE_FPRINTF(stderr,2, "%f ", ff[i]); TEE_FPRINTF(stderr,2, ", missing = %f\n", ff[NP]) ;
  lf.maxa = lf.mina = lf.maxs = lf.mins = 0 ;
  lf = IEEE32_extrema(ff+NP-1, 3) ;
  TEE_FPRINTF(stderr,2, "SHORT        : maxs = %f, mins = %f, maxa = %f, mina = %f\n", lf.maxs, lf.mins, lf.maxa, lf.mina) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[NP-1], ff[NP+1], -ff[NP+1], ff[NP]) ;
  errmsg = "not getting expected value" ;
  if(lf.maxs != ff[NP-1] || lf.mins != ff[NP+1] || lf.maxa != -ff[NP+1] || lf.mina != ff[NP]) goto error ;

  lf = IEEE32_extrema_missing_simd(ff+NP-1, 3, ff+NP, 0) ;
  TEE_FPRINTF(stderr,2, "SHORT MISSING: maxs = %f, mins = %f, maxa = %f, mina = %f\n", lf.maxs, lf.mins, lf.maxa, lf.mina) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[NP-1], ff[NP+1], -ff[NP+1], ff[NP-1]) ;
  errmsg = "not getting expected value" ;
  if(lf.maxs != ff[NP-1] || lf.mins != ff[NP+1] || lf.maxa != -ff[NP+1] || lf.mina != ff[NP-1]) goto error ;

  lf.maxa = lf.mina = lf.maxs = lf.mins = 0 ;
  lf = IEEE32_extrema(ff, NP2) ;
  TEE_FPRINTF(stderr,2, "NO MISSING   : maxs = %f, mins = %f, maxa = %f, mina = %f\n", lf.maxs, lf.mins, lf.maxa, lf.mina) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[0], ff[NP2-1], -ff[NP2-1], ff[NP]) ;
  if(lf.maxs != ff[0] || lf.mins != ff[NP2-1] || lf.maxa != -ff[NP2-1] || lf.mina != ff[NP]) goto error ;

  lf.maxa = lf.mina = lf.maxs = lf.mins = 0 ;
  lf = IEEE32_extrema_missing_simd(ff, NP2, ff+NP, 0) ;
  TEE_FPRINTF(stderr,2, "WITH MISSING : maxs = %f, mins = %f, maxa = %f, mina = %f\n", lf.maxs, lf.mins, lf.maxa, lf.mina) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[0], ff[NP2-1], -ff[NP2-1], ff[NP-1]) ;
  if(lf.maxs != ff[0] || lf.mins != ff[NP2-1] || lf.maxa != -ff[NP2-1] || lf.mina != ff[NP-1]) goto error ;

  TIME_LOOP_EZ(1000, NP2, lf = IEEE32_extrema(ff, NP2)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema         : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NP2, lf = IEEE32_extrema_missing_simd(ff, NP2, ff+NP, 0)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema_missing : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NP2, lf = IEEE32_extrema_c_missing(ff, NP2, ff+NP, 0)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema_c_missing : %s\n",timer_msg);

  li.maxa = li.mina = li.maxs = li.mins = 0 ;
  li = int32_extrema(fi, NP2) ;
  TEE_FPRINTF(stderr,2, "maxs = %d, mins = %d, maxa = %d, mina = %d\n", li.maxs, li.mins, li.maxa, li.mina) ;

  TIME_LOOP_EZ(1000, NP2, li = int32_extrema(fi, NP2)) ;
  TEE_FPRINTF(stderr,2, "int32_extrema    : %s\n",timer_msg);

  TEE_FPRINTF(stderr,2, "\nSUCCESS\n") ;
  return 0 ;

error:
  TEE_FPRINTF(stderr,2, "\nERROR : %s\n", errmsg) ;
  return 1 ;
}
