#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include <rmn/timers.h>
#include <rmn/test_helpers.h>

#if !defined(NPTS)
#define NPTS 128
#endif

#define NTIMES 100000

#include <rmn/lorenzo.h>

int main(int argc, char **argv){
  int32_t data[NPTS+1][NPTS] ;
  int32_t data2[NPTS+1][NPTS] ;
  int32_t pred[NPTS+1][NPTS] ;
//   int32_t pred2[NPTS+1][NPTS] ;
  int i, j, errors ;
  uint64_t tmin, tmax, freq ;
  double nano, tavg ;
//   int niter = NTIMES ;
  char buf[1024] ;
  size_t bufsiz = sizeof(buf) ;

  start_of_test("lorenzo (C)");
  freq = cycles_counter_freq() ;
  nano = 1000000000.0 ;
  fprintf(stderr, "nano = %8.2G\n", nano) ;
  nano = nano / freq ;
  fprintf(stderr, "nano = %8.2G\n", nano) ;

  for(j=0 ; j<NPTS+1 ; j++){
    for(i=0 ; i<NPTS ; i++){
      data[j][i]  = (2*i + 3*j + 5) ;
      data2[j][i] = 999999 ;
      pred[j][i]  = 999999 ;
//       pred2[j][i] = 999999 ;
    }
  }

  LorenzoPredict(&data[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%d ",pred[NPTS][i]); fprintf(stderr, "\n");
  LorenzoUnpredict(&data2[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%d ",data2[NPTS][i]); fprintf(stderr, "\n");
  errors = 0 ;
  for(j=0 ; j<NPTS ; j++){
    for(i=0 ; i<NPTS ; i++){
      if(data2[j][i] != (2*i + 3*j + 5) ) errors++;
    }
  }
  fprintf(stderr, "LorenzoUnpredict    : errors = %d\n\n",errors);

  LorenzoPredict(&data[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%d ",pred[NPTS][i]); fprintf(stderr, "\n");
  LorenzoUnpredict(&data2[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%d ",data2[NPTS][i]); fprintf(stderr, "\n");
  errors = 0 ;
  for(j=0 ; j<NPTS ; j++){
    for(i=0 ; i<NPTS ; i++){
      if(data2[j][i] != (2*i + 3*j + 5) ) errors++;
    }
  }
  fprintf(stderr, "LorenzoUnpredict_   : errors = %d\n\n",errors);

//   LorenzoPredictShort(&data[0][0], &pred2[0][0], NPTS, NPTS, NPTS, NPTS) ;
//   for(i=0 ; i<8 ; i++) fprintf(stderr, "%d ",pred2[NPTS][i]); fprintf(stderr, "\n");
//   for(j=0 ; j<NPTS+1 ; j++) for(i=0 ; i<NPTS ; i++) data2[j][i] = 999999 ;
//   LorenzoUnpredict(&data2[0][0], &pred2[0][0], NPTS, NPTS, NPTS, NPTS) ;
//   for(i=0 ; i<8 ; i++) fprintf(stderr, "%d ",data2[NPTS][i]); fprintf(stderr, "\n");
//   errors = 0 ;
//   for(j=0 ; j<NPTS ; j++){
//     for(i=0 ; i<NPTS ; i++){
//       if(data2[j][i] != (2*i + 3*j + 5) ) errors++;
//     }
//   }
//   fprintf(stderr, "LorenzoPredict_S  : errors = %d\n\n",errors);

  TIME_LOOP(tmin, tmax, tavg, NTIMES, (NPTS*NPTS), buf, bufsiz, LorenzoPredict(&data[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) )
  fprintf(stderr, "LorenzoPredict    : %s\n",buf);

//   TIME_LOOP(tmin, tmax, tavg, NTIMES, (NPTS*NPTS), buf, bufsiz, LorenzoPredictShort(&data[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) )
//   fprintf(stderr, "LorenzoPredict_S  : %s\n",buf);

  TIME_LOOP(tmin, tmax, tavg, NTIMES, (NPTS*NPTS), buf, bufsiz, LorenzoUnpredict(&data[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) )
  fprintf(stderr, "LorenzoUnpredict  : %s\n",buf);

  TIME_LOOP(tmin, tmax, tavg, NTIMES, (NPTS*NPTS), buf, bufsiz, LorenzoUnpredict(&data[0][0], &pred[0][0], NPTS, NPTS, NPTS, NPTS) )
  fprintf(stderr, "LorenzoUnpredict_ : %s\n",buf);

  if(tmax == 0) fprintf(stderr, "tmax == 0, should not happen\n") ;
}
