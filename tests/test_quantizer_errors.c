#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>

#include <rmn/test_helpers.h>
#include <rmn/misc_operators.h>
#include <rmn/move_blocks.h>
#include <rmn/c_record_io.h>
#include <rmn/ieee_quantize.h>
#include <rmn/compress_data.h>
#include <rmn/compress_data.h>
#include <rmn/eval_diff.h>
#include <rmn/timers.h>
#include <rmn/lorenzo.h>
#include <rmn/tile_encoders.h>
#include <rmn/print_bitstream.h>

#define NTIMES 100000

#define INDEX_F(F, I, LNI, J) (F + I + J*LNI)

typedef struct{
  uint64_t bias:32,  // minimum absolute value (actually never more than 31 bits)
           npts:16,  // number of points, 0 means unknown
           resv:3 ,
           shft:5 ,  // shift count (0 -> 31) for quantize/restore
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32p ;
typedef struct {
  uint64_t bias:32,  // integer offset reflecting minimum value of packed field
           npts:16,  // number of points, 0 means unknown
           efac:8 ,  // exponent for quantization mutliplier
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32q ;
typedef union{    // the union allows to transfer the whole 64 bit contents in one shot
  ieee32p   p  ;
  ieee32q   q  ;
  uint64_t  u  ;
} ieee32_props ;

void process_data_2d(void *data, int ni, int nj, error_stats *e0, char *name){
  float *f = (float *) data ;
  int i0, i, j0, j, in, jn, nblk, status, nbits ;
  float *t = (float *) malloc(ni*nj*sizeof(float)) ;
  float *t0 = t ;
  float *f0 = f ;
  float block0[64*64] ;
  float block1[64*64] ;
  int32_t block2[64*64] ;
  error_stats ee ;
  error_stats *e1 = &ee ;
  uint64_t h64 ;
  float quantum ;
  ieee32_props p64 ;
  int btab[33] ;
  int64_t nbtot, nbts ;

  fprintf(stderr, "allocated and initialized t[%d,%d]\n", ni, nj) ;
//   while((*name != '/') && (*name != '\0')) name++ ; if(*name == '/') name++ ;

  nblk = 0 ;
  nbtot = 0 ;
  for(i=0 ; i<33 ; i++) btab[i] = 0 ;
  update_error_stats(f, t, 0, e0) ;
  update_error_stats(f, t, 0, e1) ;
  for(j0=0 ; j0<nj ; j0+=BLK_J){
//     if(j0 >= 64) break ;
    jn = (j0+BLK_J < nj) ? BLK_J : nj-j0 ;
//     fprintf(stderr, "j [%4d,%4d] , i [", j0, j0+jn-1) ;
    for(i0=0 ; i0<ni ; i0+=BLK_I){
//       if(i0 >= 512) break ;
      nblk++ ;
      in = (i0+BLK_I < ni) ? BLK_I : ni-i0 ;
      // step 0 : get a block (64 x 64 or smaller)
//       get_word_block(INDEX_F(f,i0,ni,j0), block0, in, ni, jn) ;
      move_word32_block(INDEX_F(f,i0,ni,j0), in, block0, ni, ni, jn, raw_data, NULL) ;
      // step 1 : quantize
      bzero(block2, sizeof(block2)) ;
      quantum = .01f ; nbits = 0 ;
//       quantum = .00f ; nbits = 12 ;
//       h64 = IEEE32_linear_quantize_0(block0, in*jn, nbits, quantum, block2, NULL) ; p64.u = h64 ; nbts = p64.p.nbts ;
//       h64 = IEEE32_linear_quantize_1(block0, in*jn, nbits, quantum, block2) ; p64.u = h64 ; nbts = p64.q.nbts ;
      btab[nbts]++ ;
      nbtot += nbts*in*jn ;
      // step 2 : predict (Lorenzo)
      // step 3 : pseudo-encode
      // step 4 : unpredict
      // step 5 : restore
      bzero(block1, sizeof(block1)) ;
//       status = IEEE32_linear_restore_0(block2, h64, in*jn, block1) ;
//       status = IEEE32_linear_restore_1(block2, h64, in*jn, block1) ;
//       memcpy(block1, block0, sizeof(float)*in*jn) ;
      // step 6 : analyze
      update_error_stats(block0, block1, in*jn, e0) ;
// fprintf(stderr, "max error in block = %f (%d), avg error = %f\n", e0->max_error, e0->ndata, e0->abs_error/e0->ndata) ;
//       put_word_block(INDEX_F(t,i0,ni,j0), block1, in, ni, jn) ;
      move_word32_block(block1, ni, INDEX_F(t,i0,ni,j0), in, ni, jn, raw_data, NULL) ;
//       fprintf(stderr, "%4d,",i0) ;
    }
//     fprintf(stderr, "%4d]\n", i0) ;
  }
end:
  fprintf(stderr, "number of blocks = %d, bits/point = %f\n", nblk, 1.0*nbtot/ni/nj) ;
  fprintf(stderr, "btab = ") ; for(i=0 ; i<24 ; i++) fprintf(stderr, "%4d", btab[i]) ; fprintf(stderr, "\n") ;
  update_error_stats(f, t, -ni*nj, e1) ;

  e0->bias /= e0->ndata ; e0->abs_error /= e0->ndata ; e0->sum /= e0->ndata ;
  fprintf(stderr, "[e0] name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
          name[0], name[1], e0->ndata, e0->bias, e0->abs_error, e0->max_error, e0->sum) ;

  e1->bias /= e1->ndata ; e1->abs_error /= e1->ndata ; e1->sum /= e1->ndata ;
  fprintf(stderr, "[e1] name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
          name[0], name[1], e1->ndata, e1->bias, e1->abs_error, e1->max_error, e1->sum) ;
return ;

  fprintf(stderr, "allocated and initialized t[%d,%d]\n", ni, nj) ;
  for(i=0 ; i<ni*nj ; i++) t[i] = 0.0f ;

  update_error_stats(f, t, 0, e0) ;
  for(j0 = 0 ; j0<nj ; j0+=64){
    jn = (j0+64) > nj ? nj : j0+64 ;
    for(i0=0 ; i0<ni ; i0+=64){
      in = (i0+64) > ni ? ni : i0+64 ;
// ========================================================================
//    do something to data
#if 1
      for(i=0 ; i<4096 ; i++) block0[i] = 999999.0f ;
//       get_word_block(f+i0+j0*ni, block0, in-i0, ni, jn-j0) ;
      move_word32_block(f+i0+j0*ni, in-i0, block0, ni, ni, jn-j0, raw_data, NULL) ;
//    alter block0
      for(j=0 ; j<jn-j0 ; j++){
        for(i=0 ; i<in-i0 ; i++){
          block0[i+j*(in-i0)] +=  0.1f ;
        }
      }
//    put block0 back
//       put_word_block(t+i0+j0*ni, block0, in-i0, ni, jn-j0) ;
      move_word32_block(block0, ni, t+i0+j0*ni, in-i0, ni, jn-j0, raw_data, NULL) ;
#else
      for(j=j0 ; j<jn ; j++){
        for(i=i0 ; i<in ; i++){
        }
      }
      for(j=j0 ; j<jn ; j++){
        for(i=i0 ; i<in ; i++){
          t[i+j*ni] = f[i+j*ni] + 0.1f ;
        }; 
      }
#endif
// ========================================================================
      for(j=j0 ; j<jn ; j++){
        update_error_stats(f+i0+j*ni, t+i0+j*ni, in-i0, e0) ;
      }
    }
  }
  free(t) ;
  e0->bias /= e0->ndata ; e0->abs_error /= e0->ndata ; e0->sum /= e0->ndata ;
  while((*name != '/') && (*name != '\0')) name++ ;
  if(*name == '/') name++ ;
  fprintf(stderr, "name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
          name[0], name[1], e0->ndata, e0->bias, e0->abs_error, e0->max_error, e0->sum) ;
  fprintf(stderr, "freed t\n") ;
}

void analyze_float_data(void *fv, int n){
  float *ff = (float *) fv ;
  uint32_t *fi = (uint32_t *) fv ;
  double t, sum = 0.0, sum2 = 0.0, dev, avg, avgabs = 0.0 ;
  int i, i0, in ;
  float min = ff[0], max = ff[0] ;
  uint32_t exp[256], te ;

  for(i=0 ; i<256 ; i++) exp[i] = 0 ;
  for(i=0 ; i<n ; i++){
    t = ff[i] ;
    te = 0xFF & (fi[i] >> 23) ;  // IEEE exponent
    exp[te] ++ ;
    min = (t < min) ? t : min ;
    max = (t > max) ? t : max ;
    avgabs += ((t < 0) ? -t : t) ;
    sum += t ;
    sum2 += (t * t) ;
  }
  for(i0=1 ; i0<255 ; i0++) if(exp[i0] != 0) break ;
  for(in=255 ; in>0 ; in--) if(exp[in] != 0) break ;
  fprintf(stderr, ">           exponent range   = %d -> %d\n", i0, in) ;
  fprintf(stderr, ">           exponent pop/256 = ");
  for(i=i0 ; i<=in ; i++) fprintf(stderr, "%4d ", exp[i] >> 8) ;
  fprintf(stderr, "\n") ;
  avg = sum / n ;
  avgabs /= n ;
  dev = sum2 - 2.0 * avg * sum + n * avg * avg ;
  dev /= n ;
  dev = sqrt(dev) ;
  fprintf(stderr, ">           average = %10.3E |%10.3E|, std dev = %10.3E, min = %10.3E, max = %10.3E \n", avg, avgabs, dev, min, max) ;
}

void evaluate_abs_diff(float *rt, float *ft, int n, char *msg){
  uint32_t *irt = (uint32_t *) rt , *ift = (uint32_t *) ft ;
  float err, avgerr, bias, minflt = rt[0], maxflt = rt[0] ;
  int i, plus, minus, i0 = 0 ;

  err = avgerr = bias = 0.0f ;
  plus = minus = 0 ;
  for(i=0 ; i<n ; i++){
    float errloc = rt[i] - ft[i] ;
    if(errloc < 0) plus++ ; else minus ++ ;
    bias += errloc ;
    errloc = (errloc < 0) ? -errloc : errloc ;
    if(errloc > err){
      err =  errloc ;
      i0 = i ;
    }
    minflt = (rt[i] < minflt) ? rt[i] : minflt ;
    maxflt = (rt[i] > maxflt) ? rt[i] : maxflt ;
    avgerr += errloc ;
  }
  avgerr /= n ;
  bias /= n ;
  if(err == 0.0f){
    fprintf(stderr, "%s : no differences\n", msg) ;
    return ;
  }
  fprintf(stderr, "%s : max(avg) abs error = %12.5g(%12.5g), bias = %12.5e, max at %4d (%10g vs %10g), range = %10g",
                  msg, err, avgerr, bias, i0, rt[i0], ft[i0], maxflt-minflt) ;
  fprintf(stderr, ", + = %d, - = %d\n", plus, minus) ;
//   for(i=0 ; i<n ; i+=n/16) fprintf(stderr, "%8.8x ", (irt[i]^ift[i]) ) ; fprintf(stderr, "\n") ;
//   for(i=0 ; i<n ; i+=n/16) fprintf(stderr, "%8.8x ", ift[i]) ; fprintf(stderr, "\n") ;
}

void evaluate_rel_diff(float *rt, float *ft, int n, char *msg){
  uint32_t *irt = (uint32_t *) rt , *ift = (uint32_t *) ft ;
  float err, avgerr, bias ;
  int i, plus, minus, i0 = 0 ;
  err = avgerr = bias = 0.0f ;
  plus = minus = 0 ;
  for(i=0 ; i<n ; i++){
    float errloc = 1.0f - rt[i] / ft[i] ;
    if(ft[i] == 0.0f) continue ;
    if(errloc > .5f)continue ;   // ignore the result of initial values < qzero
    if(errloc < 0) plus++ ; else minus ++ ;
    bias += errloc ;
    errloc = (errloc < 0) ? -errloc : errloc ;
// if(errloc > 1.0f) fprintf(stderr, "i = %d, err = %12.5g, rt = %12.5g, ft = %12.5g\n", i, errloc, rt[i], ft[i]);
    if(errloc > err){
      err = errloc ;
      i0 = i ;
    }
    avgerr += errloc ;
  }
  avgerr /= n ;
  bias /= n ;
  if(err == 0.0f){
    fprintf(stderr, "%s : no differences\n", msg) ;
    return ;
  }
  fprintf(stderr, "%s : max(avg) rel error = %12.5g(%12.5g), 1 part in %9.3f(%9.3f), rel bias = %12.5e, max at %4d (%g vs %g)",
                  msg, err, avgerr, 1.0f/err,1.0f/avgerr, bias, i0, rt[i0], ft[i0] ) ;
  fprintf(stderr, ", + = %d, - = %d\n", plus, minus) ;
//   for(i=0 ; i<n ; i+=n/16) fprintf(stderr, "%8.8x ", (irt[i]^ift[i]) ) ; fprintf(stderr, "\n") ;
//   for(i=0 ; i<n ; i+=n/16) fprintf(stderr, "%8.8x ", ift[i]) ; fprintf(stderr, "\n") ;
}

int32_t count_bits(int32_t *q, int n){
  int i, count = 0, wired_or = 0 ;
  for(i=0 ; i<n ; i++) {
    wired_or |= q[i] ;
    if(q[i] >= 0) count += (32 -lzcnt_32(q[i])) ;
    if(q[i] <  0) count += (32 -lzcnt_32(-(int32_t)q[i])) ;
  }
  return count + ((wired_or < 0) ? n : 0) ;  // one extra bit per item if negative numbers are present
}

void test_log_quantizer(void *fv, int ni, int nj, error_stats *e, char *vn){
}

#define NPTSIJ 12
#define NPTS_TIMING 4096

int check_fake_log_1(void){
  float    f[NPTSIJ], fr[NPTSIJ] ;
  float ft[NPTS_TIMING], rt[NPTS_TIMING], xt[NPTS_TIMING], ft2[NPTS_TIMING], rt2[NPTS_TIMING] ;
  uint32_t q[NPTSIJ] ;
  int32_t qt[NPTS_TIMING] ;
  uint32_t *fi = (uint32_t *)f ;
  int i, diff ;
  q_rules r_f = {.ref = 1.0f, .rng10 = 0, .clip = 0, .type = Q_FAKE_LOG_1, .nbits = 0, .mbits = 6, .state = TO_QUANTIZE} ;
  q_encode qu_q ;
  q_decode qut_r ;
  TIME_LOOP_DATA ;
  float err, avgerr, bias ;
  int plus, minus ;
  limits_w32 l32, *l32p = &l32 ;

  fprintf(stderr, "==================== testing fake_log quantizer ====================\n");
goto long_test ;

  f[0] = 1.1 ; q[0] = 0xFFFFFFFFu ; fr[0] = 999999999 ;
  for(i=1 ; i<NPTSIJ ; i++){
    f[i] = 2.11f * f[i-1] ;
    q[i] = q[i-1] ;
    fr[i] = fr[i-1] ;
  }
  f[1] = .22f ;
  fprintf(stderr, "%d values, %12.5g <= value <=%12.5g\n", NPTSIJ, f[0], f[NPTSIJ-1]) ;

  l32 = IEEE32_extrema(f, NPTSIJ) ;
  fprintf(stderr, "IEEE32_fakelog_quantize_0\n");
  qu_q = IEEE32_fakelog_quantize_0(f, NPTSIJ, r_f, q, &l32, NULL) ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.5g", f[i]) ;
  fprintf(stderr, "\n") ;
  printf_quant_out(stderr, qu_q) ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.8x", q[i]) ;
  fprintf(stderr, "\n") ;

  fprintf(stderr, "IEEE32_fakelog_restore_0\n");
  qut_r = IEEE32_fakelog_restore_0(fr, NPTSIJ, qu_q, q) ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.5g", fr[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.7f", fr[i]/f[i]) ;
  fprintf(stderr, "\n") ;
  fprintf(stderr, "\n") ;

  fprintf(stderr, "IEEE32_fakelog_quantize_1\n");
  qu_q = IEEE32_fakelog_quantize_1(f, NPTSIJ, r_f, q, &l32, NULL) ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.5g", f[i]) ;
  fprintf(stderr, "\n") ;
  printf_quant_out(stderr, qu_q) ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.8x", q[i]) ;
  fprintf(stderr, "\n") ;

  fprintf(stderr, "IEEE32_fakelog_restore_1\n");
  qut_r = IEEE32_fakelog_restore_1(fr, NPTSIJ, qu_q, q) ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.5g", fr[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTSIJ ; i++) fprintf(stderr, "%12.7f", fr[i]/f[i]) ;
  fprintf(stderr, "\n") ;
  fprintf(stderr, "\n") ;
// return 0 ;

long_test:
  // clip to 0.0 values < 1.0f ;
  r_f = (q_rules){.ref = 1.001f, .rng10 = 0, .clip = 1, .type = Q_FAKE_LOG_1, .nbits = 0, .mbits = 6, .state = TO_QUANTIZE} ;
//   r_f = (q_rules){.ref = 0.000f, .rng10 = 0, .clip = 0, .type = Q_FAKE_LOG_1, .nbits = 0, .mbits = 6, .state = TO_QUANTIZE} ;
  ft[0] = 1.001f ;
//   ft[0] = r_f.ref ;
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00016f ;      // 1.001 <= value <=      1.9272
  for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00035f ;      // 1.001 <= value <=      4.1954
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00136f ;      // 1.001 <= value <=      261.56
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * -1.0018f ;      // 1.001 <= value <=      1579.9
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00203f ;      // 1.001 <= value <=        4046
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00254f ;      // 1.001 <= value <=       32503
  ft[1] = .22f ;   // under r_f.ref, to test clipping
  fprintf(stderr, "%d values, %12.5g <= value <=%12.5g\n\n", NPTS_TIMING, ft[0], ft[NPTS_TIMING-1] < 0 ? -ft[NPTS_TIMING-1] : ft[NPTS_TIMING-1]) ;

  l32 = IEEE32_extrema(ft, NPTS_TIMING) ;         // precompute data info for quantizer
goto cyclical ;

#if 0
  qu.u = IEEE32_fakelog_quantize_0(ft, NPTS_TIMING,  r_f, qt, &l32).u ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", ft[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", ft[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.8x", qt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.8x", qt[i]) ;
  fprintf(stderr, " [%d]", count_bits(qt, NPTS_TIMING)) ;
  fprintf(stderr, "\n") ;
  printf_quant_out(stderr, qu_q) ;
  for(i=0 ; i<NPTS_TIMING ; i++) rt[i] = 0.0f ;
  qut_r = IEEE32_fakelog_restore_0(rt, NPTS_TIMING, qu_q, qt) ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", rt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", rt[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", rt[i]/ft[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", rt[i]/ft[i]) ;
  fprintf(stderr, "\n") ;
  evaluate_rel_diff(rt, ft, NPTS_TIMING) ;
  fprintf(stderr, "\n") ;
#endif
  qu_q = IEEE32_fakelog_quantize_1(ft, NPTS_TIMING,  r_f, qt, &l32, NULL) ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.8x", qt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.8x", qt[i]) ;
  fprintf(stderr, " [%d]", count_bits(qt, NPTS_TIMING)) ;
  fprintf(stderr, "\n") ;
  printf_quant_out(stderr, qu_q) ;
  for(i=0 ; i<NPTS_TIMING ; i++) rt[i] = 0.0f ;
  qut_r = IEEE32_fakelog_restore_1(rt, NPTS_TIMING, qu_q, qt) ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", rt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", rt[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", rt[i]/ft[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", rt[i]/ft[i]) ;
  fprintf(stderr, "\n") ;
  evaluate_rel_diff(rt, ft, NPTS_TIMING, "") ;
  fprintf(stderr, "\n") ;
// return 0 ;
cyclical:
  fprintf(stderr, "\n") ;
  int cy, cycles = 5 ;
  r_f = (q_rules){.ref = 1.001f, .rng2 = 7, .clip = 1, .type = Q_FAKE_LOG_1, .nbits = 0, .mbits = 6, .state = TO_QUANTIZE} ;
  l32 = IEEE32_extrema(ft, NPTS_TIMING) ;         // precompute data info for quantizer

#if 0
  fprintf(stderr, "cyclical quantize->restore->quantize->restore test (fakelog_quantize_0)\n") ;
//   fprintf(stderr, "cyclical encoding differences = ") ;
  for(cy=0 ; cy<cycles ; cy++){
    qu.u = IEEE32_fakelog_quantize_0(rt, NPTS_TIMING,  r_f, qt, &l32).u ;  // quantize rt -> qt
    for(i=0 ; i<NPTS_TIMING ; i++) xt[i] = 0.0f ;
    qut_r = IEEE32_fakelog_restore_0(xt, NPTS_TIMING, qu_q, qt) ;        // decode qt -> xt
    diff = 0 ;
    for(i=0 ; i<NPTS_TIMING ; i++){
      diff += ((rt[i] != xt[i]) ? 1 : 0) ;                               // compare rt. qt
    }
    if(diff > 0) {
      evaluate_rel_diff(rt, xt, NPTS_TIMING) ;
//       evaluate_rel_diff(ft, xt, NPTS_TIMING) ;
    }
    fprintf(stderr, " [%4d]", diff) ;
    for(i=0 ; i<NPTS_TIMING ; i++) rt[i] = xt[i] ;                       // qt -> rt
  }
  fprintf(stderr, "\n") ;
  evaluate_rel_diff(ft, xt, NPTS_TIMING) ;
  fprintf(stderr, "\n") ;
#endif

  fprintf(stderr, "cyclical quantize->restore->quantize->restore test (fakelog_quantize_1)\n\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", ft[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", ft[i]) ;
  fprintf(stderr, "\n\n") ;
  qu_q = IEEE32_fakelog_quantize_1(ft, NPTS_TIMING,  r_f, qt, &l32, NULL) ;    // quantize ft -> qt
  qut_r = IEEE32_fakelog_restore_1(rt, NPTS_TIMING, qu_q, qt) ;          // restore  qt -> rt
  fprintf(stderr, "compare to original after first quantization\n");
  printf_quant_out(stderr, qu_q) ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.8x", qt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.8x", qt[i]) ;
  fprintf(stderr, " [%d]", count_bits(qt, NPTS_TIMING)) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", rt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", rt[i]) ;
  fprintf(stderr, "\n") ;
  evaluate_rel_diff(ft, rt, NPTS_TIMING, "") ;
  fprintf(stderr, "\n") ;
// return 0 ;
  fprintf(stderr, "compare to original after %d more rounds of quantization/restore\n", cycles) ;
  fprintf(stderr, "cyclical differences = ") ;
  for(cy=0 ; cy<cycles ; cy++){
    qu_q = IEEE32_fakelog_quantize_1(rt, NPTS_TIMING,  r_f, qt, &l32, NULL) ;  // quantize rt -> qt
    for(i=0 ; i<NPTS_TIMING ; i++) xt[i] = 0.0f ;
    qut_r = IEEE32_fakelog_restore_1(xt, NPTS_TIMING, qu_q, qt) ;        // restore  qt -> xt
    diff = 0 ;
    for(i=0 ; i<NPTS_TIMING ; i++) diff += ((rt[i] != xt[i]) ? 1 : 0) ;  // compare xt with rt
    if(diff > 0) evaluate_rel_diff(rt, xt, NPTS_TIMING, "") ;
    fprintf(stderr, " [%4d]", diff) ;
    for(i=0 ; i<NPTS_TIMING ; i++) rt[i] = xt[i] ;                       // xt -> rt
  }
  fprintf(stderr, "\n\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.8x", qt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.8x", qt[i]) ;
  fprintf(stderr, " [%d]", count_bits(qt, NPTS_TIMING)) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<4 ; i++)                       fprintf(stderr, "%12.5g", rt[i]) ;
  for(i=NPTS_TIMING-4 ; i<NPTS_TIMING ; i++) fprintf(stderr, "%12.5g", rt[i]) ;
  fprintf(stderr, "\n") ;
  evaluate_rel_diff(ft, xt, NPTS_TIMING, "") ;
  fprintf(stderr, "\n") ;

int timing = 1 ;
if(timing){
//   TIME_LOOP_EZ(1000, NPTS_TIMING, qu.u = IEEE32_fakelog_quantize_0(ft, NPTS_TIMING, r_f, qt, NULL).u ) ; // no precomputed info
//   fprintf(stderr, "fakelog_quantize_0      : %s\n",timer_msg);
// 
//   TIME_LOOP_EZ(1000, NPTS_TIMING, qu.u = IEEE32_fakelog_quantize_0(ft, NPTS_TIMING, r_f, qt, &l32).u ) ; // with precomputed info
//   fprintf(stderr, "fakelog_quantize_0(l32) : %s\n",timer_msg);
// 
//   TIME_LOOP_EZ(1000, NPTS_TIMING, qut_r = IEEE32_fakelog_restore_0(ft, NPTS_TIMING, qu_q, qt) ) ;
//   fprintf(stderr, "fakelog_restore_0       : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS_TIMING, qu_q = IEEE32_fakelog_quantize_1(ft, NPTS_TIMING, r_f, qt, NULL, NULL) ) ; // no precomputed info
  fprintf(stderr, "fakelog_quantize_1      : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS_TIMING, qu_q = IEEE32_fakelog_quantize_1(ft, NPTS_TIMING, r_f, qt, &l32, NULL) ) ; // with precomputed info
  fprintf(stderr, "fakelog_quantize_1(l32) : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS_TIMING, qut_r = IEEE32_fakelog_restore_1(ft, NPTS_TIMING, qu_q, qt) ) ;
  fprintf(stderr, "fakelog_restore_1       : %s\n",timer_msg);
}
  return 0 ;
}

void evaluate_int_diff(int32_t *ref, int32_t *new, int npts, char *msg){
  int i, count = 0 ;
  for(i=0 ; i<npts ; i++) if(ref[i] != new[i]) count++ ;
  fprintf(stderr, "%s : %d differences\n", msg, count) ;
}

void print_ints(int32_t *q, int npts, int nb){
  int i ;
  for(i=0      ; i<4    ; i++) fprintf(stderr, "%12.8x", q[i]) ;
  for(i=npts-4 ; i<npts ; i++) fprintf(stderr, "%12.8x", q[i]) ;
  if(nb > 0) fprintf(stderr, " [%d]", nb) ;
  fprintf(stderr, "\n") ;
}

void print_flts(float *f, int npts, int nb){
  int i ;
  for(i=0      ; i<4    ; i++) fprintf(stderr, "%12.5g", f[i]) ;
  for(i=npts-4 ; i<npts ; i++) fprintf(stderr, "%12.5g", f[i]) ;
  if(nb > 0) fprintf(stderr, " [%d]", nb) ;
  fprintf(stderr, "\n") ;
}

int check_linear(quantizer_fnptr qfunc, restore_fnptr rfunc, char *type, float *ft, int npts, q_rules r, float *rto){
  float rt[npts], xt[npts], ft2[npts], rt2[npts], ft0[npts] ;
  int32_t qt[npts], qt0[npts], qt2[npts] ;
  int i ;
  q_encode qu_q ;
  q_decode qut_r ;
  limits_w32 l32, *l32p = &l32 ;

  for(i=0 ; i<npts ; i++) ft0[i] = ft[i] ;
  fprintf(stderr, "==================== testing %s ====================\n", type);
  print_flts(ft, npts, 0) ;
  fprintf(stderr, "%d values, %12.5g <= value <=%12.5g\n\n", npts, ft[0], ft[npts-1]) ;
  l32 = IEEE32_extrema(ft, npts) ;         // precompute data info for quantizer

  for(i=0 ; i<npts ; i++) rt[i] = 0.0f ;
  qu_q = (*qfunc)(ft, npts,  r, qt, &l32, NULL) ;    // quantize ft -> qt
  for(i=0 ; i<npts ; i++) qt0[i] = qt[i] ;             // save quantized result
  qut_r = (*rfunc)(rt, npts, qu_q, qt) ;               // restore  qt -> rt

  print_ints(qt, npts, count_bits(qt, npts)) ;
  printf_quant_out(stderr, qu_q) ;
  print_flts(rt, npts, 0) ;
//   evaluate_rel_diff(ft, rt, npts, "rt VS ft") ;                    // compare rt with ft
  evaluate_abs_diff(ft, rt, npts, "rt VS ft") ;                    // compare rt with ft
  evaluate_rel_diff(ft, rt, npts, "rt VS ft") ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<npts ; i++) rto[i] = rt[i] ;
// return 0 ;
cyc_test:
  fprintf(stderr, "cyclical quantize->restore->quantize->restore test\n\n") ;
  print_flts(ft, npts, 0) ;
  fprintf(stderr, "%d values, %12.5g <= value <=%12.5g\n\n", npts, ft[0], ft[npts-1] < 0 ? -ft[npts-1] : ft[npts-1]) ;
  for(i=0 ; i<npts ; i++) rt2[i] = 0.0f ;
  qu_q = (*qfunc)(ft, NPTS_TIMING,  r, qt, &l32, NULL) ;    // quantize ft -> qt
  qut_r = (*rfunc)(rt2, NPTS_TIMING, qu_q, qt) ;              // restore  qt -> rt2
  fprintf(stderr, "compare to original after first quantization\n");
  printf_quant_out(stderr, qu_q) ;
  evaluate_int_diff(qt0, qt, npts, "qt0 VS qt2") ;             // compare qt2 with original qt
//   evaluate_rel_diff(ft, rt2, npts, "ft VS rt2 ") ;             // compare rt2 with original ft
//   evaluate_rel_diff(rt, rt2, npts, "rt VS rt2 ") ;             // compare rt2 with original rt
  evaluate_abs_diff(ft, rt2, npts, "ft VS rt2 ") ;             // compare rt2 with original ft
  evaluate_abs_diff(rt, rt2, npts, "rt VS rt2 ") ;             // compare rt2 with original rt
  fprintf(stderr, "\n") ;

  int cy, cycles = 3 ;
  fprintf(stderr, "compare to first round restore, %d more rounds of quantization/restore\n", cycles) ;
  for(cy=0 ; cy<cycles ; cy++){
    qu_q = (*qfunc)(rt2, NPTS_TIMING,  r, qt2, &l32, NULL) ;  // quantize rt2 -> qt2
    for(i=0 ; i<NPTS_TIMING ; i++) xt[i] = 0.0f ;
    qut_r = (*rfunc)(xt, NPTS_TIMING, qu_q, qt2) ;              // restore  qt2 -> xt
//     evaluate_rel_diff(rt, xt, NPTS_TIMING, "xt VS rt ") ;
    evaluate_abs_diff(rt, xt, NPTS_TIMING, "xt VS rt ") ;
    for(i=0 ; i<NPTS_TIMING ; i++) rt2[i] = xt[i] ;             // xt -> rt2
  }

  return 0 ;
}

#define NPTSI 100
#define NPTSJ 100

int test_linear_log(){
  float ft[NPTS_TIMING], rt0[NPTS_TIMING], rt1[NPTS_TIMING], rt2[NPTS_TIMING], rt3[NPTS_TIMING] ;
  q_rules r_f ;
  int i ;

  ft[0] = 1.001f ;
  for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00016f ;      // 1.001 <= value <=      1.9272
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00035f ;      // 1.001 <= value <=      4.1954
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00111f ;      // 1.001 <= value <=      94.043
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00131f ;      // 1.001 <= value <=      213.13
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00136f ;      // 1.001 <= value <=      261.56
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * -1.0018f ;      // 1.001 <= value <=      1579.9
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00203f ;      // 1.001 <= value <=        4046
//   for(i=1 ; i<NPTS_TIMING ; i++) ft[i] = ft[i-1] * 1.00254f ;      // 1.001 <= value <=       32503
//   ft[1] = .22f ;   // under r.f.ref, to test clipping
for(i=0 ; i<NPTS_TIMING ; i++) ft[i] = i * 7.97f / NPTS_TIMING + 3123.0f ;

//   if(check_fake_log_1()) goto error ;
  r_f = (q_rules) {.ref = 0.0f, .rng10 = 0, .clip = 0, .type = 0, .nbits = 8, .mbits = 0, .state = TO_QUANTIZE} ;

//   r_f.type = Q_LINEAR_0 ;
//   if(check_linear(IEEE32_linear_quantize_0, IEEE32_linear_restore_0, "linear_quantize_0", ft, NPTS_TIMING, r_f, rt0)) goto error ;
//   evaluate_abs_diff(rt0, rt0, NPTS_TIMING, "rt0 VS rt0") ;

  r_f.type = Q_LINEAR_1 ;
  if(check_linear(IEEE32_linear_quantize_1, IEEE32_linear_restore_1, "linear_quantize_1", ft, NPTS_TIMING, r_f, rt1)) goto error ;
//   evaluate_abs_diff(rt0, rt1, NPTS_TIMING, "rt0 VS rt1") ;

  r_f.type = Q_LINEAR_2 ;
  if(check_linear(IEEE32_linear_quantize_2, IEEE32_linear_restore_2, "linear_quantize_2", ft, NPTS_TIMING, r_f, rt2)) goto error ;

  r_f.type  = Q_FAKE_LOG_1 ;
//   r_f.mbits = 9 ;
  if(check_linear(IEEE32_fakelog_quantize_1, IEEE32_fakelog_restore_1, "fakelog_quantize_1", ft, NPTS_TIMING, r_f, rt3)) goto error ;
//   evaluate_abs_diff(rt0, rt3, NPTS_TIMING, "rt0 VS rt3") ;

  return 0 ;

error:
  return 1 ;
}
#if 0
static int index_ij(int i, int j, int ni, q_rules r){
  return (i + j * ni) ;
}
#endif
static int32_t decode_block(void *decoded, int ni, int nj, bitstream *encoded){
  int nbits_tot = 0 ;
  nbits_tot = decode_as_tiles(decoded, ni, ni, nj, encoded) ;
  return nbits_tot ;
}

static int32_t encode_block(void *decoded, int ni, int nj, bitstream *encoded){
  int nbits_tot = 0 ;
  nbits_tot = encode_as_tiles(decoded, ni, ni, nj, encoded) ;
  return nbits_tot ;
}

static int predict_nbits_avant = 0 ;
static int predict_nbits_apres = 0 ;

int count_encoded_bits(int32_t *q, int ni, int lni, int nj);

static int32_t unpredict_block(void *f, int ni, int nj, void *p){
  int32_t *p_i = (int32_t *) p ;
  int32_t *r_i = (int32_t *) f ;
  int i, nbits_tot = 0 ;

  for(i=0 ; i<ni*nj ; i++) r_i[i] = 0 ;
  LorenzoUnpredict(f, p, ni, ni, ni, nj);
  // count bits needed for restored block
  nbits_tot = count_encoded_bits(f, ni, ni, nj) ;
  return nbits_tot ;
}

static int32_t predict_block(void *f, int ni, int nj, void *p){
  int32_t *p_i = (int32_t *) p ;
  int32_t *f_i = (int32_t *) f ;
  int32_t fr[ni*nj] ;
  int nbits_tot = 0 ;
  int i, avant, apres ;

  // apply predictor
  avant = count_encoded_bits(f, ni, ni, nj) ;
  predict_nbits_avant += avant ;
  for(i=0 ; i<ni*nj ; i++) fr[i] = 123456789 ;
  LorenzoPredict(f, p, ni, ni, ni, nj);

  apres = unpredict_block(fr, ni, nj, p);  // check that restore has no errors
  if(avant != apres){
    fprintf(stderr, "avant(%d)/apres(%d) error\n", avant, apres);
    exit(1) ;
  }
//   LorenzoUnpredict(fr, p, ni, ni, ni, nj);
  for(i=0 ; i<ni*nj ; i++) if(fr[i] != f_i[i]) {
    fprintf(stderr, "predictor/restore error\n");
    exit (1) ;
  }
// for(i=0 ; i<16 ; i++) fprintf(stderr, "%8i", f_i[i]) ; fprintf(stderr, "\n");
// for(i=0 ; i<16 ; i++) fprintf(stderr, "%8i", p_i[i]) ; fprintf(stderr, "\n");
  // count bits needed
  for(i=0 ; i<ni*nj ; i++){
    uint32_t temp = (p_i[i] < 0) ? -p_i[i] : p_i[i] ;
    nbits_tot += (33 - lzcnt_32(temp)) ;
  }
  predict_nbits_apres += count_encoded_bits(p, ni, ni, nj) ;
  return nbits_tot ;
}

int32_t count_bits_of_max(int32_t *q, int n){
  int i, count = 0, wired_or = 0 ;
  int temp ;
  int maxa = 0 ;
  for(i=0 ; i<n ; i++) {
    temp = q[i] ;
    wired_or |= temp ;
    temp = (temp < 0) ? (-temp) : temp ;
    maxa = (temp > maxa) ? temp : maxa ;
  }
  count = n * ( 32 - lzcnt_32(maxa)) ;
  return count + ((wired_or < 0) ? n : 0) ;  // one extra bit per item if negative numbers are present
}

int count_encoded_bits(int32_t *q, int ni, int lni, int nj){
  int i0, j0, i1, j1, i, base, nbp, nbits, tot_bits = 0 ;
  int32_t block[64] ;

  for(j0 = 0 ; j0 < nj ; j0+= 8){
    j1 = (j0 + 8) > nj ? nj : (j0 + 8) ;
    base = j0 * lni ;
    for(i0 = 0 ; i0 < ni ; i0 += 8){
      i1 = (i0 + 8) > ni ? ni : (i0 + 8) ;
//       get_word_block(q + base, block, i1 - i0, ni, j1 - j0) ;     // get 8 x 8 block
      move_word32_block(q + base, i1-i0, block, ni, ni, j1-j0, raw_data, NULL) ;
      nbp = (j1 - j0) * (i1 - i0) ;                               // number of points in block
      tot_bits += count_bits_of_max(block, nbp) ;
      tot_bits += 16 ;                                            // block overhead
      base += 8 ;
    }
  }
  return tot_bits ;
}

static q_encode qe ;   // last encoded block
static q_decode qd ;   // last decoded block

static int32_t dequantize_block(void *f, int ni, int nj, void *q, q_rules r){
  int nbits_tot = 0 ;
  int np = ni * nj ;

  if(qe.state != QUANTIZED){
//     fprintf(stderr, "bad state after quantize : %d\n", qe.state) ;
    return 0 ;
  }

  switch(qe.type)
  {
    case Q_LINEAR_0:
      qd = IEEE32_linear_restore_0(f, np, qe, q) ;
      break ;
    case Q_LINEAR_1:
      qd = IEEE32_linear_restore_1(f, np, qe, q) ;
      break ;
    case Q_LINEAR_2:
      qd = IEEE32_linear_restore_2(f, np, qe, q) ;
      break ;
    case Q_FAKE_LOG_1:
      qd = IEEE32_fakelog_restore_1(f, np, qe, q) ;
      break ;
    default:
      fprintf(stderr, "unrecognized quantizer : %d\n", r.type) ;
      return 0 ;
  }
  if(qd.state != RESTORED) {
//     fprintf(stderr, "bad state after restore : %d\n", qd.state) ;
    return 0 ;
  }
  return count_encoded_bits(q, ni, ni, nj) ;
}

static int32_t quantize_block(void *f, int ni, int nj, void *q, q_rules r){
  int nbits_tot = 0 ;
  int np = ni * nj ;

  switch(r.type)
  {
    case Q_LINEAR_0:
      qe = IEEE32_linear_quantize_0(f, np, r, q, NULL, NULL) ;
      break ;
    case Q_LINEAR_1:
      qe = IEEE32_linear_quantize_1(f, np, r, q, NULL, NULL) ;
      break ;
    case Q_LINEAR_2:
      qe = IEEE32_linear_quantize_2(f, np, r, q, NULL, NULL) ;
      break ;
    case Q_FAKE_LOG_1:
      qe = IEEE32_fakelog_quantize_1(f, np, r, q, NULL, NULL) ;
      break ;
    default:
      fprintf(stderr, "unrecognized quantizer : %d\n", r.type) ;
      return -1 ;
  }

  if(qe.state != QUANTIZED){
//     fprintf(stderr, "bad state after quantize : %d\n", qe.state) ;
    return -1 ;
  }
  return count_encoded_bits(q, ni, ni, nj) ;
}

static float update_abs_err(float maxabserr, float *ref, float *new, int np){
  int i ;
  for(i=0 ; i<np ; i++){
    float temp = ref[i] - new[i] ;
    temp = (temp < 0) ? -temp : temp ;
    maxabserr = (temp > maxabserr) ? temp : maxabserr ;
  }
  return maxabserr ;
}

static float min_float(float *f, int n){
  float min = f[0] ;
  int i ;
  for(i=0 ; i<n ; i++) min = (f[i] < min) ? f[i] : min ;
  return min ;
}
static float max_float(float *f, int n){
  float max = f[0] ;
  int i ;
  for(i=0 ; i<n ; i++) max = (f[i] > max) ? f[i] : max ;
  return max ;
}

static int diff_count(void *f1, void *f2, int n){
  int i, diff = 0 ;
  int32_t *p1 = (int32_t *) f1 ;
  int32_t *p2 = (int32_t *) f2 ;
  for(i=0 ; i<n ; i++) diff = diff + ((p1[i] != p2[i]) ? 1 : 0) ;
  return diff ;
}

typedef struct{
  uint32_t stripe_len ;
  uint32_t block_len[] ;
} stripe_map ;

int compress_2d_field(float *f, int ni, int nj, q_rules r){
  int nbits_tot = 0 ;
  int i0, j0, i1, j1, i, j, base, iblk, tot_bits, k, enc_bits, dec_bits, tbits, tot_pbits ;
  int blocki = 64, blockj = 64 ;
  float   fblk[blocki * blockj] ;  // float block
  int32_t qblk[blocki * blockj] ;  // quantized block
  float   rblk[blocki * blockj] ;  // restored block
  int32_t pblk[blocki * blockj] ;  // predicted block
  int32_t ublk[blocki * blockj] ;  // unpredicted block
  int32_t eblk[blocki * blockj] ;  // encoded block
  int32_t dblk[blocki * blockj] ;  // decoded block
  float abserr, maxabserr = 0.0f ;
  float relerr, maxrelerr = 0.0f ;
  float referr ;
  float gblk[ni*nj], g[ni*nj] ;    // global arrays
  int32_t fq[ni*nj] ;              // globally quantized array

  uint32_t nbi, nbj, nbi2 ;
  nbi = (ni + blocki - 1) / blocki ; nbi2 = (nbi + 1) >> 1 ;
  nbj = (nj + blockj - 1) / blockj ;
  int32_t qbts[nbi * nbj] ;        // bits used by quantizer
  float qbts_p[nbi * nbj] ;        // bits/point used by quantizer
  int32_t pbts[nbi * nbj] ;        // bits used by predictor
  int32_t ebts[nbi * nbj] ;        // bits used by encoder
// variable length array in structure not supported by aocc
//   struct{
//     uint32_t stripe_len ;
//     uint32_t block_len[nbi2] ;
//   } dmap[nbj] ;
//   for(j=0 ; j<nbj ; j++){
//     dmap[j].stripe_len = 0 ;
//     for(i=0 ; i<nbi2 ; i++) dmap[j].block_len[i] = 0 ;
//   }

  for(i=0 ; i<nbi*nbj ; i++){
    qbts[i] = 0 ;
    pbts[i] = 0 ;
    ebts[i] = 0 ;
  }
  fprintf(stderr, "ref = %f, nbi = %d, nbj = %d\n", r.ref, nbi, nbj) ;
  for(i=0 ; i<ni*nj ; i++) g[i] = 999.0f ;
//   tot_bits = quantize_block(f, ni, nj, fq, r, g) ;
  enc_bits =   quantize_block(f, ni, nj, fq, r) ;
  if(enc_bits != dequantize_block(g, ni, nj, fq, r)) exit(1) ;
//   enc_bits = count_encoded_bits(fq, ni, ni, nj) ;
//   tot_bits = count_bits(fq, ni*nj) ;
//   enc_bits = count_bits_of_max(fq, ni*nj) ;
  referr = update_abs_err(0.0f, f, g, ni*nj) ;
//   evaluate_abs_diff(f, g, ni*nj, "global f VS g") ; 
  fprintf(stderr, "total bits = %d, %4.1f bits/point, max abs err = %12.5g(%f)\n", enc_bits, enc_bits*1.0f/ni/nj, referr, r.ref) ;
  for(i=0 ; i<ni*nj ; i++) g[i] = 0 ;

// goto end ;
  iblk = 0 ;                       // block number
  int oddities = 0 ;
  int nbp, nbpi, nbpj, bi, bj ;
  int nbits1, nbits2, ndiff ;
  tot_pbits = tot_bits = enc_bits = 0 ;
  predict_nbits_apres = predict_nbits_avant = 0 ;

  bitstream bstream ;
  BeStreamInit(&bstream, NULL, ni*nj*sizeof(int32_t), BIT_INSERT | BIT_XTRACT | BIT_FULL_INIT) ;
  print_stream_params(bstream, "Init bstream", "RW") ;

  for(j0=0, bj=0 ; j0<nj ; j0+=blockj, bj++){
    j1 = j0 + blockj ; j1 = (j1 > nj) ? nj : j1 ;
    base = j0 * ni ;
    // uint32_t packed[ni * blockj]    // worst possible case
    // stripe_map map ;
    // BeStreamInit(&stream0, packed_, sizeof(packed_), 0) ;  // packed_ has to be large enough for a stripe (ni * blockj)
    for(i0=0, bi=0 ; i0<ni ; i0+=blocki, bi++){
      i1 = i0 + blocki ; i1 = (i1 > ni) ? ni : i1 ;
      nbpi = i1 - i0 ;
      nbpj = j1 - j0 ;
      nbp = nbpj * nbpi ;

//       get_word_block(f + base, fblk, nbpi, ni, nbpj) ;           // get block
      move_word32_block(f + base, nbpi, fblk, ni, ni, nbpj, raw_data, NULL) ;
      qbts[iblk] = quantize_block(fblk, nbpi, nbpj, qblk, r) ;   // quantize according to rules
//       put_word_block(g    + base, fblk, nbpi, ni, nbpj) ;        // put original block into g
      move_word32_block(fblk, ni, g + base, nbpi, ni, nbpj, raw_data, NULL) ;
      if(qbts[iblk] < 0) {
        fprintf(stderr, "error while quantizing or restoring\n") ;
        exit(1) ;                                                // error while quantizing or restoring
      }
      tot_bits += qbts[iblk] ;                                   // cumulative total
      tot_bits += 128 ;                                          // add block header overhead

      pbts[iblk] = predict_block(qblk, nbpi, nbpj, pblk) ;        // apply predictor (Lorenzo)

      // align stream to 32 bit boundary (primitive to do so needed in bi_endian_pack.c or bi_endian_pack.h)
      // loop over tiles
      //   nbtot = encode_tile(tile3, ti, lti, tj, &stream0, temp) ;  // temp == space for largest tile (64)
      nbits1 = encode_block(pblk, nbpi, nbpj, &bstream) ;                      // encode predicted block
//       print_stream_params(bstream, "after encode", "RW") ;
//       LeStreamRewind(&bstream, 0) ;
      nbits2 = decode_block(dblk, nbpi, nbpj, &bstream) ;                      // decode to restore predicted block
//       print_stream_params(bstream, "after decode", "RW") ;
      ndiff = diff_count(pblk, dblk, nbpi*nbpj) ;              // check that encode/decode was lossless
      if(ndiff != 0 || nbits2 != nbits1){
        fprintf(stderr, "ERROR: nbits1 = %d, nbits2 = %d, ndiff = %d, i0 = %d, j0 = %d, nbpi = %d, nbpj= %d\n", nbits1, nbits2, ndiff, i0, j0, nbpi, nbpj) ;
        exit(1) ;
      }
      StreamReset(&bstream) ;
//       BeStreamInit(&bstream, NULL, ni*nj*sizeof(int32_t), BIT_INSERT | BIT_XTRACT) ;

      dec_bits = unpredict_block(ublk, nbpi, nbpj, pblk) ;        // unpredict block
      if(diff_count(qblk, ublk, nbpi*nbpj) != 0){                 // check that we get back the original block
        fprintf(stderr, "predict/unpredict mismatch\n") ;
        exit(1) ;
      }
      if(qbts[iblk] != dec_bits){
        fprintf(stderr, "bit count mismatch after unpredict, expected %d, got %d\n", qbts[iblk]-128, dec_bits) ;
        exit(1) ;
      }

      if(qbts[iblk] != dequantize_block(rblk, nbpi, nbpj, ublk, r)) exit(1) ;  // dequantize unpredicted block
//       put_word_block(gblk + base, rblk, nbpi, ni, nbpj) ;  // put restored block into gblk
      move_word32_block(rblk, ni, gblk + base, nbpi, ni, nbpj, raw_data, NULL) ;

      qbts_p[iblk] = qbts[iblk] * 1.0f / nbp ;                   // bits per point
//       enc_bits += count_encoded_bits(qblk, nbpi, nbpi, nbpj) ;
      abserr = update_abs_err(0.0f, fblk, rblk, nbp) ;
      maxabserr = (abserr > maxabserr) ? abserr : maxabserr ;
      if(abserr > r.ref && r.ref > 0.0f) oddities++ ;                            // absolute error should not be larger than reference

//       ebts[iblk] = encode_block(qblk, ni, nj) ;                           // pseudo encoder (discard output stream)
      base += blocki ;
      iblk++ ;
    }
  }

  iblk = 0 ;
//   for(j0=0 ; j0<nj ; j0+=blockj){
//     for(i0=0 ; i0<ni ; i0+=blocki){
//       fprintf(stderr, "%3.1f ", qbts_p[iblk]) ;
//       iblk++ ;
//     }
//     fprintf(stderr, "\n") ;
//   }
//   enc_bits = enc_bits + 128 * nbi * nbj ;  // add block headers 
  fprintf(stderr, "total bits = %d, %4.1f bits/point, max abs err = %12.5g(%f), oddities = %d\n",
          tot_bits, tot_bits * 1.0f / ni / nj, maxabserr, r.ref, oddities) ;
//           tot_bits, tot_bits * 1.0f / ni / nj, maxabserr, r.ref, oddities) ;
  evaluate_abs_diff(f, gblk, ni*nj, "blocked f VS gblk") ;
  fprintf(stderr, "gain(<0 = loss) through prediction = %5.2f, before = %5.2f, after = %5.2f\n",
          (predict_nbits_avant - predict_nbits_apres) * 1.0f / (ni*nj) , 
          predict_nbits_avant * 1.0f / ni / nj,
          predict_nbits_apres * 1.0f / ni / nj) ;
//   evaluate_abs_diff(f, g   , ni*nj, "blocked f VS g   ") ;
//   for(j0=0 ; j0<nj ; j0+=blockj){
//     for(i0=0 ; i0<ni ; i0+=blocki){
//       fprintf(stderr, "%3.1f ", pbts[iblk] * 1.0 / blocki / blockj) ;
//       iblk++ ;
//     }
//     fprintf(stderr, "\n") ;
//   }

//   for(j0=0 ; j0<nj ; j0+=blockj){
//     for(i0=0 ; i0<ni ; i0+=blocki){
//       fprintf(stderr, "%3d ", (qbts[iblk] - pbts[iblk])/2 ) ;
//       iblk++ ;
//     }
//     fprintf(stderr, "\n") ;
//   }
end:
  fprintf(stderr, "\n") ;
  return tot_bits ;
}

static uint32_t code_var(char *nomvar, int len){
  int k ;
  uint32_t codevar = 0 ;
  for(k=0 ; k<len ; k++) codevar = (codevar << 8) + nomvar[k] ;
  return codevar ;
}

static void print_rules(q_rules r, char *name){
  fprintf(stderr, "q rules (%4s) : ", name) ;
  if(r.type >=0 && r.type <= 4) fprintf(stderr, "type = %s", Q_NAMES[r.type]) ;
  if(r.rng2  != 0)  fprintf(stderr, ", rng2 = %2d ", r.rng2) ;
  if(r.rng10 != 0)  fprintf(stderr, ", rng10 = %2d ", r.rng10) ;
  if(r.nbits != 0)  fprintf(stderr, ", nbits = %2d ", r.nbits) ;
  if(r.mbits != 0)  fprintf(stderr, ", mbits = %2d ", r.mbits) ;
  if(r.clip  != 0)  fprintf(stderr, ", clip = %1d ", r.clip) ;
  if(r.ref != 0.0f) fprintf(stderr, ", R = %g ", r.ref) ;
  fprintf(stderr, "\n");
}

static q_rules make_rules(char *nomvar){
  q_rules r = q_desc_0.f ;
  uint32_t code = code_var(nomvar, strnlen(nomvar,4)) ;
  if(code == code_var("TT", 2)){
    r = (q_rules) { .ref = .01f, .type = Q_LINEAR_2 } ;
  }else if(code == code_var("TD", 2)){
    r = (q_rules) { .ref = .01f, .type = Q_LINEAR_2 } ;
  }else if(code == code_var("ES", 2)){
    r = (q_rules) { .ref = .01f, .type = Q_LINEAR_2 } ;
  }else if(code == code_var("HU", 2)){
    r = (q_rules) { .type = Q_FAKE_LOG_1, .rng10 = 4, .mbits = 10 } ;
  }else if(code == code_var("UU", 2)){
    r = (q_rules) { .ref = .05f, .type = Q_LINEAR_2 } ;
  }else if(code == code_var("VV", 2)){
    r = (q_rules) { .ref = .05f, .type = Q_LINEAR_2 } ;
  }else if(code == code_var("QQ", 2)){
    r = (q_rules) { .type = Q_FAKE_LOG_1, .rng10 = 4, .mbits = 10 } ;
  }else if(code == code_var("QR", 2)){
    r = (q_rules) { .type = Q_FAKE_LOG_1, .rng10 = 4, .mbits = 10 } ;
  }else if(code == code_var("DD", 2)){
    r = (q_rules) { .type = Q_FAKE_LOG_1, .rng10 = 4, .mbits = 10 } ;
  }else if(code == code_var("WW", 2)){
//     r = (q_rules) { .type = Q_FAKE_LOG_1, .rng10 = 3, .mbits = 10 } ;
    r = (q_rules) { .nbits = 10, .type = Q_LINEAR_1 } ;
  }else if(code == code_var("GZ", 2)){
    r = (q_rules) { .ref = .005f, .type = Q_LINEAR_0 } ;
//     r = (q_rules) { .nbits = 9, .type = Q_LINEAR_0 } ;
  }else if(code == code_var("ZZ", 2)){
    r = (q_rules) { .ref = .002f, .type = Q_LINEAR_1 } ;  // problems with Q_LINEAR_0, max error too large
  }else{
  }
  r.state = TO_QUANTIZE ;
  return r ;
}

int main(int argc, char **argv){
  int dims[10] ;
  int dim3[] = { 1,2,3 } ;
  int buf3[6] ;
  int ndim = 0 ;
  int fd = 0 ;
  int i, j, status ;
  void *buf ;
  int ndata, test_mode = -1, maxrec = 999999, nrec = 0 ;
  error_stats e ;
  bitstream **streams ;
  compressed_field field ;
  compress_rules rules ;
  char ca ;
  char *nomvar = "ZzZz" ;
  uint32_t codevar = 0 ;
  char vn[5] ;
  TIME_LOOP_DATA ;
  float ref[NPTSJ], new[NPTSJ] ;
  q_rules q_r ;

  if(argc < 2){
    fprintf(stderr, "usage: %s [-ttest_mode] [-nnb_rec] [-vvar_name] file_1 ... [[-ttest_mode] [-nnb_var] [-vvar_name] file_n]\n", argv[0]) ;
    fprintf(stderr, "ex: %s -t0 -n4 -vDD path/my_data_file\n", argv[0]) ;
    fprintf(stderr, "    test 0, first 4 records of variable DD from file path/my_data_file\n") ;
    exit(1) ;
  }
  start_of_test(argv[0]);

//   for(j=0 ; j<NPTSJ ; j++){
//     for(i=0 ; i<NPTSI ; i++){
//       ref[i+j*NPTSJ] = 1.0f ;
//       new[i+j*NPTSJ] = 1.1f ;
//     }
//   }
//   update_error_stats(ref, new, 0, &e) ;
//   ndata = update_error_stats(ref, new, -NPTSI*NPTSJ, &e) ;
//   e.bias /= e.ndata ; e.abs_error /= e.ndata ; e.sum /= e.ndata ;
//   fprintf(stderr, "ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", e.ndata, e.bias, e.abs_error, e.max_error, e.sum) ;
//   process_data_2d(ref, NPTSI, NPTSJ, &e, "/XX") ;
//   e.bias /= e.ndata ; e.abs_error /= e.ndata ; e.sum /= e.ndata ;
//   fprintf(stderr, "ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", e.ndata, e.bias, e.abs_error, e.max_error, e.sum) ;

  for(i=1 ; i<argc ; i++){
    ca = *argv[i] ;
    if( ca == '-' ){
      ca = argv[i][1] ;
//       fprintf(stderr, "option %c = '%s'[%d] ", ca, argv[i]+2, strlen(argv[i]+2));
      if(ca == 't') {
        test_mode = atoi(argv[i]+2) ;
        fprintf(stderr, "test mode = %d ", test_mode) ;
        if(test_mode == 100) {
          status = test_linear_log() ;
          return status ;
        }
        if(test_mode == 101) {
          status = check_fake_log_1() ;
          return status ;
        }
      }
      if(ca == 'n') {
        maxrec = atoi(argv[i]+2) ;
        fprintf(stderr, "maxrec = %d ", maxrec) ; 
      }
      if(ca == 'v') {
        nomvar = argv[i] + 2 ; codevar = code_var(nomvar, strlen(nomvar)) ;
        fprintf(stderr, "variable = '%s'[%lu], code = %8.8x, ", nomvar, strlen(nomvar), codevar) ;
      }
      continue ;
    }
    fd = 0 ;
    ndim = 0 ;
    nrec = 0 ;
    vn[0] = vn[1] = vn[2] = vn[3] = ' ' ; vn[4] = '\0' ;
    buf = read_32bit_data_record_named(argv[i], &fd, dims, &ndim, &ndata, vn) ;
    fprintf(stderr, " file = '%s', fd = %d\n\n", argv[i], fd) ;
    while(buf != NULL && nrec < maxrec){
      nrec++ ;
      if(strncmp(nomvar, vn, strnlen(nomvar,4)) != 0) {
        fprintf(stderr, "'%s'(%8.8x) and '%s'(%8.8x) do not match\n", nomvar, codevar, vn, code_var(vn,strnlen(nomvar,4))) ;
        continue ;
      }
//       fprintf(stderr, "dimensions = %d : (", ndim) ;
      fprintf(stderr, "%c%c%c%c(", vn[0], vn[1], vn[2], vn[3]) ;
      for(j=0 ; j<ndim ; j++) fprintf(stderr, "%d ", dims[j]) ;
      fprintf(stderr, ") [%d] (%f -> %f)\n", ndata, min_float(buf, dims[0]*dims[1]), max_float(buf, dims[0]*dims[1])) ;
//       for(j=0 ; j<ndim ; j++) fprintf(stderr, "%d ", dims[j]) ; fprintf(stderr, ") [%d], name = '%c%c%c%c'\n", ndata, vn[0], vn[1], vn[2], vn[3]);
      if(ndim == 2){    // 2 dimensional data only
        switch(test_mode)
        {
          case 0:  // analyze data
//             fprintf(stderr, "analysis test\n");
            analyze_float_data(buf, dims[0] * dims[1]) ;
            break;
          case 1:  // linear quantizer test
//             process_data_2d(buf, dims[0], dims[1], &e, nomvar ? nomvar : argv[i]) ;
            break;
          case 2:  // fake log quantizer test
//             test_log_quantizer(buf, dims[0], dims[1], &e, vn) ;
            break;
          case 3:  // compression test
            q_r = make_rules(nomvar) ;
            print_rules(q_r, nomvar) ;
            compress_2d_field(buf, dims[0], dims[1], q_r) ;
//             field = compress_2d_data(buf, dims[0], dims[0], dims[1], rules) ;
            break;
          default:
            fprintf(stderr, "unrecognized test mode %d\n", test_mode) ;
            exit(1) ;
        }
      }
      free(buf) ;    // done with this record
// break ;   // only one record for now
//       fprintf(stderr, "reading next record, fd = %d\n", fd);
      vn[0] = vn[1] = vn[2] = vn[3] = ' ' ;
      buf = read_32bit_data_record_named("", &fd, dims, &ndim, &ndata, vn) ;
    }
    nomvar = NULL ;
    if(ndata == 0 || nrec >= maxrec) {
      if(ndata == 0) fprintf(stderr, "end of file '%s', close status = %d\n", argv[i], close(fd)) ;
      if(ndata != 0) fprintf(stderr, "closing file '%s', close status = %d\n", argv[i], close(fd)) ;
    }else{
      fprintf(stderr, "error reading record\n") ;
    }
  }
  return 0 ;

error:
  return 1 ;
}
