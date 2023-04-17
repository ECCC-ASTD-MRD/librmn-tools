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

// read 32 bit (integer or float) data record : 
// layout :  number_of_dimensions dimensions[number_of_dimensions] data[] number_of_dimensions
// data array is in Fortran order (first index varying first)
// filename    [IN] : path/to/file (only valid if fdi == 0)
// fdi      [INOUT] : ABS(fdi) = file descriptor. if fdi < 0, close file after reading record
// dims[]   [INOUT] : array dimensions (max 10 dimensions)
// ndim     [INOUT] : number of dimensions (0 means unknown)
// return : pointer to data array from file
void *read_32bit_data_record(char *filename, int *fdi, int *dims, int *ndim){
  int fd = *fdi ;
  fd = (fd < 0) ? -fd : fd ;   // ABS(fd)

  if(fd == 0 && filename == NULL) return NULL ;                // filename should be valid
  if(fd != 0 && filename != NULL) return NULL ;                // invalid fd / filename combination
  fd = (filename != NULL) ? open(filename, O_RDONLY) : fd ;    // open file if filename supplied
  if(fd < 0) return NULL ;                                     // open failed
  if(*fdi == 0) *fdi = fd ;

  size_t nc = 4, nd ;
  ssize_t nr ;
  void *buf = NULL ;
  int ntot ;
  int i, ni ;

  nr = read(fd, &ntot, nc) ;  // read number of dimensions
  if(nr != nc) {
    fprintf(stderr,"ERROR, short read\n");
    return NULL ;
  }
  if(ntot > 10){
    fprintf(stderr,"ERROR, more than 10 dimensions (%d)\n", ntot) ;
    return NULL ;
  }
// fprintf(stderr,"number of dimensions : %d\n", ntot) ;
  if(*ndim > 0 && ntot != *ndim) {
    fprintf(stderr,"ERROR, read ndim = %d, should be %d\n", ntot, *ndim) ;
    goto error ;
  }
  nd = 1 ;
  for(i=0 ; i<ntot ; i++) {      // read the dimensions
    nr = read(fd, &ni, nc) ;
    if(nr != nc) goto error ;    // OOPS short read
    if(*ndim == 0){
      dims[i] = ni ;             // store dimensions
//       fprintf(stderr,"DEBUG read ni = %d\n", ni) ;
    }else{
      if(dims[i] != ni){         // check dimensions
        fprintf(stderr,"ERROR, dimensions mismatch\n");
        goto error ;
      }
    }
    nd *= ni ;                   // update size of array to read
  }
  if(*ndim == 0) {
    *ndim = ntot ;  // store number of dimensions
//     fprintf(stderr,"DEBUG, *ndim set to %d\n", ntot);
  }
  if(nd <= 0) fprintf(stderr,"ERROR, one or more dimension is <= 0\n") ;

  buf = malloc(nd * 4) ;         // allocate space to read record data[]
  if(buf == NULL) goto error ;   // alloc failed
  nr = read(fd, buf, nd*4) ;     // read data
  if(nr < nd*4) {                // OOPS short read
    fprintf(stderr,"ERROR, attempted to read %ld bytes, got %ld\n", nd*4, nr) ;
    goto error ;
  }else{
//     fprintf(stderr,"DEBUG read %ld bytes\n", nr) ;
  }
  nr = read(fd, &ntot, nc) ;      // check number of dimensions
  if(nr != nc) {
    fprintf(stderr,"ERROR, short read (number of dimensions)\n");
    goto error ;       // OOPS short read
  }
  if(ntot != nd) {
    fprintf(stderr,"ERROR, inconsistent number of dimensions ntot = %d. nd = %d\n", ntot, nd);
    goto error ;  // inconsistent number of dimensions
  }

end:
  if(*fdi < 0) {
    fprintf(stderr,"DEBUG : closing fd = %d\n", fd);
    close(fd) ;
  }
  return buf ;

error:
fprintf(stderr,"ERROR in read_32bit_data_record\n");
  if(buf != NULL) free(buf) ;
  buf = NULL ;
  goto end ;
}

#if defined(SELF_TEST)
static inline int indx3(int i, int j, int k, int ni, int nj,  int nk){
  return (i + ni*j + nj*k) ;
}

void FDWT53i_8x8_3level(int32_t *x, int lni);

int main(int argc, char **argv){
  int ni, nj, nk, ndim, fd ;
  float *data = NULL ;
  int32_t *q = NULL ;

  if(argc < 2){
    fprintf(stderr,"USAGE: %s file_name\n", argv[0]) ;
  }
  ndim = 2 ; ni = nj = nk = 1 ;
  if(argc >2) ndim = atoi(argv[2]) ;

  data = read_raw_data(argv[1], fd = 0, &ni, &nj, &nk, &ndim) ;
  q = malloc(ni*nj*nk*sizeof(int32_t)) ;

  if(data == NULL){
    fprintf(stderr,"ERROR reading %dD %s\n", ndim, argv[1]) ;
  }else{
    fprintf(stderr,"INFO: ndim = %d, ni = %d, nj = %d, nk = %d, data[0] = %11f, data[last] = %11f (%s)\n", 
            ndim, ni, nj, nk, data[0], data[ni*nj*nk-1], argv[1]) ;
  }

  int base = indx3(ni/2, nj/2, 1, ni, nj, nk) ;
  FDWT53i_8x8_3level(q+base, ni) ;
}
#endif
