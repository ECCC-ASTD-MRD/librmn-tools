//
// Copyright (C) 2023  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2023
//

#include <stdint.h>

// generic signed/unsigned/float 32 bit word
typedef union{
  int32_t  i ;
  uint32_t u ;
  float    f ;
} iuf32_t ;
// basic block block properties, set while gathering block
typedef struct{
  iuf32_t  maxs ;    // max value in block
  iuf32_t  mins ;    // min value in block
  iuf32_t  min0 ;    // min NON ZERO absolute value in block
  int32_t  zeros ;   // number of ZERO values in block
} block_properties ;

int get_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj) ;
int put_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj) ;

int gather_int32_block(int32_t *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp) ;
int gather_float_block(float *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp) ;

int scatter_word_block(void *restrict src, void *restrict blk, int ni, int lni, int nj) ;
