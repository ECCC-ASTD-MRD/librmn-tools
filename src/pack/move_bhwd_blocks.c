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
int element_bytes(int code){
  switch(code){
    case byte_data:
      return 1 ;
    case ubyte_data:
      return 1 ;
    case short_data:
      return 2 ;
    case ushort_data:
      return 2 ;
    case int_data:
      return 4 ;
    case uint_data:
      return 4 ;
    case long_data:
      return 8 ;
    case ulong_data:
      return 8 ;
    case float_data:
      return 4 ;
    case double_data:
      return 8 ;
    case raw_data:
      return 4 ;
    case bf16_data:
      return 2 ;
    case fp16_data:
      return 2 ;
    default:
      return -1 ;
  }
}

// array to block copy functions table indexed by type code
bhwd_fn fn_into_block(int code){
  switch(code){
    case byte_data:
      return (bhwd_fn)move_i8_to_i32 ;
    case ubyte_data:
      return (bhwd_fn)move_u8_to_u32 ;
    case short_data:
      return (bhwd_fn)move_i16_to_i32 ;
    case ushort_data:
      return (bhwd_fn)move_u16_to_u32 ;
    case int_data:
      return (bhwd_fn)move_i32_to_w32 ;
    case uint_data:
      return (bhwd_fn)move_u32_to_w32 ;
    case long_data:
      return (bhwd_fn)move_i64_to_i32 ;
    case ulong_data:
      return (bhwd_fn)move_u64_to_u32 ;
    case float_data:
      return (bhwd_fn)move_f32_to_w32 ;
    case double_data:
      return (bhwd_fn)move_d64_to_f32 ;
    case raw_data:
      return (bhwd_fn)move_u32_to_w32 ;
    case bf16_data:
      return (bhwd_fn)move_b16_to_f32 ;
    case fp16_data:
      return (bhwd_fn)move_f16_to_f32 ;
    default:
      return NULL ;
  }
}

