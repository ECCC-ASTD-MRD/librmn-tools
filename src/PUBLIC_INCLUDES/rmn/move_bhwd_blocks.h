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

#include <rmn/data_properties.h>
#include <rmn/array_nd.h>

#define MAX_ARRAY_TYPES 10

// static uint8_t  UINT8_T  ;
// static int8_t   INT8_T   ;
// static uint16_t UINT16_T ;
// static int16_t  INT16_T  ;
// static uint32_t UINT32_T ;
// static int32_t  INT32_T  ;
// static float    FLOAT    ;
// static uint64_t UINT64_T ;
// static int64_t  INT64_T  ;
// static double   DOUBLE   ;

typedef void (* bhwd_fn)(void *, void *, int, int, int, void *, int) ;

extern int element_bytes[MAX_ARRAY_TYPES] ;       // length in bytes of array elements indexed by type code
extern bhwd_fn into_bhwd[MAX_ARRAY_TYPES] ;       // array to block copy functions table indexed by type code
extern bhwd_fn from_bhwd[MAX_ARRAY_TYPES] ;       // block to array copy functions table indexed by type code

block_properties block_zminmax(void *s, int n);
void full_block_properties(block_properties *bp, data_kind datatype);
block_properties fix_block_properties(block_properties bp, data_kind datatype);
void  print_block_properties(block_properties bp);
int set_bhwd_debug(int value);


#define set_block_properties(bp, block) full_block_properties((bp), array_block_kind((block)))

#define get_block_properties(block, n) fix_block_properties(block_zminmax((void *)(block), (n)), array_block_kind((block)))

// call block transfer function pointed to by FN
// automatically add zero value at the end of the argument list
// _Generic will check that FN is a pointer to a block transfer function
// generic into/from block, no properties computed if copy into block
#define copy_block_fn(FN, ...)  _Generic((FN), \
                                bhwd_fn : (*FN) \
                                ) (__VA_ARGS__, NULL, 0)
// copy into block, properties will be computed, block properties pointer mandatory
#define copy_into_block(FN, ...)  _Generic((FN), \
                                  bhwd_fn : (*FN) \
                                  ) (__VA_ARGS__, 0)
// copy from block, NULL inserted for block properties pointer
#define copy_from_block(FN, ...)  _Generic((FN), \
                                  bhwd_fn : (*FN) \
                                  ) (__VA_ARGS__, NULL, 0)

// block should be cast to this type for this array type
#define w32_cast(block, array) _Generic((array), \
                               uint8_t  *: (uint32_t *) block, \
                               int8_t   *: (int32_t  *) block, \
                               uint16_t *: (uint32_t *) block, \
                               int16_t  *: (int32_t  *) block, \
                               uint32_t *: (uint32_t *) block, \
                               int32_t  *: (int32_t  *) block, \
                               float    *: (float    *) block, \
                               uint64_t *: (uint32_t *) block, \
                               int64_t  *: (int32_t  *) block, \
                               double   *: (float    *) block, \
                               default   : (void     *) block  \
                               )

// number of bytes for each array element according to array type
#define element_length(what) _Generic((what), \
                             uint8_t  *: 1, uint8_t    : 1,  \
                             int8_t   *: 1, int8_t     : 1,  \
                             uint16_t *: 2, uint16_t   : 2, \
                             int16_t  *: 2, int16_t    : 2, \
                             uint32_t *: 4, uint32_t   : 4, \
                             int32_t  *: 4, int32_t    : 4, \
                             float    *: 4, float      : 4, \
                             uint64_t *: 8, uint64_t   : 8, \
                             int64_t  *: 8, int64_t    : 8, \
                             double   *: 8, double     : 8, \
                             default   : 0 \
                             )

// type ordinal code associated with array or element
#define array_type_code(what) _Generic((what), \
                              uint8_t  *: 0, uint8_t    : 0, \
                              int8_t   *: 1, int8_t     : 1, \
                              uint16_t *: 2, uint16_t   : 2, \
                              int16_t  *: 3, int16_t    : 3, \
                              uint32_t *: 4, uint32_t   : 4, \
                              int32_t  *: 5, int32_t    : 5, \
                              float    *: 6, float      : 6, \
                              uint64_t *: 7, uint64_t   : 7, \
                              int64_t  *: 8, int64_t    : 8, \
                              double   *: 9, double     : 9, \
                              default   : -1 \
                              )

// type name associated with array or element
#define array_type_name(what) _Generic((what), \
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

// data kind associated to block type (rmn/data_kind.h)
#define array_block_kind(block) _Generic((block), \
                                uint32_t *: uint_data,  \
                                int32_t  *: int_data,   \
                                float    *: float_data, \
                                default   : bad_data \
                                )

