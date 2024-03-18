#include <stdio.h>
#include <stdlib.h>


void end_of_test(void){
  fprintf(stderr,"==================== end of test  ==================== ") ;
  system("date '+%Y-%m-%d_%H.%M.%S' 1>&2") ;
}

void start_of_test(char *name){
  fprintf(stderr,"==================== test : %s ==================== ", name) ;
  system("date '+%Y-%m-%d_%H.%M.%S' 1>&2") ;
  atexit(end_of_test) ;
}
// common Fortran mangling
void start_of_test_(char *name){ start_of_test(name) ; };
void start_of_test__(char *name){ start_of_test(name) ; };