// block to array copy functions table indexed by type code
bhwd_fn fn_from_block(int code){
  switch(code){
    case byte_data:
      return (bhwd_fn)move_i32_to_i8 ;
    case ubyte_data:
      return (bhwd_fn)move_u32_to_u8 ;
    case short_data:
      return (bhwd_fn)move_i32_to_i16 ;
    case ushort_data:
      return (bhwd_fn)move_u32_to_u16 ;
    case int_data:
      return (bhwd_fn)move_w32_to_i32 ;
    case uint_data:
      return (bhwd_fn)move_w32_to_u32 ;
    case long_data:
      return (bhwd_fn)move_i32_to_i64 ;
    case ulong_data:
      return (bhwd_fn)move_u32_to_u64 ;
    case float_data:
      return (bhwd_fn)move_w32_to_f32 ;
    case double_data:
      return (bhwd_fn)move_f32_to_d64 ;
    case raw_data:
      return (bhwd_fn)move_w32_to_u32 ;
    case bf16_data:
      return (bhwd_fn)move_f32_to_b16 ;
    case fp16_data:
      return (bhwd_fn)move_f32_to_f16 ;
    default:
      return NULL ;
  }
}

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
// to_w32 copies from array[nj][lni] to block[nj][ni]
static void to_w32(void * restrict blk_, void * restrict w32_, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  int32_t *blk = (int32_t *)blk_, *w32 = (int32_t *)w32_ ;
  int i ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ blk[i] = w32[i] + z ; };
    blk +=  ni ;
    w32 += lni ;
  }
}
// from_w32 copies from block[nj][ni] to array[nj][lni]
static void from_w32(void * restrict w32_, void * restrict blk_, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  int32_t *blk = (int32_t *)blk_, *w32 = (int32_t *)w32_ ;
  int i ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = blk[i] + z ; };
    blk +=  ni ;
    w32 += lni ;
  }
}
void move_u32_to_w32(uint32_t * restrict blk, uint32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // 32 array -> 32 block
  (void) (z) ; int nij = ni*nj ;
if(DEBUG > 0)  fprintf(stderr, "move_w32_to_u32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  to_w32(blk, w32, lni, ni, nj, z) ;
  if(bp != NULL){ *bp = get_block_properties(blk, nij) ; }
}
void move_w32_to_u32(uint32_t * restrict w32, uint32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z){  // 32 block -> 32 array
  (void) (z) ; (void) (bp) ;
if(DEBUG > 0)  fprintf(stderr, "move_w32_to_u32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  from_w32(w32, blk, lni, ni, nj, z) ;
}
void move_i32_to_w32(int32_t * restrict blk, int32_t * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // 32 array -> 32 block
  (void) (z) ; int nij = ni*nj ;
if(DEBUG > 0)  fprintf(stderr, "move_i32_to_w32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  to_w32(blk, w32, lni, ni, nj, z) ;
  if(bp != NULL){ *bp = get_block_properties(blk, nij) ; }
}
void move_w32_to_i32(int32_t * restrict w32, int32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z){  // 32 block -> 32 array
  (void) (z) ; (void) (bp) ;
if(DEBUG > 0)  fprintf(stderr, "move_w32_to_i32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  from_w32(w32, blk, lni, ni, nj, z) ;
}
void move_f32_to_w32(float * restrict blk, float * restrict w32, int lni, int ni, int nj, block_properties *bp, int z){  // 32 array -> 32 block
  (void) (z) ; int nij = ni*nj ;
if(DEBUG > 0)  fprintf(stderr, "move_f32_to_w32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  to_w32(blk, w32, lni, ni, nj, z) ;
  if(bp != NULL){ *bp = get_block_properties(blk, nij) ; }
}
void move_w32_to_f32(float * restrict w32, float * restrict blk, int lni, int ni, int nj, block_properties *bp, int z){  // 32 block -> 32 array
  (void) (z) ; (void) (bp) ;
if(DEBUG > 0)  fprintf(stderr, "move_w32_to_f32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  from_w32(w32, blk, lni, ni, nj, z) ;
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
// ============ float to/from float 16 ============
//
// get subarray of f16 into f32 block (signed)
void move_f16_to_f32(float * restrict f32, _Float16 * restrict f16, int lni, int ni, int nj, block_properties *bp, int z){  // signed 16 -> 32
  (void) (z) ; int nij = ni*nj ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_f16_to_f32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ f32[i] = f16[i]; };
    f32 +=  ni ;
    f16 += lni ;
  }
  if(bp != NULL){ *bp = get_block_properties(f32-nij, nij) ; }
}
//
// store subarray of f16 from f32 block (signed)
void move_f32_to_f16(_Float16 * restrict f16, float * restrict f32, int lni, int ni, int nj, block_properties *bp, int z){  // signed 32 -> 16
  (void) (z) ; (void) (bp) ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_f32_to_f16, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ f16[i] = f32[i]; };
    f32 +=  ni ;
    f16 += lni ;
  }
}
//
// ============ float to/from brainfloat 16 ============
//
void move_b16_to_f32(float * restrict f32_, void * restrict b16_, int lni, int ni, int nj, block_properties *bp, int z){
  (void) (z) ; (void) (bp) ;
  uint32_t *f32 = (uint32_t *)f32_ ;
  uint16_t *b16 = (uint16_t *)b16_ ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_b16_to_f32, lni = %d, ni = %d, nj = %d, bp = %s\n", lni, ni, nj, bp ? "ON" : "OFF") ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ f32[i] = b16[i] ; f32[i] <<= 16 ; };
    f32 +=  ni ;
    b16 += lni ;
  }
}

void move_f32_to_b16(void * restrict b16_, float * restrict f32_, int lni, int ni, int nj, block_properties *bp, int z){
  (void) (z) ; (void) (bp) ;
  uint32_t *f32 = (uint32_t *)f32_ ;
  uint16_t *b16 = (uint16_t *)b16_ ;
  int i ;
if(DEBUG > 0)  fprintf(stderr, "move_f32_to_b16, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ b16[i] = ((f32[i] + 0x80FFu) >> 16) ; };
    f32 +=  ni ;
    b16 += lni ;
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
    if(bp.kind == float_data){
      fprintf(stderr, ", min = %f, max = %f", bp.mins.f, bp.maxs.f) ;
    }
  }
  fprintf(stderr, "\n") ;
}
//
// ============ transfers to/from arrays <rmn/array_nd.h> ============
//
// starting point in array dim[0].ln0 , dim[1].ln0
// block size dim[0].lnn x dim[1].lnn
// blk must be large enough to accomodate extracted data
// if bp is not NULL, block properties will be computed
// return pointer to extracted block, NULL if error
// in case of error, blk is left untouched
block_2d *array_to_block(array_2d * restrict a, block_2d * restrict blk, block_properties * restrict bp){
  char *msg = "" ;
  uint32_t type ;

  if(a == NULL || blk == NULL) goto fail ;
  if(a->rank != 2 || blk->w32 == NULL) goto fail ;

  void *src = subarray_address(a) ;                // source address
  void *dst = blk->w32 ;                           // destination (block) address
  uint32_t lni = a->dim[0].gnn ;                   // row storage length in source
  uint32_t ni  = a->dim[0].lnn ;                   // number of values along i
  uint32_t nj  = a->dim[1].lnn ;                   // number of values along j
  msg = "block is too small" ;
  if(blk->end < (ni * nj)) goto fail ;             // block is too small
// fprintf(stderr, "array_to_block : lni = %d, ni = %d, nj = %d, type = %d\n", lni, ni, nj, a->type) ;
  switch(a->type){
    case byte_data   :
      bhwd2block(dst, (int8_t *)src, lni, ni, nj, bp)   ; type = int_data ;
      break ;
    case ubyte_data  :
      bhwd2block(dst, (uint8_t *)src, lni, ni, nj, bp)  ; type = uint_data ;
      break ;
    case short_data  :
      bhwd2block(dst, (int16_t *)src, lni, ni, nj, bp)  ; type = int_data ;
      break ;
    case ushort_data :
      bhwd2block(dst, (uint16_t *)src, lni, ni, nj, bp) ; type = uint_data ;
      break ;
    case int_data    :
      bhwd2block(dst, (int32_t *)src, lni, ni, nj, bp)  ; type = int_data ;
      break ;
    case uint_data   :
      bhwd2block(dst, (uint32_t *)src, lni, ni, nj, bp) ; type = uint_data ;
      break ;
    case long_data   :
      bhwd2block(dst, (int64_t *)src, lni, ni, nj, bp)  ; type = int_data ;
      break ;
    case ulong_data  :
      bhwd2block(dst, (uint64_t *)src, lni, ni, nj, bp) ; type = uint_data ;
      break ;
    case float_data  :
      bhwd2block(dst, (float *)src, lni, ni, nj, bp)    ; type = float_data ;
      break ;
    case bf16_data  :
      bhwd2block(dst, (float *)src, lni, ni, nj, bp)    ; type = float_data ;
      break ;
    case fp16_data  :
      bhwd2block(dst, (float *)src, lni, ni, nj, bp)    ; type = float_data ;
      break ;
    case double_data :
      bhwd2block(dst, (double *)src, lni, ni, nj, bp)   ; type = float_data ;
      break ;
    case raw_data    :   // treated as unsigned 32 bits
      bhwd2block(dst, (uint32_t *)src, lni, ni, nj, bp) ; type = uint_data ;
      break ;
    default :
      msg = "invalid type" ;
      if(DEBUG > 0) fprintf(stderr, "array_to_block : invalid type %s (%d)\n", printable_type[a->type], a->type) ;
      goto fail ;
  }
  if(DEBUG > 1)  fprintf(stderr, "array_to_block, ->[%d:%d]\n", ni, nj) ;
  blk->lni   = ni ;
  blk->lnj   = nj ;
  blk->zero  = 0 ;
  blk->type  = type ;
  blk->flags = 0 ;

  return blk ;

fail :
  if(DEBUG > 0) fprintf(stderr, "array_to_block : %s\n", msg) ;
  return NULL ;  // miserable failure
}

array_2d *block_to_array(array_2d * restrict a, block_2d * restrict blk){
  char *msg = "" ;

  if(a == NULL || blk == NULL) goto fail ;
  if(a->rank != 2 || blk->w32 == NULL) goto fail ;

  int32_t ni = blk->lni ;
  int32_t nj = blk->lnj ;
  int32_t lni = a->dim[0].gnn ;
  void *dst = subarray_address(a) ;                // source address
  void *src = blk->w32 ;                           // destination (block) address
  msg = "index range mismatch along i" ;
  if(a->dim[0].lnn != (uint32_t)ni) goto fail ;
  msg = "index range mismatch along j" ;
  if(a->dim[1].lnn != (uint32_t)nj) goto fail ;
  int32_t limit ;
  msg = "index overflow along i" ;
  limit = a->dim[0].gn0 + a->dim[0].gnn ;
  if(a->dim[0].ln0 + ni > limit) goto fail ;  // index overflow along i
  msg = "index overflow along j" ;
  limit = a->dim[1].gn0 + a->dim[1].gnn ;
  if(a->dim[1].ln0 + nj > limit) goto fail ;  // index overflow along j

  msg = "inappropriate block type" ;
  switch(a->type){
    case byte_data   :
      if(blk->type != int_data) goto fail ;
      block2bhwd((int8_t *)dst, src, lni, ni, nj) ;
      break ;
    case ubyte_data  :
      if(blk->type != uint_data) goto fail ;
      block2bhwd((uint8_t *)dst, src, lni, ni, nj) ;
      break ;
    case short_data  :
      if(blk->type != int_data) goto fail ;
      block2bhwd((int16_t *)dst, src, lni, ni, nj) ;
      break ;
    case ushort_data :
      if(blk->type != uint_data) goto fail ;
      block2bhwd((uint16_t *)dst, src, lni, ni, nj) ;
      break ;
    case int_data    :
      if(blk->type != int_data) goto fail ;
      block2bhwd((int32_t *)dst, src, lni, ni, nj) ;
      break ;
    case uint_data   :
      if(blk->type != uint_data) goto fail ;
      block2bhwd((uint32_t *)dst, src, lni, ni, nj) ;
      break ;
    case long_data   :
      if(blk->type != int_data) goto fail ;
      block2bhwd((int64_t *)dst, src, lni, ni, nj) ;
      break ;
    case ulong_data  :
      if(blk->type != uint_data) goto fail ;
      block2bhwd((uint64_t *)dst, src, lni, ni, nj) ;
      break ;
    case float_data  :
      if(blk->type != float_data) goto fail ;
      block2bhwd((float *)dst, src, lni, ni, nj) ;
      break ;
    case bf16_data  :
      if(blk->type != float_data) goto fail ;
      block2bhwd((float *)dst, src, lni, ni, nj) ;
      break ;
    case fp16_data  :
      if(blk->type != float_data) goto fail ;
      block2bhwd((float *)dst, src, lni, ni, nj) ;
      break ;
    case double_data :
      if(blk->type != float_data) goto fail ;
      block2bhwd((double *)dst, src, lni, ni, nj) ;
      break ;
    case raw_data    :   // treated as unsigned 32 bits
      if(blk->type != uint_data) goto fail ;
      block2bhwd((uint32_t *)dst, src, lni, ni, nj) ;
      break ;
    default :
      if(DEBUG > 0) fprintf(stderr, "block_to_array : invalid type %s (%d)\n", printable_type[a->type], a->type) ;
      msg = "invalid type" ;
      goto fail ;
  }
  if(DEBUG > 1) fprintf(stderr, "block_to_array, %-10s, [%d,%d]->[%d:%d,%d:%d] of [%d:%d,%d:%d]\n",
                printable_type[a->type], ni, nj,
                a->dim[0].ln0, a->dim[0].ln0+ni-1,
                a->dim[1].ln0, a->dim[1].ln0+nj-1,
                a->dim[0].gn0, a->dim[0].gnn-1,
                a->dim[1].gn0, a->dim[1].gnn-1 ) ;

  return a ;

fail :
  if(DEBUG > 0) fprintf(stderr, "block_to_array : %s\n", msg) ;
  return NULL ;  // miserable failure
}

// view block as 2 2D array
void array_from_block(array_2d * restrict a, block_2d * restrict blk){
  a->data  = (uint8_t *) blk->w32 ;
  uint32_t size = blk->lni * blk->lnj ;
  a->limit = a->data + size * sizeof(uint32_t) ;
  a->esize = 4 ;
  a->type  = blk->type ;
  a->ndim  = 2 ;
  a->rank  = 2 ;
  a->flags = 0 ;
  a->dim[0].ln0 = a->dim[0].gn0 = 0 ;
  a->dim[0].lnn = a->dim[0].gnn = blk->lni ;
  a->dim[1].ln0 = a->dim[1].gn0 = 0 ;
  a->dim[1].lnn = a->dim[1].gnn = blk->lnj ;
  a->signature = HAS_DATA ;
}

#if 0
// deprecated old code, temporarily kept as reference
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
      bhwd2block(dst, (int8_t *)src, lni, ni, nj, bp) ;
      break ;
    case ubyte_data  :
      bhwd2block(dst, (uint8_t *)src, lni, ni, nj, bp) ;
      break ;
    case short_data  :
      bhwd2block(dst, (int16_t *)src, lni, ni, nj, bp) ;
      break ;
    case ushort_data :
      bhwd2block(dst, (uint16_t *)src, lni, ni, nj, bp) ;
      break ;
    case int_data    :
      bhwd2block(dst, (int32_t *)src, lni, ni, nj, bp) ;
      break ;
    case uint_data   :
      bhwd2block(dst, (uint32_t *)src, lni, ni, nj, bp) ;
      break ;
    case long_data   :
      bhwd2block(dst, (int64_t *)src, lni, ni, nj, bp) ;
      break ;
    case ulong_data  :
      bhwd2block(dst, (uint64_t *)src, lni, ni, nj, bp) ;
      break ;
    case float_data  :
      bhwd2block(dst, (float *)src, lni, ni, nj, bp) ;
      break ;
    case double_data :
      bhwd2block(dst, (double *)src, lni, ni, nj, bp) ;
      break ;
    case raw_data    :   // treated as unsigned 32 bits
      bhwd2block(dst, (uint32_t *)src, lni, ni, nj, bp) ;
      break ;
    default :
//       fprintf(stderr, "array_to_block : invalid type = %d\n", a->type) ;
      goto fail ;
  }

  return blk ;

fail :
  return NULL ;  // miserable failure
}
#endif
//
// ==========================================================================
//

