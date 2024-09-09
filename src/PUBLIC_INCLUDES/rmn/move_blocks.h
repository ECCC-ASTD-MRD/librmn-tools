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
  int32_t  i ;    // signed integer
  uint32_t u ;    // unsigned integer
  float    f ;    // float
} iuf32_t ;

// generic 64 bit word
typedef union{
  double    d ;    // double
  void   *ptr ;    // address
  int64_t   l ;    // long long signed integer
  uint64_t lu ;    // long long unsigned integer
  int32_t   i ;    // signed integer
  uint32_t  u ;    // unsigned integer
  float     f ;    // float
} iuf64_t ;

// expected data types for block properties
typedef enum {
  bad_data = 0,     // invalid or unknown
  int_data = 1,     // signed integers
  uint_data = 2,    // unsigned integers
  float_data = 3,   // floats
  raw_data = 4      // any 32 bit quantities (block_properties meaningless in that case)
} int_or_float ;
static char *printable_type[] = { "BAD", "SIGNED", "UNSIGNED", "FLOAT", "RAW32" } ;

// basic block block properties, set while gathering block
typedef struct{
  iuf32_t  maxs ;      // max value in block
  iuf32_t  mins ;      // min value in block
  iuf32_t  minu ;      // min absolute value in block
  iuf32_t  maxu ;      // max absolute value in block (needed for uint_data)
  int32_t  zeros ;     // number of ZERO values in block (-1 if unknown)
  int_or_float kind ;  // data type (signed / unsigned / float / unknown)
} block_properties ;

int32_t fake_int(float f);
float unfake_float(int32_t fake);

int move_word32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, int_or_float datatype, block_properties *bp);

int move_uint32_block(int32_t *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_int32_block(int32_t  *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);
int move_float_block(float    *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp);

typedef struct{
  uint64_t nargs ;      // number of arguments
  iuf64_t  args[] ;     // arguments ( [0] .. [nargs-1] )
} fn_args ;             // processing function argument list

typedef int (*fnptr)(int lni, int ni, int nj, block_properties *bp, void *data, fn_args *args) ;   // pointer to processing function

int split_and_process(void *array, uint32_t lgni, uint32_t gni, uint32_t gnj, int_or_float datatype, int ni, int nj, fnptr fn, fn_args *fnargs);

// int get_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj) ;
// int put_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj) ;

// int gather_int32_block(int32_t *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp) ;
// int gather_float_block(float *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp) ;

// int scatter_word32_block(void *restrict src, void *restrict blk, int ni, int lni, int nj) ;
// int gather_word32_block(void *restrict src, void *restrict blk, int ni, int lni, int nj) ;
