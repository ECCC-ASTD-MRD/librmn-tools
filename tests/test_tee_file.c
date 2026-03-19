#include <stdio.h>
#include <stdlib.h>

#define WITHOUT_APP
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

static FILE *tee_file = NULL ;

#if defined(OVERRIDE_WITH_LOCAL)
// override the weak entry point User_Tee_Log
void User_Tee_Log(FILE *f, char *what, TApp_LogLevel level){
  if(level > get_msg_level()) return ;
  if(get_use_app()){
    char _TeMp_[4096] ;
    snprintf(_TeMp_, sizeof(_TeMp_), "MY USER TEE LOG [%s] %s", msg_level_name(level), what) ;
    Lib_Log(TEE_LIBRMN, level, "%s", _TeMp_);
  }else{
    fprintf(f, "MY USER TEE LOG [%s] %s", msg_level_name(level), what) ;
  }
  if(tee_file != NULL){
    fprintf(tee_file, "MY USER TEE LOG %s", what) ;
  }
}
#endif

void dummy(char *msg){
  TEE_PRINTF(TEE_DEBUG, "in dummy\n");
  Lib_Log(TEE_LIBRMN, TEE_INFO, "%s\n", msg);
}
static char *TEE_FILE_DIR  = "TEE_FILE_DIR=./LOGFILES" ;
static char *TEE_FILE_NAME = "TEE_FILE_NAME=tee_test_002.log" ;

int main(int argc, char **argv){
  putenv(TEE_FILE_DIR) ;
  putenv(TEE_FILE_NAME) ;

  start_of_test(argv[0]);
  if(argc > 1){
    set_use_app(1) ;
  }
  system("rm LOGFILES/*.log ; ls -al LOGFILES") ;
  // set logging levels to DEBUG
  set_msg_level(TEE_DEBUG) ;
  Lib_LogLevelNo(TEE_LIBRMN, TEE_DEBUG) ;
  set_tee_msg_level(TEE_DEBUG) ;
//   set_msg_file(stderr) ;
  dummy("dummy's message") ;
  fprintf(get_msg_file(), "-------\n");

  tee_file = get_tee_file() ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
  TEE_PRINTF(TEE_INFO, "MUST SEE : %d %d %d\n", 10, 20, 30) ;

  FILE *f1 = open_tee_file("LOGFILES/tee_test_001.log") ;
  if(f1 == NULL) exit(1) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
  TEE_PRINTF(TEE_WARNING, "MUST SEE : %d %d %d\n", 1, 2, 3) ;

  FILE *f2 = open_tee_file(NULL) ;  // automatic open
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
  fclose(f1) ;
  fclose(f2) ;
  system("ls -l LOGFILES") ;
}