// usage : block_pointer = new_block_2d(mem, size, monolithic)
// if mem == NULL, allocate monolithic block
// if mem is not NULL, 
// if monolithic is true, use mem for block, adjust size to account for sizeof(block_2d)
// if monolithic is false, use mem for data, no need to adjust size
block_2d *new_block_2d(void *mem, size_t size, int monolithic){
  block_2d *result ;
  if(mem == NULL){                      // allocate struct with data area
    result = (block_2d *) malloc(sizeof(block_2d) + size * sizeof(uint32_t *)) ;
    monolithic = 1 ;
  }else{
    if(monolithic){
      result = (block_2d *) mem ;                              // point struct to mem
      size = size - (sizeof(block_2d) / sizeof(uint32_t *)) ;  // adjust size
    }else{
      result = (block_2d *) malloc(sizeof(block_2d)) ;         // allocate struct
    }
  }
  if(result != NULL){
    result->u32 = monolithic ? (&(result->w[0])) : mem ;       // point data to mem if not monolithic
    result->end = size ;
    result->lni = size ;
    result->lnj = 1 ;
    result->zero  = 0 ;
    result->type  = 0 ;
    result->flags = 0 ;
    if(monolithic) result->flags |= STRUCT_CAN_FREE ;
  }
  return result ;
}

