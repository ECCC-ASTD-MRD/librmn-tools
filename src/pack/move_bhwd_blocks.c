// Hopefully useful code for C (memory block movers)
// Copyright (C) 2026  Recherche en Prevision Numerique
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
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2026
//
#include <stdio.h>
#include <rmn/move_bhwd_blocks.h>

#include <rmn/identify_fc_compiler.h>
#if defined(COMPILER_IS_GCC)
#pragma GCC optimize "tree-vectorize"
#endif

//
// ============ auxiliary tables and internal variables ============
static int DEBUG = 0 ;
//
// length in bytes of array elements indexed by type code
int element_bytes[MAX_ARRAY_TYPES] = { 1, 1, 2, 2, 4, 4, 4, 8, 8, 8 } ;
// {
//   element_length(UINT8_T ) ,
//   element_length(INT8_T  ) ,
//   element_length(UINT16_T) ,
//   element_length(INT16_T ) ,
//   element_length(UINT32_T) ,
//   element_length(INT32_T ) ,
//   element_length(FLOAT   ) ,
//   element_length(UINT64_T) ,
//   element_length(INT64_T ) ,
//   element_length(DOUBLE  )
// } ;

// array to block copy functions table indexed by type code
bhwd_fn into_bhwd[MAX_ARRAY_TYPES] =
{
  (bhwd_fn)move_u8_to_u32 ,
  (bhwd_fn)move_i8_to_i32 ,
  (bhwd_fn)move_u16_to_u32,
  (bhwd_fn)move_i16_to_i32,
  (bhwd_fn)move_i32_to_blk,
  (bhwd_fn)move_u32_to_blk,
  (bhwd_fn)move_flt_to_blk,
  (bhwd_fn)move_u64_to_u32,
  (bhwd_fn)move_i64_to_i32,
  (bhwd_fn)move_d64_to_f32
} ;

// block to array copy functions table indexed by type code
bhwd_fn from_bhwd[MAX_ARRAY_TYPES] =
{
  (bhwd_fn)move_u32_to_u8 ,
  (bhwd_fn)move_i32_to_i8 ,
  (bhwd_fn)move_u32_to_u16,
  (bhwd_fn)move_i32_to_i16,
  (bhwd_fn)move_blk_to_i32,
  (bhwd_fn)move_blk_to_u32,
  (bhwd_fn)move_blk_to_flt,
  (bhwd_fn)move_u32_to_u64,
  (bhwd_fn)move_i32_to_i64,
  (bhwd_fn)move_f32_to_d64
} ;