// block kind associated to array type (rmn/data_kind.h)
#define block_kind(array) _Generic((array), \
                          uint8_t  *: uint_data,   \
                          int8_t   *: int_data,    \
                          uint16_t *: uint_data,   \
                          int16_t  *: int_data,    \
                          uint32_t *: uint_data,   \
                          int32_t  *: int_data,    \
                          float    *: float_data,  \
                          uint64_t *: ulong_data,  \
                          int64_t  *: long_data,   \
                          double   *: double_data, \
                          default   : bad_data \
                          )

// block kind name associated to array type
#define block_kind_name(array) _Generic((array), \
                               uint8_t  *: "uint_data",  \
                               int8_t   *: "int_data",   \
                               uint16_t *: "uint_data",  \
                               int16_t  *: "int_data",   \
                               uint32_t *: "uint_data",  \
                               int32_t  *: "int_data",   \
                               float    *: "float_data", \
                               uint64_t *: "uint_data",  \
                               int64_t  *: "int_data",   \
                               double   *: "float_data", \
                               default   : "bad_data" \
                               )

// block -> array copy functions associated to array/scalar type
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
                           _Float16 *: (bhwd_fn)move_f32_to_f16, _Float16 : (bhwd_fn)move_f32_to_f16, \
                           default   : NULL \
                           )
// block -> array copy function names associated to array/scalar type
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
                             _Float16 *: "move_f32_to_f16", _Float16 : "move_f32_to_f16", \
                             default   : "INVALID" \
                             )

// block <- array copy functions associated to array/scalar type
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
                         _Float16 *: (bhwd_fn)move_f16_to_f32, _Float16 : (bhwd_fn)move_f16_to_f32, \
                         default   : NULL \
                         )

// block <- array copy function names associated to array/scalar type
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

// array section to block movers
// blocks (blk) are contiguous 32 bit arrays
// bhwd can byte/halfword/word/doubleword depending upon the function
// w32 is a 32 bit word

// if bp is not NULL, block properties are computed when moving from bhwd to a block
// when moving from a block into bhwd, bp is ignored
// the last argument, z, is only used for 32 bit -> 32 bit moves, and should be 0
// its purpose is to prevent compilers from using the library memory mover for short transfers

void move_u8_to_u32(uint32_t * restrict blk , uint8_t * restrict bhwd , int lni, int ni, int nj, block_properties *bp, int z);  // unsigned 8 -> 32
void move_u32_to_u8(uint8_t * restrict bhwd  , uint32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z);  // unsigned 32 -> 8

void move_i8_to_i32(int32_t * restrict blk  , int8_t * restrict bhwd  , int lni, int ni, int nj, block_properties *bp, int z);  // signed 8 -> 32
void move_i32_to_i8(int8_t * restrict bhwd   , int32_t * restrict blk , int lni, int ni, int nj, block_properties *bp, int z);  // signed 32 -> 8

void move_u16_to_u32(uint32_t * restrict blk, uint16_t * restrict bhwd, int lni, int ni, int nj, block_properties *bp, int z);  // unsigned 16 -> 32
void move_u32_to_u16(uint16_t * restrict bhwd, uint32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z);  // unsigned 32 -> 16

void move_i16_to_i32(int32_t * restrict blk , int16_t * restrict bhwd , int lni, int ni, int nj, block_properties *bp, int z);  // signed 16 -> 32
void move_i32_to_i16(int16_t * restrict bhwd , int32_t * restrict blk , int lni, int ni, int nj, block_properties *bp, int z);  // signed 32 -> 16

void move_f16_to_f32(float * restrict f32, _Float16 * restrict f16, int lni, int ni, int nj, block_properties *bp, int z);      // float 16 -> 32
void move_f32_to_f16(_Float16 * restrict f16, float * restrict f32, int lni, int ni, int nj, block_properties *bp, int z);      // float 32 -> 16

void move_u32_to_blk(uint32_t * restrict blk, uint32_t * restrict w32 , int lni, int ni, int nj, block_properties *bp, int z);  // array 32 -> block 32 (unsigned)
void move_blk_to_u32(uint32_t * restrict w32, uint32_t * restrict blk , int lni, int ni, int nj, block_properties *bp, int z);  // block 32 -> array 32 (unsigned)

void move_i32_to_blk(int32_t * restrict blk , int32_t * restrict w32  , int lni, int ni, int nj, block_properties *bp, int z);  // array 32 -> block 32 (signed)
void move_blk_to_i32(int32_t * restrict w32 , int32_t * restrict blk  , int lni, int ni, int nj, block_properties *bp, int z);  // block 32 -> array 32 (signed)

void move_flt_to_blk(float * restrict blk   , float * restrict w32    , int lni, int ni, int nj, block_properties *bp, int z);  // array 32 -> block 32 (float)
void move_blk_to_flt(float * restrict w32   , float * restrict blk    , int lni, int ni, int nj, block_properties *bp, int z);  // block 32 -> array 32 (float)

