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

#if ! defined(block2bhwd)

#include <stdint.h>

typedef void (* bhwd_fn)(void *, void *, int, int, int, int) ;

// call block transfer function pointed to by FN
// automatically add zero value at the end of the argument list
// _Generic will check that FN is a pointer to a block transfer function
#define block_fn(FN, ...)  _Generic((FN), \
                           bhwd_fn : (*FN) \
                           ) (__VA_ARGS__, 0)

#define w32_cast(what, to) _Generic((to), \
                           uint8_t  *: (uint32_t *) what , \
                           int8_t   *: (int32_t  *) what , \
                           uint16_t *: (uint32_t *) what , \
                           int16_t  *: (int32_t  *) what , \
                           int32_t  *: (int32_t  *) what , \
                           uint32_t *: (uint32_t *) what , \
                           float    *: (float    *) what , \
                           uint64_t *: (uint32_t *) what , \
                           int64_t  *: (int32_t  *) what , \
                           double   *: (float    *) what   \
                           )

#define block_type(what) _Generic((what), \
                         uint8_t  *:  1, uint8_t    :  1,  \
                         int8_t   *:  2, int8_t     :  2,  \
                         uint16_t *:  3, uint16_t   :  3, \
                         int16_t  *:  4, int16_t    :  4, \
                         int32_t  *:  5, int32_t    :  5, \
                         uint32_t *:  6, uint32_t   :  6, \
                         float    *:  7, float      :  7, \
                         uint64_t *:  8, uint64_t   :  8, \
                         int64_t  *:  9, int64_t    :  9, \
                         double   *: 10, double     : 10, \
                         default   : 0 \
                         )

#define block_type_name(what) _Generic((what), \
                              uint8_t  *: "uint8_t" , uint8_t    : "uint8_t" , \
                              int8_t   *: "int8_t"  , int8_t     : "int8_t"  , \
                              uint16_t *: "uint16_t", uint16_t   : "uint16_t", \
                              int16_t  *: "int16_t" , int16_t    : "int16_t" , \
                              int32_t  *: "int32_t" , int32_t    : "int32_t" , \
                              uint32_t *: "uint32_t", uint32_t   : "uint32_t", \
                              float    *: "float"   , float      : "float"   , \
                              uint64_t *: "uint64_t", uint64_t   : "uint64_t", \
                              int64_t  *: "int64_t" , int64_t    : "int64_t" , \
                              double   *: "double"  , double     : "double"  , \
                              default   : "UNKNOWN" \
                              )

#define from_block_fn(src) _Generic((src), \
                           uint8_t  *: (bhwd_fn)move_u32_to_u8 , uint8_t  : (bhwd_fn)move_u32_to_u8 , \
                           int8_t   *: (bhwd_fn)move_i32_to_i8 , int8_t   : (bhwd_fn)move_i32_to_i8 , \
                           uint16_t *: (bhwd_fn)move_u32_to_u16, uint16_t : (bhwd_fn)move_u32_to_u16, \
                           int16_t  *: (bhwd_fn)move_i32_to_i16, int16_t  : (bhwd_fn)move_i32_to_i16, \
                           int32_t  *: (bhwd_fn)move_blk_to_i32, int32_t  : (bhwd_fn)move_blk_to_i32, \
                           uint32_t *: (bhwd_fn)move_blk_to_u32, uint32_t : (bhwd_fn)move_blk_to_u32, \
                           float    *: (bhwd_fn)move_blk_to_flt, float    : (bhwd_fn)move_blk_to_flt, \
                           uint64_t *: (bhwd_fn)move_u32_to_u64, uint64_t : (bhwd_fn)move_u32_to_u64, \
                           int64_t  *: (bhwd_fn)move_i32_to_i64, int64_t  : (bhwd_fn)move_i32_to_i64, \
                           double   *: (bhwd_fn)move_f32_to_d64, double   : (bhwd_fn)move_f32_to_d64, \
                           default   : NULL \
                           )
