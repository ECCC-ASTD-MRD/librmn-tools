#include <stdio.h>
#include <stdlib.h>

// #define WITHOUT_APP
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

static FILE *tee_file = NULL ;

#if defined(OVERRIDE_WITH_LOCAL)
// override the weak entry point print_diag
void print_diag(FILE *f, char *what, int level){
  if(level > get_msg_level()) return ;
  if(get_use_app()){
    char _TeMp_[4096] ;
    snprintf(_TeMp_, sizeof(_TeMp_), "MY PRINT DIAG [%s] %s", msg_level_name(level), what) ;
    Lib_Log(APP_LIBRMN, (TApp_LogLevel)level, "%s", _TeMp_);
  }else{
    fprintf(f, "MY PRINT DIAG [%s] %s", msg_level_name(level), what) ;
  }
  if(tee_file != NULL){
    fprintf(tee_file, "MY PRINT DIAG %s", what) ;
  }
}
#endif

void dummy(char *msg){
  TEE_PRINTF(TEE_DEBUG, "in dummy\n");
  Lib_Log(APP_LIBRMN, TEE_INFO, "%s\n", msg);
}

int main(int argc, char **argv){
  start_of_test(argv[0]);
  if(argc > 1){
    set_use_app(1) ;
  }
  set_msg_level(TEE_DEBUG) ;
  Lib_LogLevelNo(APP_LIBRMN, TEE_DEBUG) ;
  set_tee_msg_level(TEE_DEBUG) ;
//   set_msg_file(stderr) ;
  dummy("dummy's message") ;
  fprintf(get_msg_file(), "-------\n");

  tee_file = get_tee_file() ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
  TEE_PRINTF(TEE_INFO, "MUST SEE : %d %d %d\n", 10, 20, 30) ;

  FILE *f1 = open_tee_file("test_tee_file.log") ;
  if(f1 == NULL) exit(1) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
  TEE_PRINTF(TEE_WARNING, "MUST SEE : %d %d %d\n", 1, 2, 3) ;

  FILE *f2 = open_tee_file(NULL) ;
  if(f2 == NULL) exit(1) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 4, 5, 6) ;
  TEE_PRINTF(TEE_ERROR, "MUST SEE : %d %d %d\n", 4, 5, 6) ;

  fprintf(stdout,"====================\n");
  // TEE_DIAG uses weak overridable print_diag function
  // TEE_PRINTF used App if WITHOUT_APP is not defined
  for(int i = 0 ; i<3 ; i++){
    set_use_app(i%2) ;
    set_msg_level(TEE_DEBUG) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
    TEE_DIAG(TEE_INFO, "MUST SEE : %d %d %d\n", 10, 20, 30) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
    TEE_DIAG(TEE_WARNING, "MUST SEE : %d %d %d\n", 1, 2, 3) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 4, 5, 6) ;
    TEE_DIAG(TEE_ERROR, "MUST SEE : %d %d %d\n", 4, 5, 6) ;
    TEE_DIAG(-1, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", -7, -8, -9) ;
    set_msg_level(TEE_EXTRA) ;
    TEE_DIAG(TEE_EXTRA, "MUST SEE : %d %d %d\n", -7, -8, -9) ;
    fprintf(stdout,"====================\n");
  }
}
