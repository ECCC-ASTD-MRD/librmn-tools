#include <stdio.h>
#include <rmn/compress_data.h>

#define INDEX2D(array, col, lrow, row) ((array) + ((col)-1) + ((row)-1)*(lrow))

/*
                         a field is subdivided into chunks
                         (basic chunk size = 256 x 64)
                         (last chunk along a dimension may be shorter)
       <------ 256 ----->                                  <--- <= 256 ----->
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
 <= 64 | chunk(nci,0)   |                                  | chunk(nci,ncj) |<= 64
     | |                |                                  |                | |
     v +----------------+----------------------------------+----------------+ v
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
    64 | chunk(0,0)     |                                  | chunk(nci,0)   |64
     | |                |                                  |                | |
     v +----------------+----------------------------------+----------------+ v
       <------ 256 ----->                                  <--- <= 256 ----->

         each chunk is then subdivided into quantization/prediction blocks
         (basic block size = 64 x 64)
         (last block along a dimension may be shorter)
       <------ 64 ------>                                  <--- <= 64 ------>
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
 <= 64 | block(nbi,0)   |                                  | block(nbi,nbj) |<= 64
     | |                |                                  |                | |
     v +----------------+----------------------------------+----------------+ v
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
    64 | block(0,0)     |                                  | block(nbi,0)   |64
     | |                |                                  |                | |
     v +----------------+----------------------------------+----------------+ v
       <------ 64 ------>                                  <--- <= 64 ------>

                  each block is then subdivided into encoding tiles
                  (basic block size = 8 x 8)
                  (last block along a dimension may be shorter)
       <------- 8 ------>                                  <---- <= 8 ------>
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
  <= 8 |  tile(nti,0)   |                                  |  tile(nti,ntj) |<= 8
     | |                |                                  |                | |
     v +----------------+----------------------------------+----------------+ v
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
       |                |                                  |                |
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
     8 |  tile(0,0)     |                                  |  tile(nbi,0)   | 8
     | |                |                                  |                | |
     v +----------------+----------------------------------+----------------+ v
       <------- 8 ------>                                  <---- <= 8 ------>

  field compression goes as follows:
  - loop over chunks
    instantiate a bit stream
    insert chunk header into bit stream
    - loop over blocks
      quantize and predict (if useful) block
      insert block header into bit stream
      - loop over tiles
        insert tile header into bit stream
        encode tile into bit stream
    close bit stream
  we now have one bit stream per chunk, ready to be consolidated with field header

  field restoration goes as follows:
  - loop over chunks
    instantiate a bit stream using compressed field stream
    get chunk header from bit stream
    - loop over blocks
      get block header from bit stream
      - loop over tiles
        get tile header from bit stream
        decode tile from bit stream
      unpredict (if useful) and unquantize block
    close bit stream
  field restoration is now complete
*/

bitstream *compress_2d_chunk(void *data, int lni, int ni, int nj, compress_rules r){
  return NULL ;
}

// compress 2d 32 bit data according to rules r
// data is expected in "Fortran order"
// lni : data row storage dimension (>= ni)
// ni  : useful data row length
// nj  : number of data rows
// return an array of bit streams (NULL terminated)
bitstream **compress_2d_data(void *data, int lni, int ni, int nj, compress_rules r){
  uint32_t *f = (uint32_t *) data ;
  int i0, j0, cni, cnj, nci, ncj, i ;
  bitstream **streams = NULL ;
  bitstream **t ;

  nci = (ni+CHUNK_I-1) / CHUNK_I ;
  ncj = (nj+CHUNK_J-1) / CHUNK_J ;
  streams = (bitstream **) malloc( (nci*ncj+1) * sizeof(bitstream *) ) ;
  t = streams ;
  for(i=0 ; i <= nci*ncj ; i++) t[i] = NULL ;

  i = 0 ;
  for(j0 = 0 ; j0 < nj ; j0 += CHUNK_J){
    cnj = (j0+CHUNK_J > nj) ? nj-j0 : CHUNK_J ;
fprintf(stderr, "j0 = %4d, i0 =", j0);
    for(i0 = 0 ; i0 < ni ; i0 += CHUNK_I){
fprintf(stderr, "%5d", i0);
      cni = (i0+CHUNK_I > ni) ? ni-i0 : CHUNK_I ;
      t[i] = compress_2d_chunk(INDEX2D(f,i0,lni,j0), lni, cni, cnj, r) ;
      i++ ;
    }
fprintf(stderr, "%5d\n", i0);
  }
  return streams ;
}
