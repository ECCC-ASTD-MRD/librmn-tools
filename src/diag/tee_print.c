//
// Hopefully useful code for C
// Copyright (C) 2023-2026  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// print messages from application, with option to duplicate them to a log file
//
// export MP_CHILD=\${MP_CHILD:-\${PMI_RANK:-\${OMPI_COMM_WORLD_RANK:-\${ALPS_APP_PE}}}}
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define WITHOUT_APP
#include <rmn/tee_print.h>

static int32_t tee_auto_init = 1 ;
static FILE *msg_file = NULL ;                     // no default message file
static FILE *tee_file = NULL ;                     // no tee log file by default
static TApp_LogLevel msg_level     = TEE_WARNING ; // everything by default
static TApp_LogLevel tee_msg_level = TEE_WARNING ; // everything by default
static int32_t auto_open = 0 ;                     // OFF by default
static int32_t use_app = 0 ;                       // use App for messages

static char *names[] = { "ALWAYS", "FATAL", "SYSTEM", "ERROR", "WARNING", "INFO", "STAT", "TRIVIAL", "DEBUG", "EXTRA", "QUIET", "INVALID" } ;

char *msg_level_name(TApp_LogLevel level){
  if(level < 0 || level > 10) return names[11] ;
  return names[level] ;
}

// get use App flag
int32_t get_use_app(void) { return use_app ; }

// set use App flag
int32_t set_use_app(int32_t yes){
  int32_t old = use_app ;
  use_app = yes ;
  return old ;
}

// get message threshold levels
TApp_LogLevel get_msg_level(void){ return msg_level ; }

// set message threshold levels
// return old message threshold level
TApp_LogLevel set_msg_level(TApp_LogLevel level){
  TApp_LogLevel old = msg_level ;
  msg_level = level ;
  if(use_app){           // in App mode, set App log levels
    Lib_LogLevelNo(APP_LIBRMN, level) ;
    App_LogLevelNo(level) ;
  }  return old ;
}

// get tee log threshold level
TApp_LogLevel get_tee_msg_level(void){ return tee_msg_level ; }

// set tee log threshold level
// return old log threshold level
TApp_LogLevel set_tee_msg_level(TApp_LogLevel level){
  TApp_LogLevel old = tee_msg_level ;
  tee_msg_level = level ;
  return old ;
}

// set auto open mode (non zero = true)
// return previous mode
int32_t set_tee_auto_open(int32_t mode){
  int32_t temp = auto_open ;
  auto_open = mode ;     // set new mode
  return temp ;          // return old mode
}

// set tee log file pointer to f
// return old tee log file pointer
FILE *set_tee_file(FILE *f){
  FILE *old = tee_file ;
  tee_file = f ;
  return old ;
}

// get current tee log file pointer
FILE *get_tee_file(void){
  return tee_file ;
}

// set message file pointer to f
// return old message file pointer
FILE *set_msg_file(FILE *f){
  FILE *old = msg_file ;
  msg_file = f ;
  return old ;
}

// get current message file pointer
FILE *get_msg_file(void){
  return msg_file ? msg_file : stdout ;
}

// tee log file  to file named fname
// if fname is NULL, generate a filename using environment variables TEE_FILE_DIR and TEE_FILE_NAME
// return old file pointer
FILE *open_tee_file(char *fname){
  FILE *f ;
  if(fname != NULL){                 // file name supplied
fprintf(stderr, "opening explicitly named file '%s'\n", fname) ;
    f = fopen(fname, "a+") ;
    if(f == NULL) return tee_file ;  // open failed, NO-OP
  }else{                             // no file name supplied, generate one
    // ./tee_out_{pid}  if neither TEE_FILE_DIR nor TEE_FILE_NAME are present
    char name[4096] ;
    char *d = getenv("TEE_FILE_DIR") ;
    if(d == NULL) d = "." ;
    char *n = getenv("TEE_FILE_NAME") ;
    if(n == NULL) {
      n = "NoNe" ;
      snprintf(name, sizeof(name),"%s/tee_out_%d.log", d, getpid());    // ${TEE_FILE_DIR}/tee_out_{pid}.log or ./tee_out_{pid}.log
    }else{
      snprintf(name, sizeof(name),"%s/%s", d, n);                       // ${TEE_FILE_DIR}/${TEE_FILE_NAME} or ./${TEE_FILE_NAME}
    }
fprintf(stderr, "opening automatic file '%s', dir = '%s', file name = '%s'\n", name, d, n) ;
    f = fopen(name, "a+") ;
    if(f == NULL) return tee_file ;  // open failed, NO-OP
  }
  set_tee_file(f) ;
  return f ;
}

// diagnostic print, goes both to file f and tee_file (if not NULL)
// if tee_file is NULL and auto_open is true, a tee log file is automatically opened
// f     [IN] : message file
// what  [IN] : message
// level [IN] : message level, TEE_EXTRA -> TEE_FATAL
#pragma weak User_Tee_Log=_Default_User_Tee_Log_
void _Default_User_Tee_Log_(FILE *f, char *what, TApp_LogLevel level){

  if(level > msg_level && msg_level != TEE_ALWAYS) return ;       // message level higher than threshold
  if(use_app){
    Lib_Log(APP_LIBRMN, level, "%s", what);
  }else{
    fprintf(f, "%s", what) ;
  }

  if(tee_auto_init == 1){
    if( getenv("TEE_AUTO_OPEN") != NULL ) {
      auto_open = 1 ;
    }
    tee_auto_init = 0 ;
  }

  if(level > tee_msg_level && tee_msg_level != TEE_ALWAYS) return ;   // message level higher than tee threshold

  if(tee_file == NULL && auto_open){                                  // no tee file, auto open mode ON
    open_tee_file(NULL) ;
  }
  if(tee_file != NULL){                                               // there is a tee file
    fprintf(tee_file, "%s", what) ;
  }
}