// make a 2D block dynamic (malloc/free/resizing for the data area)
// leave block type as it is
uint32_t dynamic_block_2d(block_2d *bp, uint32_t size){
  if(bp == NULL) goto fail ;
  if(bp->u32 != NULL){                    // data pointer already exists
    if(bp->end >= size){                  // the block is large enough
      bp->lni    = size ;                 // set block shape as 1D
      bp->lnj    = 1 ;
      return bp->end ;                    // return existing size
    }
    if(bp->flags & DATA_MAY_REALLOC){        // data reallocation is permittted
      free(bp->u32) ;
      bp->u32 = NULL ;
      bp->flags &= (~DATA_MAY_REALLOC) ;     // cancel flag
    }else{
      goto fail ;                         // OOPS, operation is not possible
    }
  }
  // at this point, bp->u32 is NULL
  bp->u32 = (uint32_t *)malloc((size)*sizeof(uint32_t)) ;
  if(bp->u32 == NULL){          // malloc failed
    *bp = block_2d_null ;       // nullify block
    goto fail ;
  }

  bp->end    = size ;
  bp->lni    = bp->end ;
  bp->lnj    = 1 ;
  bp->flags |= DATA_MAY_REALLOC ;
  return size ;
fail :
  return 0 ;
}

// usage block_pointer_2 = shape_block_2d(block_pointer, size_i, size_j)
// set block shape to ni X nj
// return NULL id error, bp if O.K.
block_2d *reshape_block_2d(block_2d *bp, uint32_t ni, uint32_t nj){
  if(bp != NULL){
    if(ni * nj <= bp->end){
      bp->lni = ni ;
      bp->lnj = nj ;
    }else{
      bp = NULL ;
    }
  }
  return bp ;
}

