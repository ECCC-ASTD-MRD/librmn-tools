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
#include <rmn/move_blocks.h>

#include <rmn/identify_fc_compiler.h>
#if defined(COMPILER_IS_GCC)
#pragma GCC optimize "tree-vectorize"
#endif

// bhwd_fn bhwd_table[][2] = {
//   { NULL                    ,  NULL                   }  ,  // INVALID
//   {(bhwd_fn) move_u8_to_u32  , (bhwd_fn) move_u32_to_u8 }  ,  // 8 bit items
//   {(bhwd_fn) move_i8_to_i32  , (bhwd_fn) move_i32_to_i8 }  ,
//   {(bhwd_fn) move_u16_to_u32 , (bhwd_fn) move_u32_to_u16 } ,  // 16 bit items
//   {(bhwd_fn) move_i16_to_i32 , (bhwd_fn) move_i32_to_i16 } ,
//   {(bhwd_fn) move_u32_to_blk , (bhwd_fn) move_blk_to_u32 } ,  // 32 bit items
//   {(bhwd_fn) move_i32_to_blk , (bhwd_fn) move_blk_to_i32 } ,
//   {(bhwd_fn) move_flt_to_blk , (bhwd_fn) move_blk_to_flt } ,
//   {(bhwd_fn) move_u64_to_u32 , (bhwd_fn) move_u32_to_u64 } ,  // 64 bit items
//   {(bhwd_fn) move_i64_to_i32 , (bhwd_fn) move_i32_to_i64 } ,
//   {(bhwd_fn) move_d64_to_f32 , (bhwd_fn) move_f32_to_d64 }
// } ;

