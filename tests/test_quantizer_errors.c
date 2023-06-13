#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <rmn/test_helpers.h>
#include <rmn/misc_operators.h>
#include <rmn/c_record_io.h>
#include <rmn/ieee_quantize.h>
#include <rmn/compress_data.h>
#include <rmn/compress_data.h>

typedef struct{
  int ndata ;         // numbre of data points involved
  float bias ;        // running sum of differences between float arrays
  float abs_error ;   // running sum of absolute differences between float arrays
  float max_error ;   // largest absolute difference
  double sum ;
} error_stats ;
int update_error_stats(float *fref, float *fnew, int nd, error_stats *e);
int float_array_differences(float *fref, float *fnew, int nr, int lref, int lnew, int nj, error_stats *e);

void process_data_2d(void *buf, int ni, int nj, error_stats *e, char *name){
  float *f = (float *) buf ;
  int i0, i, j0, j, in, jn ;
  float *t = (float *) malloc(ni*nj*sizeof(float)) ;
  float *t0 = t ;
  float *f0 = f ;
  float block[64*64] ;
return ;

  fprintf(stderr, "allocated and initialized t[%d,%d]\n", ni, nj) ;
  for(i=0 ; i<ni*nj ; i++) t[i] = 0.0f ;

  update_error_stats(f, t, 0, e) ;
  for(j0 = 0 ; j0<nj ; j0+=64){
    jn = (j0+64) > nj ? nj : j0+64 ;
    for(i0=0 ; i0<ni ; i0+=64){
      in = (i0+64) > ni ? ni : i0+64 ;
// ========================================================================
//    do something to data in buf
#if 1
      for(i=0 ; i<4096 ; i++) block[i] = 999999.0f ;
      get_word_block(f+i0+j0*ni, block, in-i0, ni, jn-j0) ;
//    alter block
      for(j=0 ; j<jn-j0 ; j++){
        for(i=0 ; i<in-i0 ; i++){
          block[i+j*(in-i0)] +=  0.1f ;
        }
      }
//    put block back
      put_word_block(t+i0+j0*ni, block, in-i0, ni, jn-j0) ;
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
        update_error_stats(f+i0+j*ni, t+i0+j*ni, in-i0, e) ;
      }
    }
  }
  free(t) ;
  e->bias /= e->ndata ; e->abs_error /= e->ndata ; e->sum /= e->ndata ;
  while((*name != '/') && (*name != '\0')) name++ ; if(*name == '/') name++ ;
  fprintf(stderr, "name = %c%c, ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", 
          name[0], name[1], e->ndata, e->bias, e->abs_error, e->max_error, e->sum) ;
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
  ndata = update_error_stats(ref, new, -NPTSI*NPTSJ, &e) ;
  e.bias /= e.ndata ; e.abs_error /= e.ndata ; e.sum /= e.ndata ;
  fprintf(stderr, "ndata = %d, bias = %f, abs_error = %f, max_error = %f, avg = %f\n", e.ndata, e.bias, e.abs_error, e.max_error, e.sum) ;
  process_data_2d(ref, NPTSI, NPTSJ, &e, "/XX") ;
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
        field = compress_2d_data(buf, dims[0], dims[0], dims[1], rules) ;
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