// point a block_2d variable to a valid memory area, initialized as a 1 dimensional block
// block : block_2d variable
// mem   : memory address
// size  : size in bytes available at memory address
uint32_t mem_block_2d(block_2d *block, void *mem, uint32_t size) {
  if(block == NULL || mem == NULL) return 0 ;
  block->u32   = (uint32_t *)(mem) ;
  block->end   = (size)/sizeof(int32_t) ;
  block->lni   = block->end ;
  block->lnj   = 1 ;
  block->flags = 0 ;
  return size ;
}

// usage : block_pointer = free_block_2d(block_pointer)
block_2d *free_block_2d(block_2d *block){
  if(block != NULL){
    if(block->flags & DATA_MAY_REALLOC){
      free(block->u32) ;    // free data block first in order not to create a memory hole
      block->u32 = NULL ;   // advertize that memory has been freed
    }
    if(block->flags & STRUCT_CAN_FREE){
      free(block) ;         // free the whole block if needed
      block = NULL ;
    }
  }
  return block ;
}

// 2D block representing 2D 32 bit array
// return block_2d struct with data pointing to arrray
// array MUST be contiguous in memory (i storage dimension == i dimension)
block_2d array_as_block(array_2d *a){
  block_2d r = block_2d_null ;

  if(a == NULL)                      goto end ;
  if(a->rank != 2)                   goto end ;
  if(a->esize != 4)                  goto end ;
  if(a->dim[0].gnn != a->dim[0].lnn) goto end ;

  int32_t ni = a->dim[0].lnn ;       // i dimension
  int32_t nj = a->dim[1].lnn ;       // j dimension
  r.w32  = subarray_address(a) ;     // start of data address
  r.end  = ni * nj ;
  r.lni  = ni ;
  r.lnj  = nj ;
  r.type = a->type ;

end :
  return r ;
}

