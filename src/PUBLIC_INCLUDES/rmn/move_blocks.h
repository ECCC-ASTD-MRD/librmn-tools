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

#include <stdlib.h>
#include <stdint.h>

#include <rmn/data_kind.h>

// generic signed/unsigned/float 32 bit word
typedef union{
  int32_t  i ;    // signed integer
  uint32_t u ;    // unsigned integer
  float    f ;    // float
} iuf32_t ;

// basic block block properties, set while gathering block
typedef struct{
  iuf32_t  maxs ;      // max value in block
  iuf32_t  mins ;      // min value in block
  iuf32_t  minu ;      // min absolute value in block
  iuf32_t  maxu ;      // max absolute value in block (needed for uint_data)
  int32_t  zeros ;     // number of ZERO values in block (-1 if unknown)
  data_kind kind ;  // data type (signed / unsigned / float / unknown)
} block_properties ;

int32_t fake_int(float f);
float unfake_float(int32_t fake);

int move_word32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, data_kind datatype, block_properties *bp);

#define move_w32_block(src,lnis,dst,lnid,ni,nj,bp) _Generic((src), \
                                                   int32_t  *: move_int32_block,  \
                                                   uint32_t *: move_uint32_block, \
                                                   float    *: move_float_block,  \
                                                   void     *: move_data32_block  \
                                                   ) (src,lnis,dst,lnid,ni,nj,bp)

int move_uint32_block(uint32_t *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_int32_block(int32_t   *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_float_block(float     *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);

int move_data32_block(void     *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_mem32_block(void      *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);

int anal_data32_block(void *restrict src, int lnis, int ni, int nj, block_properties *bp);
void set_block_properties(block_properties *bp, data_kind datatype);
void fuse_block_properties(block_properties *bp, block_properties *bp_extra);

// generic 64 bit container
typedef union{
  double    d ;    // double
  void     *p ;    // address
  int64_t   l ;    // long long signed integer
  uint64_t lu ;    // long long unsigned integer
  int32_t   i ;    // signed integer
  uint32_t  u ;    // unsigned integer
  float     f ;    // float
} iuf64_t ;

typedef struct{
  uint64_t nargs ;      // number of arguments
  iuf64_t  args[] ;     // arguments ( [0] .. [nargs-1] )
} sfn_args ;            // processing function argument list

// allocate an argument list with room for at most nmax arguments
static inline sfn_args *malloc_fn_args(uint32_t nmax) { return (sfn_args *) malloc(sizeof(sfn_args) + nmax * sizeof(iuf64_t)) ; }

typedef int (*sfn_ptr)(int lni, int ni, int nj, block_properties *bp, void *data, sfn_args *args) ;   // pointer to processing function

// int split_and_process(void *array, uint32_t lgni, uint32_t gni, uint32_t gnj, data_kind datatype, int ni, int nj, sfn_ptr fn, sfn_args *fnargs);
