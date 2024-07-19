//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// append a set of potentially sparse files to another potentially sparse file
// copy a sparse file
//
#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <rmn/sparse_concat.h>

static int verbose = 0 ;

// copy nbytes from fdi into fdo
// fdi    : input file descriptor
// fdo    : output file descriptor
// nbytes : number of bytes to copy
// returns 0 if copy successful
size_t CopyFileData(int fdi, int fdo, size_t nbytes){
  char buf[1024*1024*8] ;  // 8vi MB buffer for copying data
  ssize_t nr, nw ;
  while(nbytes > 0){
    ssize_t nr, nw ;
    size_t nb = (nbytes < sizeof(buf)) ? nbytes : sizeof(buf) ;
    nr = read(fdi, buf, nb) ;
    nw = write(fdo, buf, nr) ;
    nbytes -= nw ;
    if(nr != nw) exit(1) ;
  }
  return nbytes ;
}

// copy fdi into fdo
// fdi    : input file descriptor
// fdo    : output file descriptor
// diag   : if non zero, fdo is ignored and fdi hole/data map is printed
// returns final size of output file
size_t SparseConcatFd(int fdi, int fdo, int diag){
  off_t hole, cur_i, szi, szo, cur_o ;

  szi = lseek(fdi, 0L, SEEK_END) ;                // input file size
  szo = 0 ;
  if(diag == 0) szo = lseek(fdo, 0L, SEEK_END) ;  // output file size (only if diag == 0)

  hole = cur_i = lseek(fdi, 0L, SEEK_SET) ;
  while(hole < szi){
    hole = lseek(fdi, cur_i, SEEK_HOLE) ;         // look for a hole
    if(diag && (hole - cur_i) > 0) fprintf(stderr, "data at %12ld, %12ld bytes\n", cur_i, hole - cur_i) ;
    lseek(fdi, cur_i, SEEK_SET) ;
    if(diag == 0) CopyFileData(fdi, fdo, hole - cur_i) ;           // copy from current position up to hole
    if(hole < szi){
      cur_i = lseek(fdi, hole, SEEK_DATA) ;
      if(diag) {
        fprintf(stderr, "hole at %12ld, %12ld bytes \n", hole, cur_i - hole) ;
      }else{
        cur_o = lseek(fdo, cur_i - hole, SEEK_CUR) ;   // make a hole in output file
      }
    }
  }
  if(diag == 0) szo = lseek(fdo, 0L, SEEK_END) ;                   // return final size of output
  close(fdi) ;
  close(fdo) ;
  return szo ;
}

// copy file name2 at end of name1
// if diag is true, name2 is ignored and name1 hole/data map is printed
// name1 : output file name
// name2 : input file name
// diag   : if non zero, name2 is ignored and a map of name1 is produced
// returns final size of output file
size_t SparseConcatFile(char *name1, char *name2, int diag){
  int fdi, fdo ;

  if(diag == 0 && verbose) fprintf(stderr, "appending %s to %s\n", name2, name1);
  fdi = 0 ;
  if(diag == 0) fdi = open(name2, O_RDONLY | O_LARGEFILE) ;  // input file
  if(fdi < 0) exit(1) ;

  if(diag){
    fdo = open(name1, O_RDONLY | O_LARGEFILE) ;  // open as input file
    fdi = fdo ;
  }else{
    fdo = open(name1, O_RDWR | O_CREAT | O_LARGEFILE, 0777) ;
  }
  if(fdo < 0) exit(1) ;

  return SparseConcatFd(fdi, fdo, diag) ;
}

static void usage(char *argv0){
  fprintf(stderr, "usage :\n") ;
  fprintf(stderr, "%s file_0 \n",argv0) ;
  fprintf(stderr, "   print a hole/data map of sparse file\n") ;
  fprintf(stderr, "%s [-v|--verbose] file_0 file_1 [file_2] ... [file_n] \n",argv0) ;
  fprintf(stderr, "   append sparse files file_1 ... file_n to file_0 as a sparse file\n") ;
  fprintf(stderr, "   -v|--verbose : verbose output\n") ;
  exit(1) ;
}

// if only one argument, print data/hole map
// if more than one argument append files(s) after first to first file
int SparseConcat_main(int argc, char **argv){
  char *name_in = "/dev/null" ;
  size_t szo ;
  int i ;
  char *argv0 = argv[0] ;

  while(argc > 2 && *argv[1] == '-'){
    if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--verbose") == 0) verbose = 1 ;
    argv++ ;
    argc-- ;
  }

  if(argc < 2) usage(argv0) ;
  if(argc > 2) {
    name_in  = argv[2] ;
  }
  szo = SparseConcatFile( argv[1], name_in, argc == 2) ;            // argc ==2 : dump file map
  for(i = 3 ; i < argc ; i++){                                    // more than 2 arguments ?
    szo = SparseConcatFile(argv[1], argv[i], 0) ;
  }
  if(argc > 2 && verbose) fprintf(stderr, "final size of %s = %ld\n", argv[1], szo) ;

  return 0 ;
}