//
// ============ control functions ============
//
int set_bhwd_debug(int value){
  int old = DEBUG ;
  DEBUG = value ;
  return old ;
}
//
// ============ 8 bits to/from 32 bits ============
//
// w32 [IN/OUT] : 32 bit integer block[nj][ni]
// bhd [IN/OUT] : 8 bit integer array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
// get subarray of u8 into u32 block (unsigned)
void move_u8_to_u32(uint32_t * restrict w32, uint8_t * restrict bhd, int lni, int ni, int nj, block_properties *bp, int z){  // unsigned 8 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_u8_to_u32,  lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = bhd[i]; };
    w32 +=  ni ;
    bhd += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
//
// store subarray of u8 from u32 block (unsigned)
void move_u32_to_u8(uint8_t * restrict bhd, uint32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // unsigned 32 -> 8
  (void) (z) ; (void) (bp) ;
  int i, iter = 0 ;
if(DEBUG > 0)  fprintf(stderr, "move_u32_to_u8,  lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    iter++ ;
    for(i=0 ; i<ni; i++){ bhd[i] = (w32[i] > UINT8_MAX) ? UINT8_MAX : w32[i] ; };
    w32 +=  ni ;
    bhd += lni ;
  }
}
//
// get subarray of i8 into i32 block (signed)
void move_i8_to_i32(int32_t * restrict w32, int8_t * restrict bhd, int lni, int ni, int nj, block_properties *bp, int z){  // signed 8 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_i8_to_i32,  lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = bhd[i]; };
    w32 +=  ni ;
    bhd += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
//
// store subarray of u8 from i32 block (signed)
void move_i32_to_i8(int8_t * restrict bhd, int32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // signed 32 -> 8
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_i32_to_i8,  lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ int32_t t = w32[i] ; t = (t>INT8_MAX) ? INT8_MAX : t ; t = (t < INT8_MIN) ? INT8_MIN : t ; bhd[i] =t; };
    w32 +=  ni ;
    bhd += lni ;
  }
}
//
// ============ 16 bits to/from 32 bits ============
//
// w32 [IN/OUT] : 32 bit integer block[nj][ni]
// bhd [IN/OUT] : 16 bit integer array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
// get subarray of u16 into u32 block (unsigned)
void move_u16_to_u32(uint32_t * restrict w32, uint16_t * restrict bhd, int lni, int ni, int nj, block_properties *bp, int z){  // unsigned 16 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_u16_to_u32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = bhd[i]; };
    w32 +=  ni ;
    bhd += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
//
// store subarray of u16 from u32 block (unsigned)
void move_u32_to_u16(uint16_t * restrict bhd, uint32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // unsigned 32 -> 16
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_u32_to_u16, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ bhd[i] = (w32[i] > UINT16_MAX) ? UINT16_MAX : w32[i] ; };
    w32 +=  ni ;
    bhd += lni ;
  }
}
//
// get subarray of i16 into i32 block (signed)
void move_i16_to_i32(int32_t * restrict w32, int16_t * restrict bhd, int lni, int ni, int nj, block_properties *bp, int z){  // signed 16 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_i16_to_i32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = bhd[i]; };
    w32 +=  ni ;
    bhd += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
//
// store subarray of i16 from i32 block (signed)
void move_i32_to_i16(int16_t * restrict bhd, int32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // signed 32 -> 16
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_i32_to_i16, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ int32_t t = w32[i] ; t = (t>INT16_MAX) ? INT16_MAX : t ; t = (t < INT16_MIN) ? INT16_MIN : t ; bhd[i] =t; };
    w32 +=  ni ;
    bhd += lni ;
  }
}
//
// ============ 32 bits to/from 32 bits ============
//
// blk [IN/OUT] : 32 bit block[nj][ni]
// w32 [IN/OUT] : 32 bit array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value
// NOTE: expected value of z is 0, + z prevents the optimizer from calling memcpy
//
// to_blk copies from array[nj][lni] to block[nj][ni]
static void to_blk(void * restrict blk_, void * restrict w32_, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  int32_t *blk = (int32_t *)blk_, *w32 = (int32_t *)w32_ ;
  int i ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ blk[i] = w32[i] + z ; };
    blk +=  ni ;
    w32 += lni ;
  }
}
// to_w32 copies from block[nj][ni] to array[nj][lni]
static void to_w32(void * restrict w32_, void * restrict blk_, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  int32_t *blk = (int32_t *)blk_, *w32 = (int32_t *)w32_ ;
  int i ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = blk[i] + z ; };
    blk +=  ni ;
    w32 += lni ;
  }
}
void move_u32_to_blk(uint32_t * restrict blk, uint32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // 32 array -> 32 block
  (void) (z) ; int nij = ni*nj ;
if(DEBUG > 0)  fprintf(stderr, "move_blk_to_u32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  to_blk(blk, w32, lni, ni, nj, z) ;
  if(bp != NULL){ *bp = get_block_properties(blk, nij) ; }
}
void move_blk_to_u32(uint32_t * restrict w32, uint32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z){  // 32 block -> 32 array
  (void) (z) ; (void) (bp) ;
if(DEBUG > 0)  fprintf(stderr, "move_blk_to_u32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_w32(w32, blk, lni, ni, nj, z) ;
}
void move_i32_to_blk(int32_t * restrict blk, int32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // 32 array -> 32 block
  (void) (z) ; int nij = ni*nj ;
if(DEBUG > 0)  fprintf(stderr, "move_i32_to_blk, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  to_blk(blk, w32, lni, ni, nj, z) ;
  if(bp != NULL){ *bp = get_block_properties(blk, nij) ; }
}
void move_blk_to_i32(int32_t * restrict w32, int32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z){  // 32 block -> 32 array
  (void) (z) ; (void) (bp) ;
if(DEBUG > 0)  fprintf(stderr, "move_blk_to_i32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_w32(w32, blk, lni, ni, nj, z) ;
}
void move_flt_to_blk(float * restrict blk, float * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // 32 array -> 32 block
  (void) (z) ; int nij = ni*nj ;
if(DEBUG > 0)  fprintf(stderr, "move_flt_to_blk, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  to_blk(blk, w32, lni, ni, nj, z) ;
  if(bp != NULL){ *bp = get_block_properties(blk, nij) ; }
}
void move_blk_to_flt(float * restrict w32, float * restrict blk, int lni, int ni, int nj, block_properties *bp, int z){  // 32 block -> 32 array
  (void) (z) ; (void) (bp) ;
if(DEBUG > 0)  fprintf(stderr, "move_blk_to_flt, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_w32(w32, blk, lni, ni, nj, z) ;
}
//
// ============ 64 bits to/from 32 bits ============
//
// w32 [IN/OUT] : 32 bit integer block[nj][ni]
// bhd [IN/OUT] : 64 bit integer array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
// get subarray of u64 into u32 block (unsigned)
void move_u64_to_u32(uint32_t * restrict w32, uint64_t * restrict bhd, int lni, int ni, int nj, block_properties *bp, int z){  // unsigned 64 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_u64_to_u32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = (bhd[i] > UINT32_MAX) ? UINT32_MAX : bhd[i] ; };  // copy with saturation
    w32 +=  ni ;
    bhd += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
//
// store subarray of u64 from u32 block (signed)
void move_u32_to_u64(uint64_t * restrict bhd, uint32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // unsigned 32 -> 64
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_u32_to_u64, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ bhd[i] = w32[i]; };
    w32 +=  ni ;
    bhd += lni ;
  }
}
//
// get subarray of i64 into i32 block (unsigned)
void move_i64_to_i32(int32_t * restrict w32, int64_t * restrict bhd, int lni, int ni, int nj, block_properties *bp, int z){  // signed 64 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_i64_to_i32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){
      int64_t t = bhd[i] ;
      t = (t > INT32_MAX) ? INT32_MAX : t ;  // copy with saturation
      t = (t < INT32_MIN) ? INT32_MIN : t ;  // copy with saturation
      w32[i] =t;
    };
    w32 +=  ni ;
    bhd += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
//
// store subarray of i64 from i32 block (signed)
void move_i32_to_i64(int64_t * restrict bhd, int32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // signed 32 -> 64
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_i32_to_i64, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ bhd[i] = w32[i]; };
    w32 +=  ni ;
    bhd += lni ;
  }
}
//
// ============ float to/from double ============
//
// w32 [IN/OUT] : 32 bit float block[nj][ni]
// dp  [IN/OUT] : 64 bit double array[nj][lni]
// lni     [IN] : storage length of rows in dp
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
void move_d64_to_f32(float * restrict w32, double * restrict dp, int lni, int ni, int nj, block_properties *bp, int z){
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_d64_to_f32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni ; i++){ w32[i] = dp[i] ; }
    w32 +=  ni ;
    dp  += lni ;
  }
//   if(bp != NULL){ *bp = block_zminmax(w32-nij, nij) ; full_block_properties(bp, array_block_kind(w32)) ; }
  if(bp != NULL){ *bp = get_block_properties(w32-nij, nij) ; }
}
// bp is expected to be NULL, z is expected to be 0 in call
void move_f32_to_d64(double * restrict dp, float * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_f32_to_d64, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni ; i++){ dp[i] = w32[i] ; }
    w32 +=  ni ;
    dp  += lni ;
  }
}
//
// ============ block properties functions ============
//
// get signed and unsigned extrema and number of zero values
// data is treated as integers
// s_in  [IN] : 32 bit data values
// n     [IN] : number of values
// return a block_properties struct
block_properties block_zminmax(void *s_in, int n){
  uint32_t *s = (uint32_t *) s_in ;
  block_properties r ;
  uint32_t tu, mxu, mnu ;
  int32_t  ts, mxs, mns, zro, i ;
  mxu = mnu = (uint32_t)s[0] ;
  mxs = mns = (int32_t) s[0] ;
  zro = 0 ;
  for(i=0 ; i<n ; i++){
    tu = (uint32_t)s[i] ;                 // data as unsigned integers
    ts = (int32_t) s[i] ;                 // data as signed integers
    zro = zro + ((tu == 0) ? 1 : 0) ;     // number of zero values
    mxu = (tu > mxu) ? tu : mxu ;         // unsigned maximum
    mxs = (ts > mxs) ? ts : mxs ;         // signed maximum
    mnu = (tu < mnu) ? tu : mnu ;         // unsigned minimum
    mns = (ts < mns) ? ts : mns ;         // signed minimum
  }
  r.maxu.u = mxu ;
  r.maxs.i = mxs ;
  r.minu.u = mnu ;
  r.mins.i = mns ;
  r.zeros = zro ;
  r.kind = bad_data ;
  return r ;
}

