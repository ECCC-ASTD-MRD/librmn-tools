#include <stdio.h>
#include <stdlib.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

static FILE *tee_file = NULL ;

#if defined(OVERRIDE_WITH_LOCAL)
// override the weak entry point print_diag
void print_diag(FILE *f, char *what, int level){
  if(level < 0) return ;
  fprintf(f, "MY PRINT DIAG %s", what) ;
  if(tee_file != NULL){
    fprintf(tee_file, "MY PRINT DIAG %s", what) ;
  }
}
#endif

int main(int argc, char **argv){
  start_of_test(argv[0]);
  TEE_FPRINTF(stderr, -1, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
  TEE_FPRINTF(stderr, 1, "MUST SEE : %d %d %d\n", 10, 20, 30) ;
  FILE *f1 = open_tee_file("test_tee_file.log") ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_FPRINTF(stderr, -1, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
  TEE_FPRINTF(stderr, 1, "MUST SEE : %d %d %d\n", 1, 2, 3) ;
  FILE *f2 = open_tee_file(NULL) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_FPRINTF(stderr, -1, "MUST NOT SEE : %d %d %d\n", 4, 5, 6) ;
  TEE_FPRINTF(stderr, 1, "MUST SEE : %d %d %d\n", 4, 5, 6) ;
}
