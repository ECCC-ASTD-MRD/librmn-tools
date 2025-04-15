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

#if ! defined(move_w32_block)

#include <stdlib.h>
#include <stdint.h>

#include <rmn/data_kind.h>
#include <rmn/data_properties.h>

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

static inline int int_max_abs(block_properties bp){
  uint32_t max1, max2 ;
  if(bp.maxs.i <= 0) return -bp.mins.i ;  // all negative or 0
  if(bp.mins.i >= 0) return bp.maxs.i ;   // all positive or 0
  max1 = bp.maxs.i ;      // largest positive value
  max2 = -bp.mins.i ;     // largest negative value
  return (max1 > max2) ? max1 : max2 ;
}

static inline int int_min_abs(block_properties bp){
  uint32_t min1, min2 ;
  if(bp.maxs.i <= 0) return -bp.maxs.i ;  // all negative or 0
  if(bp.mins.i >= 0) return bp.mins.i ;   // all positive or 0
  min1 = bp.maxu.i ;      // smallest positive value
  min2 = -bp.maxu.i ;     // smallest negative value
  return (min1 < min2) ? min1 : min2 ;
}

#define FLOAT_MAX_VALUE(BP) ( ((BP).kind == float_data) ? (BP).maxs.f : fp32_nan(0) )
#define FLOAT_MIN_VALUE(BP) ( ((BP).kind == float_data) ? (BP).mins.f : fp32_nan(0) )
#define FLOAT_MAX_ABS(BP)   ( ((BP).kind == float_data) ? (BP).maxu.f : fp32_nan(0) )
#define FLOAT_MIN_ABS(BP)   ( ((BP).kind == float_data) ? (BP).minu.f : fp32_nan(0) )

#define INT_MAX_VALUE(BP)   ( ((BP).kind == int_data) ? (BP).maxs.i     : 0x80000000 )
#define INT_MIN_VALUE(BP)   ( ((BP).kind == int_data) ? (BP).mins.i     : 0x7FFFFFFF )
#define INT_MAX_ABS(BP)     ( ((BP).kind == int_data) ? int_max_abs(BP) : 0x00000000 )
#define INT_MIN_ABS(BP)     ( ((BP).kind == int_data) ? int_min_abs(BP) : 0xFFFFFFFF )

#define UINT_MAX_VALUE(BP)   ( ((BP).kind == uint_data) ? (BP).maxu.u : 0x00000000u )
#define UINT_MIN_VALUE(BP)   ( ((BP).kind == uint_data) ? (BP).minu.u : 0xFFFFFFFFu )
#define UINT_MAX_ABS(BP)     ( ((BP).kind == uint_data) ? (BP).maxu.u : 0x00000000u )
#define UINT_MIN_ABS(BP)     ( ((BP).kind == uint_data) ? (BP).minu.u : 0xFFFFFFFFu )

// int split_and_process(void *array, uint32_t lgni, uint32_t gni, uint32_t gnj, data_kind datatype, int ni, int nj, sfn_ptr fn, sfn_args *fnargs);
#endif