void move_u64_to_u32(uint32_t * restrict blk, uint64_t * restrict bhwd, int lni, int ni, int nj, block_properties *bp, int z);  // unsigned 64 -> 32
void move_u32_to_u64(uint64_t * restrict bhwd, uint32_t * restrict blk, int lni, int ni, int nj, block_properties *bp, int z);  // unsigned 32 -> 64

void move_i64_to_i32(int32_t * restrict blk , int64_t * restrict bhwd , int lni, int ni, int nj, block_properties *bp, int z);  // signed 64 -> 32
void move_i32_to_i64(int64_t * restrict bhwd , int32_t * restrict blk , int lni, int ni, int nj, block_properties *bp, int z);  // signed 32 -> 64

void move_d64_to_f32(float * restrict fp    , double * restrict dp    , int lni, int ni, int nj, block_properties *bp, int z);  // double -> float
void move_f32_to_d64(double * restrict dp   , float * restrict fp     , int lni, int ni, int nj, block_properties *bp, int z);  // float -> double

block_2d * array_to_block(array_2d * restrict a, block_2d * restrict blk, block_properties * restrict bp);
array_2d *block_to_array(array_2d * restrict a, block_2d * restrict blk);

// bhwd (value) if bhwd is not a pointer, value pointed to if bhwd is a pointer
#define BHWD(bhwd) \
  _Generic((bhwd), \
  uint8_t  *: *bhwd , uint8_t  : bhwd , \
  int8_t   *: *bhwd , int8_t   : bhwd , \
  uint16_t *: *bhwd , uint16_t : bhwd , \
  int16_t  *: *bhwd , int16_t  : bhwd , \
  int32_t  *: *bhwd , int32_t  : bhwd , \
  uint32_t *: *bhwd , uint32_t : bhwd , \
  float    *: *bhwd , float    : bhwd , \
  uint64_t *: *bhwd , uint64_t : bhwd , \
  int64_t  *: *bhwd , int64_t  : bhwd , \
  double   *: *bhwd , double   : bhwd   \
  )

// pointer to bhwd if bhwd is not a pointer, bhwd if it is a pointer
#define PBHWD(bhwd) \
  _Generic((bhwd), \
  uint8_t  *: bhwd , uint8_t  : &bhwd , \
  int8_t   *: bhwd , int8_t   : &bhwd , \
  uint16_t *: bhwd , uint16_t : &bhwd , \
  int16_t  *: bhwd , int16_t  : &bhwd , \
  int32_t  *: bhwd , int32_t  : &bhwd , \
  uint32_t *: bhwd , uint32_t : &bhwd , \
  float    *: bhwd , float    : &bhwd , \
  uint64_t *: bhwd , uint64_t : &bhwd , \
  int64_t  *: bhwd , int64_t  : &bhwd , \
  double   *: bhwd , double   : &bhwd   \
  )

// move from block, properties are irrelevant
#define block2bhwd(dst,...) _Generic((dst), \
                            uint8_t   *: move_u32_to_u8  , \
                            int8_t    *: move_i32_to_i8  , \
                            uint16_t  *: move_u32_to_u16 , \
                            int16_t   *: move_i32_to_i16 , \
                            int32_t   *: move_blk_to_i32 , \
                            uint32_t  *: move_blk_to_u32 , \
                            float     *: move_blk_to_flt , \
                            uint64_t  *: move_u32_to_u64 , \
                            int64_t   *: move_i32_to_i64 , \
                            double    *: move_f32_to_d64 , \
                            _Float16  *: move_f32_to_f16   \
                       ) (dst, __VA_ARGS__, NULL, 0)

// move to block and compute properties
#define bhwd2block(dst,src,...) _Generic((src), \
                                uint8_t   *: move_u8_to_u32 , \
                                int8_t    *: move_i8_to_i32 , \
                                uint16_t  *: move_u16_to_u32, \
                                int16_t   *: move_i16_to_i32, \
                                int32_t   *: move_i32_to_blk, \
                                uint32_t  *: move_u32_to_blk, \
                                float     *: move_flt_to_blk, \
                                uint64_t  *: move_u64_to_u32, \
                                int64_t   *: move_i64_to_i32, \
                                double    *: move_d64_to_f32, \
                                _Float16  *: move_f16_to_f32  \
                                ) (dst, src, __VA_ARGS__, 0)

// move to block without computing properties
#define bhwd2block_nobp(dst,src,...) _Generic((src), \
                                uint8_t   *: move_u8_to_u32 , \
                                int8_t    *: move_i8_to_i32 , \
                                uint16_t  *: move_u16_to_u32, \
                                int16_t   *: move_i16_to_i32, \
                                int32_t   *: move_i32_to_blk, \
                                uint32_t  *: move_u32_to_blk, \
                                float     *: move_flt_to_blk, \
                                uint64_t  *: move_u64_to_u32, \
                                int64_t   *: move_i64_to_i32, \
                                double    *: move_d64_to_f32, \
                                _Float16  *: move_f16_to_f32  \
                                ) (dst, src, __VA_ARGS__, NULL, 0)


#endif