#define from_block_name(src) _Generic((src), \
                             uint8_t  *: "move_u32_to_u8" , uint8_t  : "move_u32_to_u8" , \
                             int8_t   *: "move_i32_to_i8" , int8_t   : "move_i32_to_i8" , \
                             uint16_t *: "move_u32_to_u16", uint16_t : "move_u32_to_u16", \
                             int16_t  *: "move_i32_to_i16", int16_t  : "move_i32_to_i16", \
                             int32_t  *: "move_blk_to_i32", int32_t  : "move_blk_to_i32", \
                             uint32_t *: "move_blk_to_u32", uint32_t : "move_blk_to_u32", \
                             float    *: "move_blk_to_flt", float    : "move_blk_to_flt", \
                             uint64_t *: "move_u32_to_u64", uint64_t : "move_u32_to_u64", \
                             int64_t  *: "move_i32_to_i64", int64_t  : "move_i32_to_i64", \
                             double   *: "move_f32_to_d64", double   : "move_f32_to_d64", \
                             default   : "INVALID" \
                             )

#define to_block_fn(src) _Generic((src), \
                         uint8_t  *: (bhwd_fn)move_u8_to_u32 , uint8_t  : (bhwd_fn)move_u8_to_u32 , \
                         int8_t   *: (bhwd_fn)move_i8_to_i32 , int8_t   : (bhwd_fn)move_i8_to_i32 , \
                         uint16_t *: (bhwd_fn)move_u16_to_u32, uint16_t : (bhwd_fn)move_u16_to_u32, \
                         int16_t  *: (bhwd_fn)move_i16_to_i32, int16_t  : (bhwd_fn)move_i16_to_i32, \
                         int32_t  *: (bhwd_fn)move_i32_to_blk, int32_t  : (bhwd_fn)move_i32_to_blk, \
                         uint32_t *: (bhwd_fn)move_u32_to_blk, uint32_t : (bhwd_fn)move_u32_to_blk, \
                         float    *: (bhwd_fn)move_flt_to_blk, float    : (bhwd_fn)move_flt_to_blk, \
                         uint64_t *: (bhwd_fn)move_u64_to_u32, uint64_t : (bhwd_fn)move_u64_to_u32, \
                         int64_t  *: (bhwd_fn)move_i64_to_i32, int64_t  : (bhwd_fn)move_i64_to_i32, \
                         double   *: (bhwd_fn)move_d64_to_f32, double   : (bhwd_fn)move_d64_to_f32, \
                         default   : NULL \
                         )

#define to_block_name(src) _Generic((src), \
                           uint8_t  *: "move_u8_to_u32" , uint8_t  : "move_u8_to_u32",  \
                           int8_t   *: "move_i8_to_i32" , int8_t   : "move_i8_to_i32",  \
                           uint16_t *: "move_u16_to_u32", uint16_t : "move_u16_to_u32", \
                           int16_t  *: "move_i16_to_i32", int16_t  : "move_i16_to_i32", \
                           int32_t  *: "move_i32_to_blk", int32_t  : "move_i32_to_blk", \
                           uint32_t *: "move_u32_to_blk", uint32_t : "move_u32_to_blk", \
                           float    *: "move_flt_to_blk", float    : "move_flt_to_blk", \
                           uint64_t *: "move_u64_to_u32", uint64_t : "move_u64_to_u32", \
                           int64_t  *: "move_i64_to_i32", int64_t  : "move_i64_to_i32", \
                           double   *: "move_d64_to_f32", double   : "move_d64_to_f32", \
                           default   : "INVALID" \
                           )

// extern bhwd_fn bhwd_table[][2] ;
// #define FETCH_BLOCK 0
// #define STORE_BLOCK 1

// array section to block movers
// blocks are contiguous 32 bit arrays

void move_u8_to_u32(uint32_t * restrict w   , uint8_t * restrict bhwd , int lni, int ni, int nj, int z);  // unsigned 8 -> 32
void move_u32_to_u8(uint8_t * restrict bhwd  , uint32_t * restrict w  , int lni, int ni, int nj, int z);  // unsigned 32 -> 8

