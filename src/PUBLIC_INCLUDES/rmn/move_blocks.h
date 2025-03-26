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

#if ! defined(MOVE_BLOCKS_INCLUDED)
#define MOVE_BLOCKS_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#include <rmn/data_kind.h>

// basic block block properties, set while gathering block
typedef struct{
  iuf32_t  maxs ;      // max signed value in block
  iuf32_t  mins ;      // min signed value in block
  iuf32_t  minu ;      // min unsigned value in block
  iuf32_t  maxu ;      // max unsigned value in block (needed for uint_data)
  int32_t  zeros ;     // number of ZERO values in block (-1 if unknown)
  data_kind kind ;  // data type (signed / unsigned / float / unknown)
} block_properties ;

// sfn argument list
typedef function_args sfn_args ;

// pointer to sfn function
typedef int (*sfn_ptr)(int lni, int ni, int nj, block_properties *bp, void *data, sfn_args *args) ;   // pointer to sfn processing function

// allocate an argument list with room for at most nmax arguments
// static inline sfn_args *malloc_sfn_args(uint32_t nmax) { return (sfn_args *) malloc_fn_args(nmax) ; }
#define malloc_sfn_args(arglist, nmax) { malloc_fn_args(arglist, nmax) ; }

// transform a float into a fake signed integer (comparison order preserving)
static inline int32_t fake_int(float f){
  iuf32_t iuf ;
  iuf.f = f ;
  return (iuf.i & 0x7FFFFFFF) ^ (iuf.i >> 31) ;
}

// restore float from fake integer representing float
static inline float unfake_float(int32_t fake){
  iuf32_t iuf ;
  iuf.i = ((fake >> 31) ^ fake) | (fake & 0x80000000) ;
  return iuf.f ;
}

// move_w32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj[, block_properties *bp]);
// generic interface to block movers. bp is absent if src is not a pointer to int/uint/float/void
#define move_w32_block(src,...) _Generic((src), \
                                                   int32_t  *: move_int32_block,  \
                                                   uint32_t *: move_uint32_block, \
                                                   float    *: move_float_block,  \
                                                   void     *: move_data32_block, \
                                                   default   : move_mem32_block   \
                                                   ) (src,__VA_ARGS__)

int move_uint32_block(uint32_t *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_int32_block(int32_t   *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_float_block(float     *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_data32_block(void     *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_mem32_block(void      *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj);

void print_float_props(block_properties bp);
void print_int_props(block_properties bp);

int analyze_data32_block(void *restrict src, int lnis, int ni, int nj, block_properties *bp);
void adjust_block_properties(block_properties *bp, data_kind datatype);
void add_block_properties(block_properties *bp, block_properties *bp_extra);

// int split_and_process(void *array, uint32_t lgni, uint32_t gni, uint32_t gnj, data_kind datatype, int ni, int nj, sfn_ptr fn, sfn_args *fnargs);
#endif
