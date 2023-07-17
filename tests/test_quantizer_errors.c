#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>

#include <rmn/test_helpers.h>
#include <rmn/misc_operators.h>
#include <rmn/c_record_io.h>
#include <rmn/ieee_quantize.h>
#include <rmn/compress_data.h>
#include <rmn/compress_data.h>
#include <rmn/eval_diff.h>

#define INDEX_F(F, I, LNI, J) (F + I + J*LNI)

typedef struct{
  uint64_t bias:32,  // minimum absolute value (actually never more than 31 bits)
           npts:16,  // number of points, 0 means unknown
           resv:3 ,
           shft:5 ,  // shift count (0 -> 31) for quanze/unquantize
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
      get_word_block(INDEX_F(f,i0,ni,j0), block0, in, ni, jn) ;
      // step 1 : quantize
      bzero(block2, sizeof(block2)) ;
      quantum = .01f ; nbits = 0 ;
//       quantum = .00f ; nbits = 12 ;
//       h64 = IEEE32_linear_quantize_0(block0, in*jn, nbits, quantum, block2) ; p64.u = h64 ; nbts = p64.p.nbts ;
      h64 = IEEE32_linear_quantize_1(block0, in*jn, nbits, quantum, block2) ; p64.u = h64 ; nbts = p64.q.nbts ;
      btab[nbts]++ ;
      nbtot += nbts*in*jn ;
      // step 2 : predict (Lorenzo)
      // step 3 : pseudo-encode
      // step 4 : unpredict
      // step 5 : unquantize
      bzero(block1, sizeof(block1)) ;
//       status = IEEE32_linear_unquantize_0(block2, h64, in*jn, block1) ;
      status = IEEE32_linear_unquantize_1(block2, h64, in*jn, block1) ;
//       memcpy(block1, block0, sizeof(float)*in*jn) ;
      // step 6 : analyze
      update_error_stats(block0, block1, in*jn, e0) ;
// fprintf(stderr, "max error in block = %f (%d), avg error = %f\n", e0->max_error, e0->ndata, e0->abs_error/e0->ndata) ;
      put_word_block(INDEX_F(t,i0,ni,j0), block1, in, ni, jn) ;
//       fprintf(stderr, "%4d,",i0) ;
    }
//     fprintf(stderr, "%4d]\n", i0) ;
  }
end:
  fprintf(stderr, "number of blocks = %d, bits/point = %f\n", nblk, 1.0*nbtot/ni/nj) ;
  fprintf(stderr, "btab = ") ; for(i=0 ; i<24 ; i++) fprintf(stderr, "%4d", btab[i]) ; fprintf(stderr, "\n") ;
  update_error_stats(f, t, -ni*nj, e1) ;

  e0->bias /= e0->ndata ; e0->abs_error /= e0->ndata ; e0->sum /= e0->ndata ;
  fprintf(stderr, "name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
          name[0], name[1], e0->ndata, e0->bias, e0->abs_error, e0->max_error, e0->sum) ;

  e1->bias /= e1->ndata ; e1->abs_error /= e1->ndata ; e1->sum /= e1->ndata ;
  fprintf(stderr, "name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
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
      get_word_block(f+i0+j0*ni, block0, in-i0, ni, jn-j0) ;
//    alter block0
      for(j=0 ; j<jn-j0 ; j++){
        for(i=0 ; i<in-i0 ; i++){
          block0[i+j*(in-i0)] +=  0.1f ;
        }
      }
//    put block0 back
      put_word_block(t+i0+j0*ni, block0, in-i0, ni, jn-j0) ;
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
  while((*name != '/') && (*name != '\0')) name++ ; if(*name == '/') name++ ;
  fprintf(stderr, "name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
          name[0], name[1], e0->ndata, e0->bias, e0->abs_error, e0->max_error, e0->sum) ;
  fprintf(stderr, "freed t\n") ;
}

#define NPTSI 100
#define NPTSJ 100

int main(int argc, char **argv){
  int dims[10] ;
  int dim3[] = { 1,2,3 } ;
  int buf3[6] ;
  int ndim = 0 ;
  int fd = 0 ;
  int i, j ;
  void *buf ;
  int ndata ;
  float ref[NPTSI*NPTSJ], new[NPTSI*NPTSJ] ;
  error_stats e ;
  bitstream **streams ;
  compressed_field field ;
  compress_rules rules ;

  start_of_test(argv[0]);
  for(j=0 ; j<NPTSJ ; j++){
    for(i=0 ; i<NPTSI ; i++){
      ref[i+j*NPTSJ] = 1.0f ;
      new[i+j*NPTSJ] = 1.1f ;
    }
  }
//   update_error_stats(ref, new, 0, &e) ;
//   ndata = update_error_stats(ref, new, -NPTSI*NPTSJ, &e) ;
//   e.bias /= e.ndata ; e.abs_error /= e.ndata ; e.sum /= e.ndata ;
//   fprintf(stderr, "ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", e.ndata, e.bias, e.abs_error, e.max_error, e.sum) ;
//   process_data_2d(ref, NPTSI, NPTSJ, &e, "/XX") ;
//   e.bias /= e.ndata ; e.abs_error /= e.ndata ; e.sum /= e.ndata ;
//   fprintf(stderr, "ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", e.ndata, e.bias, e.abs_error, e.max_error, e.sum) ;

  for(i=1 ; i<argc ; i++){
    fd = 0 ;
    ndim = 0 ;
    buf = read_32bit_data_record(argv[i], &fd, dims, &ndim, &ndata) ;
    fprintf(stderr, "file = '%s', fd = %d\n", argv[i], fd) ;
    while(buf != NULL){
      fprintf(stderr, "number of dimensions = %d : (", ndim) ;
      for(j=0 ; j<ndim ; j++) fprintf(stderr, "%d ", dims[j]) ; fprintf(stderr, ") [%d]\n", ndata);
      if(ndim == 2){
        process_data_2d(buf, dims[0], dims[1], &e, argv[i]) ;
//         field = compress_2d_data(buf, dims[0], dims[0], dims[1], rules) ;
      }
      free(buf) ;
// break ;   // only one record for now
      fprintf(stderr, "reading next record, fd = %d\n", fd);
      buf = read_32bit_data_record("", &fd, dims, &ndim, &ndata) ;
    }
    if(ndata == 0) {
      fprintf(stderr, "end of file = '%s', close status = %d\n", argv[i], close(fd)) ;
    }else{
      fprintf(stderr, "error reading record\n") ;
    }
  }
}
