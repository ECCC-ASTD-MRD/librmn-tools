#include <rmn/timers.h>
#include <rmn/test_helpers.h>

#define NT 10000
int main(int argc, char **argv){
  uint64_t t0, t1, tu, tf ;
  double t ;
  int i ;
  TIME_LOOP_DATA ;

  start_of_test("fine grained timers") ;
//   fprintf(stderr, "test of fine grained timers\n") ;
  tf = cycles_counter_freq() ;    // counter frequency in Hz
  t = 1.0 ;
  t  /= tf ;                      // one time counter cycle in seconds
  t0 = elapsed_cycles() ;         // fine grain timer   (nanoseconds)
  tu = elapsed_us() ;             // medium grain timer (microsecond)
  tu = elapsed_us() -tu ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "call to elapsed_us = %ld cycles, %8.2g seconds\n", (t1-t0)/2, (t1-t0)/2*t) ;

  TIME_LOOP_EZ(NT, 1, tu = elapsed_us()) ;
  fprintf(stderr, "elapsed_us     : %s\n",timer_msg) ;

  TIME_LOOP_EZ(NT, 1, t0 = elapsed_cycles()) ;
  fprintf(stderr, "elapsed_cycles : %s\n",timer_msg) ;

  t0 = elapsed_cycles() ;
  for(i=0 ; i<NT ; i++) tu = elapsed_cycles() ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "avg call to %d elapsed_cycles = %ld cycles, %8.2f ns\n", NT, (t1-t0)/NT, (t1-t0)/NT*t*1.0E+9) ;

  t0 = elapsed_cycles() ;
  for(i=0 ; i<NT ; i++) tu = elapsed_us() ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "avg call to %d elapsed_us     = %ld cycles, %8.2f ns\n", NT, (t1-t0)/NT, (t1-t0)/NT*t*1.0E+9) ;

  if(timer_max == 0) fprintf(stderr, "timer_max ==0, should not happen\n");
}
