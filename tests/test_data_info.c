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
  limits_w32 l ;
  uint64_t u64 ;
  uint64_t t[NTIMES] ;
  TIME_LOOP_DATA ;
  char *errmsg ;
  float fg = 0.5f ;  // replacement value for "missing" values
  float pad = 1.12345f ;
  int32_t iplug = 12 ;

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
// goto timings ;
  // testing very short case (length 3)

  TEE_FPRINTF(stderr,2, "short array = ") ;
  for(i=NP-1 ; i<NP+2 ; i++) TEE_FPRINTF(stderr,2, "%f ", ff[i]); TEE_FPRINTF(stderr,2, ", missing = %f\n", ff[NP]) ;
  l.f.maxa = l.f.mina = l.f.maxs = l.f.mins = 0 ;
  l = IEEE32_extrema(ff+NP-1, 3) ;
  TEE_FPRINTF(stderr,2, "SHORT             : maxs = %f, mins = %f, maxa = %f, mina = %f\n", l.f.maxs, l.f.mins, l.f.maxa, l.f.mina) ;
  TEE_FPRINTF(stderr,2, "  expected        : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[NP-1], ff[NP+1], -ff[NP+1], ff[NP]) ;
  TEE_FPRINTF(stderr,2, "                  : allp = %d, allm = %d\n", W32_ALLP(l), W32_ALLM(l)) ;
  errmsg = "not getting expected value" ;
  if(l.f.maxs != ff[NP-1] || l.f.mins != ff[NP+1] || l.f.maxa != -ff[NP+1] || l.f.mina != ff[NP]) goto error ;
#if 0
  l = IEEE32_extrema_missing_simd(ff+NP-1, 3, ff+NP, 0, NULL) ;
  TEE_FPRINTF(stderr,2, "SHORT MISSING SIMD: maxs = %f, mins = %f, maxa = %f, mina = %f\n", l.f.maxs, l.f.mins, l.f.maxa, l.f.mina) ;
  TEE_FPRINTF(stderr,2, "  expected        : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[NP-1], ff[NP+1], -ff[NP+1], ff[NP-1]) ;
  if(l.f.maxs != ff[NP-1] || l.f.mins != ff[NP+1] || l.f.maxa != -ff[NP+1] || l.f.mina != ff[NP-1]) goto error ;
#endif
  l = IEEE32_extrema_missing(ff+NP-1, 3, ff+NP, 0, NULL) ;
  TEE_FPRINTF(stderr,2, "SHORT MISSING C   : maxs = %f, mins = %f, maxa = %f, mina = %f, missing = %f\n", l.f.maxs, l.f.mins, l.f.maxa, l.f.mina, ff[NP]) ;
  TEE_FPRINTF(stderr,2, "  expected        : maxs = %f, mins = %f, maxa = %f, mina = %f\n", ff[NP-1], ff[NP+1], -ff[NP+1], ff[NP-1]) ;
  if(l.f.maxs != ff[NP-1] || l.f.mins != ff[NP+1] || l.f.maxa != -ff[NP+1] || l.f.mina != ff[NP-1]){
    TEE_FPRINTF(stderr,2, "  errors : %g, %g, %g, %g\n", l.f.maxs-ff[NP-1], l.f.mins-ff[NP+1], l.f.maxa+ff[NP+1], l.f.mina-ff[NP-1]) ;
    goto error ;
  }
  TEE_FPRINTF(stderr,2, "\n");
  // testing at full length (NP2)
  ff[NP+3] = 0.0f ; // make sure there is a zero in the fray to test min0

  l.f.maxa = l.f.mina = l.f.maxs = l.f.mins = 0 ;
  l = IEEE32_extrema(ff, NP2) ;
  TEE_FPRINTF(stderr,2, "NO MISSING   : maxs = %f, mins = %f, maxa = %f, mina = %f, min0 = %f\n", l.f.maxs, l.f.mins, l.f.maxa, l.f.mina, l.f.min0) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f, min0 = %f\n", ff[0], ff[NP2-1], -ff[NP2-1], 0.0, ff[NP]) ;
  if(l.f.maxs != ff[0] || l.f.mins != ff[NP2-1] || l.f.maxa != -ff[NP2-1] || l.f.mina != 0.0 || l.f.min0 != ff[NP]) goto error ;
#if 0
  l.f.maxa = l.f.mina = l.f.maxs = l.f.mins = 0 ;
  l = IEEE32_extrema_missing_simd(ff, NP2, ff+NP, 0, NULL) ;   // NO replacement value for "missing" values
  TEE_FPRINTF(stderr,2, "SIMD MISSING : maxs = %f, mins = %f, maxa = %f, mina = %f, min0 = %f\n", l.f.maxs, l.f.mins, l.f.maxa, l.f.mina, l.f.min0) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f, min0 = %f\n", ff[0], ff[NP2-1], -ff[NP2-1], 0.0, ff[NP-1]) ;
  if(l.f.maxs != ff[0] || l.f.mins != ff[NP2-1] || l.f.maxa != -ff[NP2-1] || l.f.mina != 0.0 || l.f.min0 != ff[NP-1]) goto error ;
#endif
  l.f.maxa = l.f.mina = l.f.maxs = l.f.mins = 0 ;
  l = IEEE32_extrema_missing(ff, NP2, ff+NP, 0, &fg) ;       // replacement value for "missing" values supplied
  TEE_FPRINTF(stderr,2, "C    MISSING : maxs = %f, mins = %f, maxa = %f, mina = %f, min0 = %f, missing = %f\n", l.f.maxs, l.f.mins, l.f.maxa, l.f.mina, l.f.min0, ff[NP]) ;
  TEE_FPRINTF(stderr,2, "  expected   : maxs = %f, mins = %f, maxa = %f, mina = %f, min0 = %f\n", ff[0], ff[NP2-1], -ff[NP2-1], 0.0, fg) ;
  if(l.f.maxs != ff[0] || l.f.mins != ff[NP2-1] || l.f.maxa != -ff[NP2-1] || l.f.mina != 0.0 || l.f.min0 != fg) goto error ;
  TEE_FPRINTF(stderr,2, "\n");

  l.i.maxa = l.i.mina = l.i.maxs = l.i.mins = 0 ;
  fi[1] = 0 ;
  fi[NP2-2] = 0 ;
  l = INT32_extrema(fi, NP2) ;
  TEE_FPRINTF(stderr,2, "INT32_extrema : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n", l.i.maxs, l.i.mins, l.i.maxa, l.i.mina, l.i.min0) ;
  TEE_FPRINTF(stderr,2, "  expected    : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n", fi[0], fi[NP2-1], -fi[NP2-1], 0, fi[NP]) ;
  if(l.i.maxs != fi[0] || l.i.mins != fi[NP2-1] || l.i.maxa != -fi[NP2-1] || l.i.mina != 0 || l.i.min0 != fi[NP]) goto error ;

  l.i.maxa = l.i.mina = l.i.maxs = l.i.mins = 0 ;
  l = INT32_extrema_missing(fi, NP2, fi+NP, 0, NULL) ;   // NO replacement value for "missing" values
  TEE_FPRINTF(stderr,2, "I_extrema_mis : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d, missing = %d\n", l.i.maxs, l.i.mins, l.i.maxa, l.i.mina, l.i.min0, fi[NP]) ;
  TEE_FPRINTF(stderr,2, "  expected    : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n", fi[0], fi[NP2-1], -fi[NP2-1], 0, fi[NP-1]) ;
  if(l.i.maxs != fi[0] || l.i.mins != fi[NP2-1] || l.i.maxa != -fi[NP2-1] || l.i.mina != 0 || l.i.min0 != fi[NP-1]) goto error ;
  TEE_FPRINTF(stderr,2, "\n");

  l = INT32_extrema_missing(fi, NP+1, NULL, 0, NULL) ;     // lower NP+1 values, no missing value
  TEE_FPRINTF(stderr,2, "extrema(lo)    : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n", l.i.maxs, l.i.mins, l.i.maxa, l.i.mina, l.i.min0) ;
  TEE_FPRINTF(stderr,2, "  expected     : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n", fi[0], 0, fi[0], 0, fi[NP]) ;
  if(l.i.maxs != fi[0] || l.i.mins != 0 || l.i.maxa != fi[0] || l.i.mina != 0 || l.i.min0 != fi[NP]) goto error ;
  TEE_FPRINTF(stderr,2, "               : allp = %d, allm = %d, expected allp = %d, allm = %d\n", l.i.allp, l.i.allm, 1, 0) ;
  errmsg = "bad allp/allm values" ;
  if(l.i.allp != 1 || l.i.allm != 0) goto error ;

  l = INT32_extrema_missing(fi, NP+1, fi+NP, 0, &iplug) ;     // lower NP+1 values, missing value = 1, plug = 2
  TEE_FPRINTF(stderr,2, "extrema(lo)(m) : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d, missing = %d, plug = %d\n",
                      l.i.maxs, l.i.mins, l.i.maxa, l.i.mina, l.i.min0, fi[NP], iplug) ;
  TEE_FPRINTF(stderr,2, "  expected     : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n",fi[0], 0, fi[0], 0, iplug) ;
  if(l.i.maxs != fi[0] || l.i.mins != 0 || l.i.maxa != fi[0] || l.i.mina != 0 || l.i.min0 != iplug) goto error ;
  TEE_FPRINTF(stderr,2, "               : allp = %d, allm = %d, expected allp = %d, allm = %d\n", l.i.allp, l.i.allm, 1, 0) ;
  errmsg = "bad allp/allm values" ;
  if(l.i.allp != 1 || l.i.allm != 0) goto error ;

  l = INT32_extrema_missing(fi+NP, NP+1, NULL, 0, NULL) ;  // upper NP+1 values, no missing value
  TEE_FPRINTF(stderr,2, "extrema(hi)    : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n", l.i.maxs, l.i.mins, l.i.maxa, l.i.mina, l.i.min0) ;
  TEE_FPRINTF(stderr,2, "  expected     : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n",fi[NP], fi[NP2-1], -fi[NP2-1], 0, fi[NP]) ;
  if(l.i.maxs != fi[NP] || l.i.mins != fi[NP2-1] || l.i.maxa != -fi[NP2-1] || l.i.mina != 0 || l.i.min0 != fi[NP]) goto error ;
  TEE_FPRINTF(stderr,2, "               : allp = %d, allm = %d, expected allp = %d, allm = %d\n", l.i.allp, l.i.allm, 0, 0) ;
  if(l.i.allp != 0 || l.i.allm != 0) goto error ;

  iplug = -15 ;
  l = INT32_extrema_missing(fi+NP, NP+1, fi+NP, 0, &iplug) ;  // upper NP+1 values, missing value = 1
  TEE_FPRINTF(stderr,2, "extrema(hi)(m) : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d, missing = %d, plug = %d\n",
                      l.i.maxs, l.i.mins, l.i.maxa, l.i.mina, l.i.min0, fi[NP], iplug) ;
  TEE_FPRINTF(stderr,2, "  expected     : maxs = %d, mins = %d, maxa = %d, mina = %d, min0 = %d\n",0, fi[NP2-1], -fi[NP2-1], 0, -iplug) ;
  if(l.i.maxs != 0 || l.i.mins != fi[NP2-1] || l.i.maxa != -fi[NP2-1] || l.i.mina != 0 || l.i.min0 != -iplug) goto error ;
  TEE_FPRINTF(stderr,2, "               : allp = %d, allm = %d, expected allp = %d, allm = %d\n", l.i.allp, l.i.allm, 0, 1) ;
  if(l.i.allp != 0 || l.i.allm != 1) goto error ;

  ff[0] = ff[NP2-1] = fg ;
  i = W32_replace_missing(ff, NP2, &fg, 0, &pad) ;
  TEE_FPRINTF(stderr,2, "rep_missing   : %f, %f\n", ff[0], ff[NP2-1]) ;
  TEE_FPRINTF(stderr,2, "  expected    : %f, %f\n", pad, pad) ;
  if(ff[0] != pad || ff[NP2-1] != pad) goto error ;           // pad should be found at 0 and NP2-1
  errmsg = "getting unexpected pad value" ;
  for(i = 1 ; i < NP2-1 ; i++) if(ff[i] == pad) goto error ;  // pad should only be found at 0 and NP2-1

timings :
  TEE_FPRINTF(stderr,2, "====================  TIMINGS ====================\n") ;
  TIME_LOOP_EZ(1000, NP2, l = IEEE32_extrema(ff, NP2)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema             : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NP2, l = IEEE32_extrema_abs(ff, NP2)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema_abs         : %s\n",timer_msg);
#if 0
  TIME_LOOP_EZ(1000, NP2, l = IEEE32_extrema_missing_simd(ff, NP2, ff+NP, 0, NULL)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema_missing_simd: %s\n",timer_msg);
#endif
  TIME_LOOP_EZ(1000, NP2, l = IEEE32_extrema_missing(ff, NP2, ff+NP, 0, NULL)) ;
  TEE_FPRINTF(stderr,2, "IEEE32_extrema_missing     : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NP2, l = INT32_extrema(fi, NP2)) ;
  TEE_FPRINTF(stderr,2, "INT32_extrema              : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NP2, l = INT32_extrema_missing(fi, NP2, fi+NP, 0, NULL)) ;
  TEE_FPRINTF(stderr,2, "INT32_extrema_missing      : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NP2, i = W32_replace_missing(ff, NP2, &fg, 0, &pad)) ;
  TEE_FPRINTF(stderr,2, "W32_replace_missing        : %s\n",timer_msg);

  TEE_FPRINTF(stderr,2, "\nSUCCESS\n") ;
  return 0 ;

error:
  TEE_FPRINTF(stderr,2, "\nERROR : %s\n", errmsg) ;
  return 1 ;
}