// compute full bp according to data type (see data_kind.h)
// bp    [INOUT] : pointer to block properties struct (min / max / min abs)
// datatype [IN] : data type int_data / uint_data / float_data / raw_data
// similar to block_zminmax, uses a pointer argument instead of returning a struct
void full_block_properties(block_properties *bp, data_kind datatype){
  if(bp == NULL) return ;
//   if(datatype == any_data) datatype = bp->kind ;
  if(datatype == float_data){
    if(bp->maxs.i < 0){           // all numbers are negative
      float max = bp->minu.f ;
      float min = bp->maxu.f ;
      bp->mins.f =  min ;         // most negative value  (minimum value)   FLOAT_MIN_VALUE(BP)
      bp->maxs.f =  max ;         // least negative value  (maximum value)  FLOAT_MAX_VALUE(BP)
      bp->minu.f = -max ;         // smallest absolute value                FLOAT_MIN_ABS(BP)
      bp->maxu.f = -min ;         // largest absolute value                 FLOAT_MAX_ABS(BP)
    }else if(bp->mins.i < 0) {    // negative and non negative numbers
      float max = bp->maxs.f ;    // most positive value
      float min = bp->maxu.f ;    // most negative value
      float mins = bp->mins.f ;   // negative value closest to zero
      float minu = bp->minu.f ;   // positive value closest to zero
      bp->mins.f =  min ;         // largest negative value  (minimum value)        FLOAT_MIN_VALUE(BP)
      bp->maxs.f =  max ;         // largest positive value  (maximum value)        FLOAT_MAX_VALUE(BP)
      bp->minu.f = (minu < (-mins)) ? minu : (-mins) ;  // smallest absolute value  FLOAT_MIN_ABS(BP)
      bp->maxu.f = ((max > (-min)) ? max : (-min) ) ;   // largest absolute value   FLOAT_MAX_ABS(BP)
    }
    bp->kind = float_data ;
  }else if(datatype == int_data){
    bp->kind = int_data ;
    // bp->minu.i will be the smallest value >= 0
    // bp->maxu.i will be the negative value closest to 0 if negative values are present
    // if no negative values are present, bp->maxu.i will be equal to bp->maxs.i
  }else if(datatype == uint_data){
    bp->kind = uint_data ;
    // bp->maxs and bp->mins  are meaningless
  }else{
    bp->kind = bad_data ;
  }
}

