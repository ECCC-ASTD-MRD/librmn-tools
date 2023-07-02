// Hopefully useful code for C
// Copyright (C) 2022  Recherche en Prevision Numerique
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

#include <stdio.h>
#include <rmn/compress_data.h>

#define INDEX2D_C(array, col, lrow, row) ((array) + (col) + (row)*(lrow))

/*
                         a field is subdivided into chunks
                         (basic chunk size = 256 x 64)
                         (last chunk along a dimension may be shorter)
       <------ 256 ----->                                  <--- <= 256 ----->
     ^ +----------------+----------------------------------+----------------+ ^
     | |                |                                  |                | |
 <= 64 | chunk(0,ncj)   |                                  | chunk(nci,ncj) |<= 64
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
 <= 64 | block(0,nbj)   |                                  | block(nbi,nbj) |<= 64
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
  <= 8 |  tile(0,ntj)   |                                  |  tile(nti,ntj) |<= 8
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
     8 |  tile(0,0)     |                                  |  tile(nti,0)   | 8
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
/*

  data map (map of chunk positions in data stream) (sizes and offsets in 32 bit units)
  data map size = (NCI * NCJ) * 2 + 2 (in 32 bit units)
  +-------+-------+--------------+----------------+     +--------------+----------------+
  |  NCI  |  NCJ  | Chunk size 1 | Chunk offset 1 | ... | Chunk size n | Chunk offset n |
  +-------+-------+--------------+----------------+     +--------------+----------------+
  <--32b--x--32b--x-----32b------x------32b------->     <-----32b------x------32b------->

  field layout (chunks are 32 bit aligned in data stream)
                 <-------- Chunk size 1 --------->     <-------- Chunk size n --------->
  +--------------+----------------+--------------+     +----------------+--------------+
  | Field Header | Chunk Header 1 | Chunk Data 1 | ... | Chunk Header n | Chunk Data n |
  +--------------+----------------+--------------+     +----------------+--------------+
                 ^                                     ^
                 | Chunk offset 1                      | Chunk offset n

  chunk layout (PAD : 0 -> 31 bits)
  +--------------+----------------+--------------+     +----------------+--------------+-----+
  | Chunk Header | Block Header 1 | Block Data 1 | ... | Block Header n | Block Data n | PAD |
  +--------------+----------------+--------------+     +----------------+--------------+-----+
  <--- 32 bits --x------------------------------- Chunk data -------------------------------->

  quantization block layout
  +--------------+---------------+-------------+     +---------------+-------------+
  | Block Header | Tile Header 1 | Tile Data 1 | ... | Tile Header n | Tile Data n |
  +--------------+---------------+-------------+     +---------------+-------------+
  <--- 96 bits --x-------------------------- Block data --------------------------->

  encoding tile layout
  +-------------+-----------+
  | Tile Header | Tile Data |
  +-------------+-----------+
  <-- 16 bits -->
  (see tile_encoders.h for encoding tile layout)
*/

bitstream *compress_2d_chunk(void *data, int lni, int ni, int nj, compress_rules r){
//   bitstream *t ;
  // allocate stream at worst case length
  // bit stream must be flushed at end of chunk
  return NULL ;
}

// compress 2d 32 bit data according to rules r
// data is expected in "Fortran order"
// lni : data row storage dimension (>= ni)
// ni  : useful data row length
// nj  : number of data rows
// return a data map / data 
compressed_field compress_2d_data(void *data, int lni, int ni, int nj, compress_rules r){
  uint32_t *f = (uint32_t *) data ;
  int i0, j0, cni, cnj, nci, ncj, i ;
  compressed_field field = {NULL, NULL} ;
  bitstream **t ;
  uint64_t chunks_size = 0l ;

  nci = (ni+CHUNK_I-1) / CHUNK_I ;
  ncj = (nj+CHUNK_J-1) / CHUNK_J ;
  t = (bitstream **) malloc( (nci*ncj) * sizeof(bitstream *) ) ;

  for(i=0 ; i <= nci*ncj ; i++) t[i] = NULL ;

  i = 0 ;
  for(j0 = 0 ; j0 < nj ; j0 += CHUNK_J){
    cnj = (j0+CHUNK_J > nj) ? nj-j0 : CHUNK_J ;
fprintf(stderr, "j0 = %4d, i0 =", j0);
    for(i0 = 0 ; i0 < ni ; i0 += CHUNK_I){
fprintf(stderr, "%5d", i0);
      cni = (i0+CHUNK_I > ni) ? ni-i0 : CHUNK_I ;
      t[i] = compress_2d_chunk(INDEX2D_C(f,i0,lni,j0), lni, cni, cnj, r) ;
      chunks_size += ( (t[i]->out - t[i]->in) * sizeof(uint32_t) ) ;
      i++ ;
    }
fprintf(stderr, "%5d\n", i0);
  }
  return field ;
}
