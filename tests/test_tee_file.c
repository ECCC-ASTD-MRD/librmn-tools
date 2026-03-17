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

void dummy(char *msg){
  TEE_FPRINTF(stderr, TEE_DEBUG, "in dummy");
  Lib_Log(APP_LIBRMN, TEE_INFO, "%s\n", msg);
}

int main(int argc, char **argv){
//   (void)(argc) ;
  start_of_test(argv[0]);
  set_msg_level(TEE_EXTRA) ;
  set_tee_msg_level(TEE_EXTRA) ;
  if(argc > 1){
    set_use_app(1) ;
    App_LogLevelNo(APP_DEBUG) ;
  }
  dummy("dummy's message") ;

  tee_file = get_tee_file() ;
  TEE_FPRINTF(stderr, TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
  TEE_FPRINTF(stderr, TEE_INFO, "MUST SEE : %d %d %d\n", 10, 20, 30) ;
  FILE *f1 = open_tee_file("test_tee_file.log") ;
  if(f1 == NULL) exit(1) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_FPRINTF(stderr, TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
  TEE_FPRINTF(stderr, TEE_WARNING, "MUST SEE : %d %d %d\n", 1, 2, 3) ;
  FILE *f2 = open_tee_file(NULL) ;
  if(f2 == NULL) exit(1) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_FPRINTF(stderr, TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 4, 5, 6) ;
  TEE_FPRINTF(stderr, TEE_ERROR, "MUST SEE : %d %d %d\n", 4, 5, 6) ;
}