void move_i8_to_i32(int32_t * restrict w    , int8_t * restrict bhwd  , int lni, int ni, int nj, int z);  // signed 8 -> 32
void move_i32_to_i8(int8_t * restrict bhwd   , int32_t * restrict w   , int lni, int ni, int nj, int z);  // signed 32 -> 8

void move_u16_to_u32(uint32_t * restrict w  , uint16_t * restrict bhwd, int lni, int ni, int nj, int z);  // unsigned 16 -> 32
void move_u32_to_u16(uint16_t * restrict bhwd, uint32_t * restrict w  , int lni, int ni, int nj, int z);  // unsigned 32 -> 16

void move_i16_to_i32(int32_t * restrict w   , int16_t * restrict bhwd , int lni, int ni, int nj, int z);  // signed 16 -> 32
void move_i32_to_i16(int16_t * restrict bhwd , int32_t * restrict w   , int lni, int ni, int nj, int z);  // signed 32 -> 16

void move_u32_to_blk(uint32_t * restrict blk, uint32_t * restrict w32 , int lni, int ni, int nj, int z);  // array 32 -> block 32 (unsigned)
void move_blk_to_u32(uint32_t * restrict w32, uint32_t * restrict blk , int lni, int ni, int nj, int z);  // block 32 -> array 32 (unsigned)

void move_i32_to_blk(int32_t * restrict blk , int32_t * restrict w32  , int lni, int ni, int nj, int z);  // array 32 -> block 32 (signed)
void move_blk_to_i32(int32_t * restrict w32 , int32_t * restrict blk  , int lni, int ni, int nj, int z);  // block 32 -> array 32 (signed)

void move_flt_to_blk(float * restrict blk   , float * restrict w32    , int lni, int ni, int nj, int z);  // array 32 -> block 32 (float)
void move_blk_to_flt(float * restrict w32   , float * restrict blk    , int lni, int ni, int nj, int z);  // block 32 -> array 32 (float)

void move_u64_to_u32(uint32_t * restrict w  , uint64_t * restrict bhwd, int lni, int ni, int nj, int z);  // unsigned 64 -> 32
void move_u32_to_u64(uint64_t * restrict bhwd, uint32_t * restrict w  , int lni, int ni, int nj, int z);  // unsigned 32 -> 64

void move_i64_to_i32(int32_t * restrict w   , int64_t * restrict bhwd , int lni, int ni, int nj, int z);  // signed 64 -> 32
void move_i32_to_i64(int64_t * restrict bhwd , int32_t * restrict w   , int lni, int ni, int nj, int z);  // signed 32 -> 64

void move_d64_to_f32(float * restrict fp    , double * restrict dp    , int lni, int ni, int nj, int z);  // double -> float
void move_f32_to_d64(double * restrict dp   , float * restrict fp     , int lni, int ni, int nj, int z);  // float -> double

#define block2bhwd(dst,...) _Generic((dst), \
                            uint8_t   *: move_u32_to_u8,  \
                            int8_t    *: move_i32_to_i8,  \
                            uint16_t  *: move_u32_to_u16, \
                            int16_t   *: move_i32_to_i16, \
                            int32_t   *: move_blk_to_i32, \
                            uint32_t  *: move_blk_to_u32, \
                            float     *: move_blk_to_flt, \
                            uint64_t  *: move_u32_to_u64, \
                            int64_t   *: move_i32_to_i64, \
                            double    *: move_f32_to_d64  \
                       ) (dst, __VA_ARGS__, 0)

#define bhwd2block(dst,src,...) _Generic((src), \
                                uint8_t   *: move_u8_to_u32,  \
                                int8_t    *: move_i8_to_i32,   \
                                uint16_t  *: move_u16_to_u32, \
                                int16_t   *: move_i16_to_i32, \
                                int32_t   *: move_i32_to_blk, \
                                uint32_t  *: move_u32_to_blk, \
                                float     *: move_flt_to_blk, \
                                uint64_t  *: move_u64_to_u32, \
                                int64_t   *: move_i64_to_i32, \
                                double    *: move_d64_to_f32  \
                                ) (dst, src, __VA_ARGS__, 0)

#endif