// fix bp according to data type (see data_kind.h)
// bp       [IN] : block properties struct (min / max / min abs)
// datatype [IN] : data type int_data / uint_data / float_data / raw_data
// return fixed block properties
block_properties fix_block_properties(block_properties bp, data_kind datatype){
//   if(datatype == any_data) datatype = bp.kind ;
  if(datatype == float_data){
    if(bp.maxs.i < 0){           // all numbers are negative
      float max = bp.minu.f ;
      float min = bp.maxu.f ;
      bp.mins.f =  min ;         // most negative value  (minimum value)   FLOAT_MIN_VALUE(BP)
      bp.maxs.f =  max ;         // least negative value  (maximum value)  FLOAT_MAX_VALUE(BP)
      bp.minu.f = -max ;         // smallest absolute value                FLOAT_MIN_ABS(BP)
      bp.maxu.f = -min ;         // largest absolute value                 FLOAT_MAX_ABS(BP)
    }else if(bp.mins.i < 0) {    // negative and non negative numbers
      float max = bp.maxs.f ;    // most positive value
      float min = bp.maxu.f ;    // most negative value
      float mins = bp.mins.f ;   // negative value closest to zero
      float minu = bp.minu.f ;   // positive value closest to zero
      bp.mins.f =  min ;         // largest negative value  (minimum value)        FLOAT_MIN_VALUE(BP)
      bp.maxs.f =  max ;         // largest positive value  (maximum value)        FLOAT_MAX_VALUE(BP)
      bp.minu.f = (minu < (-mins)) ? minu : (-mins) ;  // smallest absolute value  FLOAT_MIN_ABS(BP)
      bp.maxu.f = ((max > (-min)) ? max : (-min) ) ;   // largest absolute value   FLOAT_MAX_ABS(BP)
    }
    bp.kind = float_data ;
  }else if(datatype == int_data){
    bp.kind = int_data ;
    // bp.minu.i will be the smallest value >= 0
    // bp.maxu.i will be the negative value closest to 0 if negative values are present
    // if no negative values are present, bp.maxu.i will be equal to bp.maxs.i
  }else if(datatype == uint_data){
    bp.kind = uint_data ;
    // bp.maxs and bp.mins  are meaningless
  }else{
    bp.kind = bad_data ;
  }
  return bp ;
}

