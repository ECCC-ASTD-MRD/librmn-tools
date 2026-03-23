#include <stdio.h>
#include <errno.h>
#include <App.h>

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  Lib_Log(APP_LIBRMN, APP_ALWAYS, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
  Lib_Log(APP_LIBRMN, APP_FATAL, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
  Lib_Log(APP_LIBRMN, APP_WARNING, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
  Lib_Log(APP_LIBRMN, APP_ERROR, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
  if(errno == 0) errno = -1 ;
  Lib_Log(APP_LIBRMN, APP_SYSTEM, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
  Lib_Log(APP_LIBRMN, APP_ALWAYS, "MUST SEE : %d %d %d\n", -4, -5, -6) ;
}
