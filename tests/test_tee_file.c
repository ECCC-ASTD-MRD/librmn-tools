#include <stdio.h>
#include <stdlib.h>

// #define WITHOUT_APP
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

static FILE *tee_file = NULL ;

#if defined(OVERRIDE_WITH_LOCAL)
// override the weak entry point User_Tee_Log
void User_Tee_Log(FILE *f, char *what, TApp_LogLevel level){

  if(level < TEE_VERBATIM) return ;
  if(level > get_msg_level() && get_msg_level() != TEE_ALWAYS) return ;
  if(get_use_app()){
    char _TeMp_[4096] ;
    snprintf(_TeMp_, sizeof(_TeMp_), "MY USER TEE LOG [%7s] %s", msg_level_name(level), what) ;
    LIB_LOG(TEE_LIBRMN, level, "%s", _TeMp_);
  }else{
    fprintf(f, "MY USER TEE LOG [%7s] %s", msg_level_name(level), what) ;
  }
  if(tee_file != NULL){
    fprintf(tee_file, "MY USER TEE LOG %s", what) ;
  }
}
#endif

void dummy(char *msg){
  TEE_PRINTF(TEE_ALWAYS, "TEE_PRINTF TEE_ALWAYS : in dummy\n");
  TEE_PRINTF(TEE_VERBATIM, "TEE_PRINTF TEE_VERBATIM : in dummy\n");
  LIB_LOG(TEE_LIBRMN, TEE_INFO, "LIB_LOG TEE_LIBRMN, TEE_INFO :%s\n", msg);
  LIB_LOG(TEE_LIBRMN, TEE_WARNING, "LIB_LOG TEE_WARNING, TEE_INFO :%s\n", msg);
  LIB_LOG(TEE_LIBRMN, TEE_ERROR, "LIB_LOG TEE_LIBRMN, TEE_ERROR :%s\n", msg);
  LIB_LOG(TEE_LIBRMN, TEE_SYSTEM, "LIB_LOG TEE_LIBRMN, TEE_SYSTEM :%s\n", msg);
  LIB_LOG(TEE_LIBRMN, TEE_FATAL, "LIB_LOG TEE_LIBRMN, TEE_FATAL :%s\n", msg);
  TEE_PKG_LOG(TEE_VERBATIM, "TEE_TEST", 076, "\002package_log message 02") ;
  TEE_PKG_LOG(TEE_VERBATIM, "TEE_TEST", 076, "package_log message 00") ;
  TEE_PKG_LOG(TEE_DEBUG, "TEE_TEST", 076, "package_log MUST SEE") ;
  TEE_PKG_LOG(TEE_STAT, "TEE_TEST", 076, "package_log MUST SEE") ;
  TEE_PKG_LOG(TEE_FATAL, "TEE_TEST", 076, "\037package_log MUST SEE") ;
  TEE_PKG_LOG(TEE_EXTRA, "TEE_TEST", 076, "package_log MUST NOT SEE") ;
}
static char *TEE_FILE_DIR  = "TEE_FILE_DIR=./LOGFILES" ;
static char *TEE_FILE_NAME = "TEE_FILE_NAME=tee_test_002.log" ;

int main(int argc, char **argv){
  putenv(TEE_FILE_DIR) ;
  putenv(TEE_FILE_NAME) ;
  FILE *f1 = NULL ;
  FILE *f2 = NULL ;

  start_of_test(argv[0]);
  if(argc > 1){
    set_use_app(1) ;
  }
  if(argc > 10) goto end ;
  system("rm LOGFILES/*.log ; ls -al LOGFILES") ;
  // set logging levels to DEBUG
  set_msg_level(TEE_DEBUG) ;
  Lib_LogLevelNo(TEE_LIBRMN, TEE_DEBUG) ;
  set_tee_msg_level(TEE_DEBUG) ;
  set_msg_file(stderr) ;

  dummy("dummy's message") ;
  fprintf(get_msg_file(), "-------\n");

  tee_file = get_tee_file() ;
  TEE_DIAG(TEE_ALWAYS,"MUST SEE : TEE_DIAG TEE_ALWAYS message 001\n") ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
  TEE_PRINTF(TEE_INFO, "MUST SEE : TEE_PRINTF TEE_INFO %d %d %d\n", 10, 20, 30) ;
  TEE_DIAG(TEE_INFO, "MUST SEE : TEE_DIAG TEE_INFO %d %d %d\n", 10, 20, 30) ;
  fprintf(get_msg_file(), "-------\n");

  f1 = open_tee_file("LOGFILES/tee_test_001.log") ;
  if(f1 == NULL) exit(1) ;
  tee_file = get_tee_file() ;
  if(tee_file == stdout) fprintf(stderr,"tee file is stdout\n") ;
  TEE_DIAG(TEE_ALWAYS,"MUST SEE : TEE_ALWAYS message 002\n") ;
  TEE_PRINTF(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
  TEE_PRINTF(TEE_WARNING, "MUST SEE : %d %d %d\n", 1, 2, 3) ;
  fprintf(get_msg_file(), "-------\n");

  f2 = open_tee_file(NULL) ;  // automatic open
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
    TEE_DIAG(TEE_ALWAYS, "TEE_ALWAYS, MUST SEE\n") ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 10, 20, 30) ;
    TEE_DIAG(TEE_INFO, "TEE_INFO, MUST SEE : %d %d %d\n", 10, 20, 30) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 1, 2, 3) ;
    TEE_DIAG(TEE_WARNING, "TEE_WARNING, MUST SEE : %d %d %d\n", 1, 2, 3) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", 4, 5, 6) ;
    TEE_DIAG(TEE_ERROR, "TEE_ERROR, MUST SEE : %d %d %d\n", 4, 5, 6) ;
    TEE_DIAG(APP_EXTRA, "MUST NOT SEE : %d %d %d\n", -4, -5, -6) ;
//     TEE_DIAG(TEE_SYSTEM, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
    TEE_DIAG(APP_FATAL, "APP_FATAL, MUST SEE : %d %d %d\n", -4, -5, -6) ;
    TEE_DIAG(TEE_EXTRA, "MUST NOT SEE : %d %d %d\n", -7, -8, -9) ;
    set_msg_level(TEE_EXTRA) ;
    TEE_DIAG(TEE_EXTRA, "TEE_EXTRA, MUST SEE : %d %d %d\n", -7, -8, -9) ;
    TEE_DIAG((TApp_LogLevel)-2, "level == -2, MUST NOT SEE\n") ;
    fprintf(stdout,"====================\n");
  }
end :
  if(f1) fclose(f1) ;
  if(f2) fclose(f2) ;
  system("ls -l LOGFILES") ;
}