// print block properties
// bp [IN] : block properties struct (min / max / min abs)
void  print_block_properties(block_properties bp){
  fprintf(stderr, "kind = %-7s", printable_type[bp.kind]) ;
  if(bp.kind != bad_data){
    fprintf(stderr, ", minu = %8.8x, maxu = %8.8x, mins = %8.8x, maxs = %8.8x, zeros = %d",
            bp.minu.u, bp.maxu.u, bp.mins.i, bp.maxs.i, bp.zeros) ;
  }
  fprintf(stderr, "\n") ;
}
//
// ============ transfers to/from arrays <rmn/array_nd.h> ============
//
// starting point in array dim[0].ln0 , dim[1].ln0
// block size dim[0].lnn x dim[1].lnn
// blk must be large enough
// if bp is not NULL, block properties will be computed
// return pointer to extracted block
array_2d * array_to_block(array_2d * restrict a, array_2d * restrict blk, block_properties * restrict bp){
  if(a == NULL || blk == NULL) goto fail ;
  if(a->rank != 2 || blk->ndim < 1 || blk->data == NULL) goto fail ;

  uint32_t npts = a->dim[0].lnn * a->dim[1].lnn ;
  ssize_t size  = a->esize ; size *= npts ;
  if(size > (blk->limit - blk->data)) goto fail ;  // blk is too small

  void *src = subarray_address(a) ;                // source address
  void *dst = blk->data ;                          // destination (block) address
  blk->rank = 1 ;                                  // collapse dimensions to rank 1
  blk->dim[0].gn0 = blk->dim[0].ln0 = 0 ;          // npts values, origin 0
  blk->dim[0].gnn = blk->dim[0].lnn = npts ;
  int lni = a->dim[0].gnn ;                        // row storage length in source
  int ni  = a->dim[0].lnn ;                        // number of values along i
  int nj  = a->dim[1].lnn ;                        // number of values along j
// fprintf(stderr, "array_to_block : lni = %d, ni = %d, nj = %d, type = %d\n", lni, ni, nj, a->type) ;
  switch(a->type){
    case byte_data   :
      bhwd2block(dst, (int8_t *)(src), lni, ni, nj, bp) ;
      break ;
    case ubyte_data  :
      bhwd2block(dst, (uint8_t *)(src), lni, ni, nj, bp) ;
      break ;
    case short_data  :
      bhwd2block(dst, (int16_t *)(src), lni, ni, nj, bp) ;
      break ;
    case ushort_data :
      bhwd2block(dst, (uint16_t *)(src), lni, ni, nj, bp) ;
      break ;
    case int_data    :
      bhwd2block(dst, (int32_t *)(src), lni, ni, nj, bp) ;
      break ;
    case uint_data   :
      bhwd2block(dst, (uint32_t *)(src), lni, ni, nj, bp) ;
      break ;
    case long_data   :
      bhwd2block(dst, (int64_t *)(src), lni, ni, nj, bp) ;
      break ;
    case ulong_data  :
      bhwd2block(dst, (uint64_t *)(src), lni, ni, nj, bp) ;
      break ;
    case float_data  :
      bhwd2block(dst, (float *)(src), lni, ni, nj, bp) ;
      break ;
    case double_data :
      bhwd2block(dst, (double *)(src), lni, ni, nj, bp) ;
      break ;
    case raw_data    :   // treated as unsigned 32 bits
      bhwd2block(dst, (uint32_t *)(src), lni, ni, nj, bp) ;
      break ;
    default :
//       fprintf(stderr, "array_to_block : invalid type = %d\n", a->type) ;
      goto fail ;
  }

  return blk ;

fail :
  return NULL ;  // miserable failure
}
//
// ==========================================================================
//
