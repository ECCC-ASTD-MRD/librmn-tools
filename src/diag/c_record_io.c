/* Hopefully useful functions
 * Copyright (C) 2023  Recherche en Prevision Numerique
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// read a 32 bit (integer or float) data record : 
// layout :  number_of_dimensions dimensions[number_of_dimensions] data[] number_of_data
// data array is in Fortran order (first index varying first)
// filename    [IN] : path/to/file (only valid if fdi == 0)
// fdi      [INOUT] : ABS(fdi) = file descriptor. if fdi < 0, close file after reading record
// dims[]   [INOUT] : array dimensions (max 10 dimensions) (OUT if ndim == 0, IN for check if ndim > 0)
// ndim     [INOUT] : number of dimensions (0 means unknown and get dimensions and number of dimensions)
// ndata      [OUT] : number of data items read, -1 if error, 0 if End Of File
// return : pointer to data array from file
void *read_32bit_data_record(char *filename, int *fdi, int *dims, int *ndim, int *ndata){
  size_t nc = 4, nd ;
  ssize_t nr ;
  void *buf = NULL ;
  int ntot ;
  int i, ni ;
  int fd = *fdi ;

  *ndata = -1 ;                                                // precondition for error
  fd = (fd < 0) ? -fd : fd ;                                   // ABS(fd), negative fdi means close after reading record
  if(fd == 0 && filename[0] == '\0') return NULL ;             // filename should be valid if fd == 0
  if(fd != 0 && filename[0] != '\0') return NULL ;             // invalid fd / filename combination
  fd = (filename[0] != '\0') ? open(filename, O_RDONLY) : fd ; // open file if filename supplied
  if(fd < 0) return NULL ;                                     // open failed
  if(*fdi == 0) *fdi = fd ;                                    // new fd stored in fdi

  if( (nr = read(fd, &ntot, nc)) == 0){                        // read number of dimensions, check for EOF
    fprintf(stderr,"INFO : EOF\n") ;
    *ndata = 0 ;                                               // EOF, no data read, not an error
    return NULL ;
  }
  if(nr != nc) {
    fprintf(stderr,"ERROR, short read\n");
    return NULL ;
  }
  if(ntot > 10){
    fprintf(stderr,"ERROR, more than 10 dimensions (%d)\n", ntot) ;
    return NULL ;
  }
  if(*ndim == 0) {
    *ndim = ntot ;                                      // return number of dimensions to caller
    for(i=0 ; i<ntot ; i++) dims[i] = 0 ;               // set dimensions to 0 (they will be returned to the caller)
  }else{
    if(ntot != *ndim) {
      fprintf(stderr,"ERROR, read ndim = %d, should be %d\n", ntot, *ndim) ;
      goto error ;
    }
  }
  nd = 1 ;
  for(i=0 ; i<ntot ; i++) {                             // read the dimensions
    if( (nr = read(fd, &ni, nc)) != nc) goto error ;    // OOPS short read
    if(dims[i] == 0) dims[i] = ni ;                     // return dimension to caller
    if(dims[i] != ni){                                  // check dimension
      fprintf(stderr,"ERROR, dimensions mismatch\n");
      goto error ;
    }
    nd *= ni ;                                          // update size of array to be read
  }
  if(nd <= 0) {
    fprintf(stderr,"ERROR, one or more dimension is <= 0\n") ;
    goto error ;
  }
  buf = malloc(nd * 4) ;                                // allocate space to read record data[]
  if(buf == NULL) goto error ;                          // alloc failed
  if( (nr = read(fd, buf, nd*4)) < nd*4) {              // OOPS short read for data
    fprintf(stderr,"ERROR, attempted to read %ld bytes, got %ld\n", nd*4, nr) ;
    *ndata = nr/4 ;                                     // number of data read
    goto error ;
  }
  if( (nr = read(fd, &ntot, nc)) != nc) {               // check number of data from record
    fprintf(stderr,"ERROR, short read (number of data)\n");
    goto error ;       // OOPS short read
  }
  if(ntot != nd) {
    fprintf(stderr,"ERROR, inconsistent number of data ntot = %d. nd = %lu\n", ntot, nd);
    goto error ;  // inconsistent number of data
  }
  *ndata = ntot ;
end:
  if(*fdi < 0) close(fd) ;

  return buf ;

error:
fprintf(stderr,"ERROR in read_32bit_data_record\n");
  if(buf != NULL) free(buf) ;
  buf = NULL ;
  goto end ;
}

// write a 32 bit (integer or float) data record : 
// layout :  number_of_dimensions dimensions[number_of_dimensions] data[] number_of_data
// data array is in Fortran order (first index varying first)
// filename    [IN] : path/to/file (only valid if fdi == 0)
// fdi      [INOUT] : ABS(fdi) = file descriptor. if fdi < 0, close file after reading record
// dims[]      [IN] : array dimensions (max 10 dimensions) (OUT if ndim == 0, IN for check if ndim > 0)
// ndim        [IN] : number of dimensions (0 means unknown and get dimensions and number of dimensions)
// buf         [IN] : data to be written
// return : number of data items written (-1 in case of error)
int write_32bit_data_record(char *filename, int *fdi, int *dims, int ndim, void *buf){
  int fd = *fdi ;
  size_t nc = 4, nd ;
  ssize_t nr ;
  int ndims = ndim, ntot, i ;

  fd = (fd < 0) ? -fd : fd ;                                   // ABS(fd), negative fdi means close after reading record
  if(fd == 0 && filename[0] == '\0') return -1 ;               // filename should be valid
  if(fd != 0 && filename[0] != '\0') return -1 ;               // invalid fd / filename combination
  fd = (filename[0] != '\0') ? open(filename, O_WRONLY | O_CREAT, 0777) : fd ;    // open file if filename supplied
  if(fd < 0) return -1 ;                                       // open failed
  if(*fdi == 0) *fdi = fd ;                                    // new fd stored in fdi
  if(buf == NULL){                                             // just open or close the file if buf is NULL
    ntot = 0 ;
fprintf(stderr,"INFO: just closing fd = %d\n", fd);
    goto end ;
  }

  if( (nr = write(fd, &ndims, nc)) != nc) goto error ;         // number of dimensions
  if( (nr = write(fd, dims, ndim*nc)) != ndim*nc) goto error ; // dimensions
  ntot = 1 ;
  for(i=0 ; i<ndim ; i++) ntot *= dims[i] ;
  if( (nr = write(fd, buf, nc*ntot)) != nc*ntot) goto error ;  // data
  if( (nr = write(fd, &ntot, nc)) != nc) goto error  ;         // number of data
end:
  if(*fdi < 0) close(fd) ;
  return ntot ;
error:
  goto end ;
}