//
// ============ 8 bits to/from 32 bits ============
//
// w   [IN/OUT] : 32 bit integer block[nj][ni]
// bhd [IN/OUT] : 8 bit integer array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
// get subarray of u8 into u32 block (unsigned)
void move_u8_to_u32(uint32_t * restrict w, uint8_t * restrict bhd, int lni, int ni, int nj, int z){  // unsigned 8 -> 32
  (void) (z) ;
  int i ;
fprintf(stderr, "move_u8_to_u32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w[i] = bhd[i]; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// store subarray of u8 from u32 block (unsigned)
void move_u32_to_u8(uint8_t * restrict bhd, uint32_t * restrict w, int lni, int ni, int nj, int z){  // unsigned 32 -> 8
  (void) (z) ;
  int i, iter = 0 ;
fprintf(stderr, "move_u32_to_u8, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    iter++ ;
    for(i=0 ; i<ni; i++){ bhd[i] = (w[i] > UINT8_MAX) ? UINT8_MAX : w[i] ; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// get subarray of i8 into i32 block (signed)
void move_i8_to_i32(int32_t * restrict w, int8_t * restrict bhd, int lni, int ni, int nj, int z){  // signed 8 -> 32
  (void) (z) ;
  int i ;
fprintf(stderr, "move_i8_to_i32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w[i] = bhd[i]; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// store subarray of u8 from i32 block (signed)
void move_i32_to_i8(int8_t * restrict bhd, int32_t * restrict w, int lni, int ni, int nj, int z){  // signed 32 -> 8
  (void) (z) ;
  int i ;
fprintf(stderr, "move_i32_to_i8, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ int32_t t = w[i] ; t = (t>INT8_MAX) ? INT8_MAX : t ; t = (t < INT8_MIN) ? INT8_MIN : t ; bhd[i] =t; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// ============ 16 bits to/from 32 bits ============
//
// w   [IN/OUT] : 32 bit integer block[nj][ni]
// bhd [IN/OUT] : 16 bit integer array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
// get subarray of u16 into u32 block (unsigned)
void move_u16_to_u32(uint32_t * restrict w, uint16_t * restrict bhd, int lni, int ni, int nj, int z){  // unsigned 16 -> 32
  (void) (z) ;
  int i ;
fprintf(stderr, "move_u16_to_u32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w[i] = bhd[i]; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// store subarray of u16 from u32 block (unsigned)
void move_u32_to_u16(uint16_t * restrict bhd, uint32_t * restrict w, int lni, int ni, int nj, int z){  // unsigned 32 -> 16
  (void) (z) ;
  int i ;
fprintf(stderr, "move_u32_to_u16, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ bhd[i] = (w[i] > UINT16_MAX) ? UINT16_MAX : w[i] ; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// get subarray of i16 into i32 block (signed)
void move_i16_to_i32(int32_t * restrict w, int16_t * restrict bhd, int lni, int ni, int nj, int z){  // signed 16 -> 32
  (void) (z) ;
  int i ;
fprintf(stderr, "move_i16_to_i32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w[i] = bhd[i]; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// store subarray of i16 from i32 block (signed)
void move_i32_to_i16(int16_t * restrict bhd, int32_t * restrict w, int lni, int ni, int nj, int z){  // signed 32 -> 16
  (void) (z) ;
  int i ;
fprintf(stderr, "move_i32_to_i16, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ int32_t t = w[i] ; t = (t>INT16_MAX) ? INT16_MAX : t ; t = (t < INT16_MIN) ? INT16_MIN : t ; bhd[i] =t; };
    w   +=  ni ;
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
static void to_w32(void * restrict blk_, void * restrict w32_, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  int32_t *blk = (int32_t *)blk_, *w32 = (int32_t *)w32_ ;
  int i ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w32[i] = blk[i] + z ; };
    blk +=  ni ;
    w32 += lni ;
  }
}
void move_i32_to_blk(int32_t * restrict blk, int32_t * restrict w32, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  (void) (z) ;
fprintf(stderr, "move_i32_to_blk, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_blk(blk, w32, lni, ni, nj, z) ;
}
void move_blk_to_i32(int32_t * restrict w32, int32_t * restrict blk, int lni, int ni, int nj, int z){  // 32 block -> 32 array
  (void) (z) ;
fprintf(stderr, "move_blk_to_i32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_w32(w32, blk, lni, ni, nj, z) ;
}
void move_u32_to_blk(uint32_t * restrict blk, uint32_t * restrict w32, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  (void) (z) ;
fprintf(stderr, "move_u32_to_blk, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_blk(blk, w32, lni, ni, nj, z) ;
}
void move_blk_to_u32(uint32_t * restrict w32, uint32_t * restrict blk, int lni, int ni, int nj, int z){  // 32 block -> 32 array
  (void) (z) ;
fprintf(stderr, "move_blk_to_u32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_w32(w32, blk, lni, ni, nj, z) ;
}
void move_flt_to_blk(float * restrict blk, float * restrict w32, int lni, int ni, int nj, int z){  // 32 array -> 32 block
  (void) (z) ;
fprintf(stderr, "move_flt_to_blk, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_blk(blk, w32, lni, ni, nj, z) ;
}
void move_blk_to_flt(float * restrict w32, float * restrict blk, int lni, int ni, int nj, int z){  // 32 block -> 32 array
  (void) (z) ;
fprintf(stderr, "move_blk_to_flt, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  to_w32(w32, blk, lni, ni, nj, z) ;
}
//
// ============ 64 bits to/from 32 bits ============
//
// w   [IN/OUT] : 32 bit integer block[nj][ni]
// bhd [IN/OUT] : 64 bit integer array[nj][lni]
// lni     [IN] : storage length of rows in bhd
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
// get subarray of u64 into u32 block (unsigned)
void move_u64_to_u32(uint32_t * restrict w, uint64_t * restrict bhd, int lni, int ni, int nj, int z){  // unsigned 64 -> 32
  (void) (z) ;
  int i ;
fprintf(stderr, "move_u64_to_u32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ w[i] = (bhd[i] > UINT32_MAX) ? UINT32_MAX : bhd[i] ; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// store subarray of u64 from u32 block (signed)
void move_u32_to_u64(uint64_t * restrict bhd, uint32_t * restrict w, int lni, int ni, int nj, int z){  // unsigned 32 -> 64
  (void) (z) ;
  int i ;
fprintf(stderr, "move_u32_to_u64, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ bhd[i] = w[i]; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// get subarray of i64 into i32 block (unsigned)
void move_i64_to_i32(int32_t * restrict w, int64_t * restrict bhd, int lni, int ni, int nj, int z){  // signed 64 -> 32
  (void) (z) ;
  int i ;
fprintf(stderr, "move_i64_to_i32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ int64_t t = bhd[i] ; t = (t>INT32_MAX) ? INT32_MAX : t ; t = (t < INT32_MIN) ? INT32_MIN : t ; w[i] =t; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// store subarray of i64 from i32 block (signed)
void move_i32_to_i64(int64_t * restrict bhd, int32_t * restrict w, int lni, int ni, int nj, int z){  // signed 32 -> 64
  (void) (z) ;
  int i ;
fprintf(stderr, "move_i32_to_i64, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni; i++){ bhd[i] = w[i]; };
    w   +=  ni ;
    bhd += lni ;
  }
}
//
// ============ float to/from double ============
//
// fp  [IN/OUT] : 32 bit float block[nj][ni]
// dp  [IN/OUT] : 64 bit double array[nj][lni]
// lni     [IN] : storage length of rows in dp
// ni      [IN] : row length
// nj      [IN] : number of rows
// z       [IN] : integer value, not used
//
void move_d64_to_f32(float * restrict fp, double * restrict dp, int lni, int ni, int nj, int z){
  (void) (z) ;
  int i ;
fprintf(stderr, "move_d64_to_f32, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni ; i++){ fp[i] = dp[i] ; }
    fp +=  ni ;
    dp += lni ;
  }
}
//
void move_f32_to_d64(double * restrict dp, float * restrict fp, int lni, int ni, int nj, int z){
  (void) (z) ;
  int i ;
fprintf(stderr, "move_f32_to_d64, lni = %d, ni = %d, nj = %d\n", lni, ni, nj) ;
  while(nj-- > 0){
    for(i=0 ; i<ni ; i++){ dp[i] = fp[i] ; }
    fp +=  ni ;
    dp += lni ;
  }
}
//
// ==========================================================================
