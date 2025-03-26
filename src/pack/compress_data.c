// Hopefully useful code for C
// Copyright (C) 2022-2025  Recherche en Prevision Numerique
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
// #include <rmn/compress_data.h>

// Fortran style indexing (column major)  array(i,j) is array(col,row)
// #define INDEX2D_C(array, col, lrow, row) ((array) + (col) + (row)*(lrow))

//
//          a FIELD is normally subdivided into into quantization/prediction BLOCKS
//          (basic block size = 64 x 64)
//          - (the first block along a dimension may be shorter [ normally at least 32 x 32])
//          - (the first block along a dimension may be longer [ up to 95 x 95])
//        <- >= 32 , < 96 ->                                  <------- 64 ------>
//      ^ +----------------+----------------+.....    .....+----------------+ ^
//      | |                |                |              |                | |
//     64 | block(0,nbj)   | block(1,nbj)   |              | block(nbi,nbj) | 64
//      | |                |                |              |                | |
//      x +----------------+----------------+.....    .....+----------------+ x
//        ........................................    .......................
//      x +----------------+----------------+.....    .....+----------------+ x
//      | |                |                |              |                | |
//     64 | block(0,1)     | block(1,1)     |              | block(nbi,1)   | 64
//      | |                |                |              |                | |
//      x +----------------+----------------+.....    .....+----------------+ x
//      | |                |                |              |                | |
//  >= 32 | block(0,0)     | block(1,0)     |              | block(nbi,0)   |>= 32 , < 96
//      | |                |                |              |                | |
//      v +----------------+----------------+.....    .....+----------------+ v
//        <- >= 32 , < 96 -x------- 64 ----->              <------- 64 ----->
// 
//        single row, FULL blocks along J
//      ^ +----------------+----------------+.....    .....+-----------------+ ^
//      | |                |                |              |                 | |
//     64 |   block(0)     |   block(1)     |              |   block(nbi)    |64
//      | |                |                |              |                 | |
//      v +----------------+----------------+.....    .....+-----------------+ v
//        <- >= 32 , < 96 -x------- 64 ----->              <------- 64 ------>
// 
//        single row, SHORT/LONG blocks along J
//      ^ +----------------+----------------+.....    .....+-----------------+ ^
//      | |                |                |              |                 | |
//   >= 1 |   block(0)     |   block(1)     |              |   block(nbi)    |>= 1 , < 96
//      | |                |                |              |                 | |
//      v +----------------+----------------+.....    .....+-----------------+ v
//        <- >= 32 , < 96 -x------- 64 ----->              <------- 64 ------>
// 
//         single column            single column
//        SHORT/LONG along I       FULL blocks along I
//      ^ +--------------+         +--------------+ ^
//      | |              |         |              | |
//     64 | block(nbj)   |         | block(nbj)   | 64
//      | |              |         |              | |
//      v +--------------+         +--------------+ v
//        ................         ................
//      ^ +--------------+         +--------------+ ^
//      | |              |         |              | |
//     64 | block(1)     |         | block(1)     | 64
//      | |              |         |              | |
//      x +--------------+         +--------------+ x
//      | |              |         |              | |
//  >= 32 | block(0)     |         | block(0)     |>= 32 , < 96
//      | |              |         |              | |
//      v +--------------+         +--------------+ v
//        < >= 32 , < 96 >         <----- 64 ----->
// 
//        single SHORT/LONG block along I and J
//      ^ +----------------+ ^
//      | |                | |
//   >= 1 |   block(0)     |>= 1 , < 96
//      | |                | |
//      v +----------------+ v
//        <- >= 1 , < 96 -->
// 
//         each BLOCK is then subdivided into encoding TILES
//         (basic tile size = 8 x 8)
//         either
//         - (the first tile along a dimension may be shorter) [normally at least 4 x 4]
//         - (the first tile along a dimension may be longer [up to 15 x 15])
//        <-- > 3 , < 16 -->                                  <------- 8 ------>
//      ^ +----------------+----------------+.....    .....+----------------+ ^
//      | |                |                |              |                | |
//      8 |  tile(0,ntj)   |  tile(1,ntj)   |              |  tile(nti,ntj) | 8
//      | |                |                |              |                | |
//      v +----------------+----------------+.....    .....+----------------+ v
//        ........................................    .......................
//      ^ +----------------+----------------+.....    .....+----------------+ ^
//      | |                |                |              |                | |
//      8 |  tile(0,1)     |  tile(1,1)     |              |  tile(nti,1)   | 8
//      | |                |                |              |                | |
//      ^ +----------------+----------------+              +----------------+ ^
//      | |                |                |              |                | |
//    > 3 |  tile(0,0)     |  tile(1,0)     |.....    .....|  tile(nti,0)   |> 3 , < 16
//      | |                |                |              |                | |
//      v +----------------+----------------+.....    .....-----------------+ v
//        <-- > 3 , < 16 --x------- 8 ------>              <------- 8 ------>
// 
//        single row
//      ^ +--------------+--------------+              +--------------+ ^
//      | |              |              |              |              | |
//  >=1 |  tile(0)       |  tile(1)     |.....    .....|  tile(nti)   |>=1 , < 16
//      | |              |              |              |              | |
//      v +--------------+--------------+.....    .....---------------+ v
//        <- > 3 , < 16 -x------ 8 ----->              <------ 8 ----->
// 
//        single column
//      ^ +--------------+  ^
//      | |              |  |
//      8 |  tile(ntj)   |  8
//      | |              |  |
//      v +--------------+  v
//        ................
//      ^ +--------------+  ^
//      | |              |  |
//      8 |  tile(1)     |  8
//      | |              |  |
//      ^ +--------------+  ^
//      | |              |  |
//    > 3 |  tile(0)     |  >3 , < 16
//      | |              |  |
//      v +--------------+  v
//        <- > 1 , < 16 -x
// 
//   field compression goes as follows:
//   - loop over chunks
//     instantiate a bit stream
//     insert chunk header into bit stream
//     - loop over blocks
//       quantize and predict (if useful) block
//       insert block header into bit stream
//       - loop over tiles
//         insert tile header into bit stream
//         encode tile into bit stream
//     close bit stream
//   we now have one bit stream per chunk, ready to be consolidated with field header
// 
//   field restoration goes as follows:
//   - loop over chunks
//     instantiate a bit stream using compressed field stream
//     get chunk header from bit stream
//     - loop over blocks
//       get block header from bit stream
//       - loop over tiles
//         get tile header from bit stream
//         decode tile from bit stream
//       unpredict (if useful) and unquantize block
//     close bit stream
//   field restoration is now complete
/*

  DATA MAP (map of chunk positions in data stream) (sizes and offsets in 32 bit units)
           (see typedef data_map in compress_data.h)
           (chunk size limited to 16GBytes)
  2024/08/xx
  evolution 4d (no 16GB limit on offsets, 256KB limit on chunk size) simpler map, no "small" chunks
  BCI (16 bits), BCJ (16 bits) : chunk dimensions (normally a multiple of 8)
  BIX (16 bits) : dimension along I of the first/last chunk in all rows (chunks in first/last column)
  BJX (16 bits) : dimension along J of chunks in the first/last row (chunks in first/last row)
  NPI = nb of points along i, NPJ = nb of points along j
  NCI = NPI/BCI, NCJ = NPJ/BCJ (number of chunks along i and j)
  the first chunk (BIX, BJX) may be smaller or larger than the following one(s)
  small blocks are unwanted (BIX is expected to be >= BCI / 2, BJX is expected to be >= BCJ / 2)
  BCI <= BIX < BCI * 2, BCJ <= BJX < BCJ * 2
  BIX == 0 means BIX == BCI (NPI is a multiple if BCI), BJX == 0 means BJX == BCJ (NPJ is a multiple if BCJ)
  data map size = (NCI * NCJ +1) / 2 + 3 (in 32 bit units)  N = NCI * NCJ
  index to block number translation : blocki = (i - (BIX - BCI)) / BCI, same method for blockj
  +-------+-------+-------+-------+-------+-------+--------------+--------------+     +--------------+
  |  NPI  |  NPJ  |  BIX  |  BCI  |  BJX  |  BCJ  | Chunk 1 size | Chunk 2 size | ... | Chunk n size |
  +-------+-------+-------+-------+-------+-------+--------------+--------------+     +--------------+
  <--32b--x--32b--x------32b------x------32b------x-------------32b------------->     x-----16b------>

  FIELD layout (field header and chunk sizes are multiples of 32 bits)
                 <-------- Chunk size 1 --------->     <-------- Chunk size n --------->
  +--------------+----------------+--------------+     +----------------+--------------+
  | Field Header | Chunk Header 1 | Chunk Data 1 | ... | Chunk Header n | Chunk Data n |
  +--------------+----------------+--------------+     +----------------+--------------+
                 ^                                     ^
                 | Chunk offset 1                      | Chunk offset n

  CHUNK layout (PAD : 0 -> 31 bits) (blocks are 32 bit aligned in data stream) (CL is a multiple of 32 bits)
  +--------------+----------------+--------------+-------+     +----------------+--------------+-------+
  | Chunk Header | Block Header 1 | Block Data 1 | PAD 1 | ... | Block Header n | Block Data n | PAD n |
  +--------------+----------------+--------------+-------+     +----------------+--------------+-------+
  <--- CL bits --x----------------------------------- Chunk data -------------------------------------->
  <-------------------------------------------------- Chunk size -------------------------------------->

  quantization/prediction BLOCK layout (block size is a multiple of 32 bits) (expected blocks : 64 x 64)
  +--------------+---------------+-------------+     +---------------+-------------+-----+
  | Block Header | Tile Header 1 | Tile Data 1 | ... | Tile Header n | Tile Data n | PAD |
  +--------------+---------------+-------------+     +---------------+-------------+-----+
  <--- BL bits --x-------------------------- Block data --------------------------------->
  <----------------------------------------- Block size --------------------------------->

  encoding TILE layout (unaligned bit stream)
  +-------------+-----------+
  | Tile Header | Tile Data |
  +-------------+-----------+
  <-- TL bits -->
  (see tile_encoders.h for encoding tile layout)
*/
#if 0
// compress a 2D block as tiles into bit stream using compress rules
void compress_2d_block(void *data, int lni, int ni, int nj, compress_rules r, bitstream *stream){
}

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


#endif