// 2D array representing a 2D block
// return array_2d structure with data poinint to block
array_2d block_as_array(block_2d *blk){
  array_2d r = array_2d_invalid ;

  if(blk == NULL) goto end ;

  r.data  = (uint8_t *)blk->u32 ;
  r.limit = (uint8_t *)(blk->u32 + blk->end) ;
  r.signature = HAS_DATA ;
  r.esize = 4 ;
  r.type  = blk->type ;
  r.ndim  = 2 ;
  r.flags = 2 ;
  r.rank  = 2 ;
  r.dim[0].gn0 = r.dim[0].ln0 = 0 ;
  r.dim[0].gnn = r.dim[0].lnn = blk->lni ;
  r.dim[1].gn0 = r.dim[1].ln0 = 0 ;
  r.dim[1].gnn = r.dim[1].lnn = blk->lnj ;

end :
  return r ;
}

void print_block_2d(block_2d *bp, char *msg){
  fprintf(stderr, "%-10s : struct at %16p, data at %16p, block[%8d:%8d], max = %8d elements",
                msg, (void *)bp, (void *)bp->u32, bp->lni, bp->lnj, bp->end) ;
  fprintf(stderr, ", flags = %s%s\n",
                   (bp->flags & STRUCT_CAN_FREE) ? "STRUCT_CAN_FREE " : "" ,
                   (bp->flags & DATA_MAY_REALLOC)   ? "DATA_MAY_REALLOC"    : "" ) ;
}
