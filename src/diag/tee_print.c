/*
 * Hopefully useful code for C
 * Copyright (C) 2023  Recherche en Prevision Numerique
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * print messages from application, with option to duplicate them to a log file
 * 
 */
// export MP_CHILD=\${MP_CHILD:-\${PMI_RANK:-\${OMPI_COMM_WORLD_RANK:-\${ALPS_APP_PE}}}}
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <rmn/tee_print.h>

static int32_t tee_auto_init = 1 ;
static FILE *tee_file = NULL ;      // no tee log file by default
static int32_t msg_level = 0 ;      // everything by default
static int32_t tee_msg_level = 0 ;  // everything by default
static int32_t auto_open = 0 ;      // OFF by default

// set auto open mode (non zero = true)
// return previous mode
int32_t set_tee_auto_open(int32_t mode){
  int32_t temp = auto_open ;
  auto_open = mode ;     // set new mode
  return temp ;          // return old mode
}

// set tee log file pointer to f
// return old file pointer
FILE *set_tee_file(FILE *f){
  FILE *old = tee_file ;
  tee_file = f ;
  return old ;
}

// get current tee log file pointer
FILE *get_tee_file(void){
  return tee_file ;
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
    char name[4096] ;
    char *d = getenv("TEE_FILE_DIR") ;
    if(d == NULL) d = "." ;
    char *n = getenv("TEE_FILE_NAME") ;
    if(n == NULL) {
      snprintf(name, sizeof(name),"%s/tee_out_%d.txt", d, getpid());
    }else{
      snprintf(name, sizeof(name),"%s/%s", d, n);
    }
fprintf(stderr, "opening automatic file '%s', prefix = '%s'\n", name, d) ;
    f = fopen(name, "a+") ;
    if(f == NULL) return tee_file ;  // open failed, NO-OP
  }
  return set_tee_file(f) ;
}

// diagnostic print, goes both to file f and tee_file (if not NULL)
// if tee_file is NULL and auto_open is true, a tee log file is automatically opened
#pragma weak print_diag=Print_diag
// void print_diag(FILE *f, char *what, int level) ;
void Print_diag(FILE *f, char *what, int level){
  if(level < msg_level) return ;       // message level lower than threshold
  fprintf(f, "%s", what) ;

  if(tee_auto_init == 1){
    if( getenv("TEE_AUTO_OPEN") != NULL ) {
      auto_open = 1 ;
fprintf(stderr, "DEBUG: auto_open = 1 \n") ;
    }
    tee_auto_init = 0 ;
  }

  if(level < tee_msg_level) return ;   // message level lower than tee threshold

  if(tee_file == NULL && auto_open){
    open_tee_file(NULL) ;
  }
  if(tee_file != NULL){
    fprintf(tee_file, "%s", what) ;
  }
}

