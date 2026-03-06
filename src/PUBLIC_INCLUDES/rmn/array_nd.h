//
// Copyright (C) 2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2024-2025
//

#if ! defined(DIM_ZERO)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// data types
#include <rmn/data_kind.h>
// extra cpp macros (VA_ARGS related)
#include <rmn/cpp_extras.h>
// block movers/analyzers
#include <rmn/move_blocks.h>

// dimensionality description along a dimension
// global   dimension index space : gn0 : gn0 + gnn - 1  ( gnn elements)
// subarray dimension index range : ln0 : ln0 + lnn - 1  ( lnn elements)
// constraints : ln0 >= gn0 , ln0 + lnn <= gn0 + gnn
typedef struct{
  int32_t  gnn ;          // number of elements stored along dimension
  int32_t  gn0 ;          // global index of first point along dimension (usually 0 or 1)
  int32_t  lnn ;          // number of elements used along dimension (sub array)
  int32_t  ln0 ;          // index of first point along dimension (usually 0 or 1)
} dim_desc ;              // ln0 == gn0 , lnn == gnn : all elements along this dimension are used

// flags description

// DATA_IS_INTERNAL set means that the array_nd struct contains both data and control information
#define DATA_IS_INTERNAL   1

// DATA_MAY_REALLOC set means that the data pointer may be freed/reallocated
#define DATA_MAY_REALLOC   2

// STRUCT_CAN_FREE means that the struct was malloc(ed) by create_array and can be freed
// if both DATA_MAY_REALLOC and STRUCT_CAN_FREE are set, the data member must be freed first
#define STRUCT_CAN_FREE    4

// prevent freeing even if DATA_MAY_REALLOC or STRUCT_CAN_FREE or DATA_IS_INTERNAL is set
// this flag is intended to be set or cleared by application code
#define DATA_IS_REFERENCED 8

// 32 bit data block, 2 dimensional, used mostly by 8/16/32/64 bit <-> 32 bit bhwd movers
// block will normally contain signed/unsigned integers or floats
typedef struct{
  union{                 // starting address of block
    void     *w32 ;      // generic pointer
    uint32_t *u32 ;      // pointer to unsigned integer
    int32_t  *i32 ;      // pointer to signed integer
    float    *f32 ;      // pointer to float
  } ;
  uint32_t end ;         // pointer to 32 bit word just beyond array
  uint32_t lni ;         // first dimension
  uint32_t lnj ;         // second dimension (1 if block is 1D)
  uint32_t zero :23 ,    // reserved for future use
           type : 4 ,    // block type (see rmn/data_kind.h) (bad/int/uint/float/any)
           flags: 5 ;    // flags and extra information
  uint32_t w[] ;         // start of data if monolithic
} block_2d ;             // 2D block, (end - u32) may me larger than (lni * lnj)

// initialize a block_2d variable to null contents
#define block_2d_null (block_2d){ .u32 = NULL, .end = 0, .lni = 0, .lnj = 0, .zero = 0, .type  = 0, .flags  = 0 }

// create a monolithic local block_2d variable and its pointer
// (variable size, initialized as a 1 dimensional block)
// block : block_2d pointer name
// size  : number of 32 bit data elements that the block may accomodate
// a local array named blockname_alias[] will be created pointed to by (block_2d *) blockname
// the dimension of blockname_alias will be size + size of base block_2d struct
// in order to be able to accomodate up to size data elements
// line 1 : declare local array with the appropriate dimensions
// line 2 : declare pointer, set it to address of local array
// line 3 : initialize *blockname to appropriate values
#define local_block_2d(blockname, size) \
  uint32_t blockname ## _alias[size + sizeof(block_2d)/sizeof(uint32_t)] ; \
  block_2d *blockname = (block_2d *) blockname ## _alias ;  \
  *blockname = (block_2d) { .u32 = blockname->w, .end = size, .lni = size, .lnj = 1, .zero = 0, .type = 0, .flags = 0 } ;

uint32_t dynamic_block_2d(block_2d *bp, uint32_t size);
void print_block_2d(block_2d *bp, char *msg);
block_2d *new_block_2d(void *mem, size_t size, int monolithic);
block_2d *reshape_block_2d(block_2d *bp, uint32_t ni, uint32_t nj);
block_2d *free_block_2d(block_2d *block);
uint32_t mem_block_2d(block_2d *block, void *mem, uint32_t size) ;

// zero dimension
#define DIM_ZERO (dim_desc) {.gnn=0, .gn0 = 0, .lnn=0, .ln0=0 }
// recent compilers seem not to care, some older compilers seem to need the define way
#if defined(__OLD_COMPILER__)
// some older compilers seemed to want it the define way
#error "this code path should not be used"
#define dim_zero DIM_ZERO
#else
// this is not a constant value according to some older compilers
static const dim_desc  dim_zero = DIM_ZERO ;
#endif

// macro argument ARRAY_PTR is a POINTER to an array_nd type structure (array_nd *)
#define HAS_DATA 0xBEBEFADA
#define array_has_data(ARRAY_PTR) ( (ARRAY_PTR)->signature == HAS_DATA )
#define array_set_used(ARRAY_PTR) { (ARRAY_PTR)->signature = HAS_DATA ; }

#define NO_DATA 0xFADABEBE
#define array_no_data(ARRAY_PTR) ( (ARRAY_PTR)->signature == NO_DATA )
#define array_set_empty(ARRAY_PTR) { (ARRAY_PTR)->signature = NO_DATA ; }

#define array_is_signed(ARRAY_PTR) ( ((ARRAY_PTR)->signature == NO_DATA) || ((ARRAY_PTR)->signature == HAS_DATA) )
#define array_signature(ARRAY_PTR) ((ARRAY_PTR)->signature)

// array_xd structures are 64 bit aligned, size is always a multiple of 64 bits
// #define SMALL_ARRAY_STRUCT
#define LARGE_ARRAY_STRUCT
typedef struct{          // generic struct for array with n dimensions
  uint8_t *data ;        // starting address of array (byte pointer)
  uint8_t *limit ;       // pointer to 1 byte beyond array (byte pointer)
  uint32_t signature ;   // MUST be 0xBEBEFADA or 0xFADABEBE
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  // 32 | 16 | 16 | 16 | 16   esize, type, flags, rank, ndim
//   uint32_t esize ;
//   uint16_t  type ;
//   uint16_t  flags ;
//   uint16_t  rank ;
//   uint16_t  ndim ;
  dim_desc dim[] ;       // dimension descriptor (flexible array member)
} array_nd ;

// ndim MUST be 0, rank MUST be 0
typedef struct{          // specific struct for 0D (rank 0) array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  dim_desc dim[0] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_0d ;

// ndim MUST be 1, rank MUST be <= 1
typedef struct{          // specific struct for 1D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  dim_desc dim[1] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_1d ;

// ndim MUST be 2, rank MUST be <= 2
typedef struct{          // specific struct for 2D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  dim_desc dim[2] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_2d ;

// ndim MUST be 3, rank MUST be <= 3
typedef struct{          // specific struct for 3D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  dim_desc dim[3] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_3d ;

// ndim MUST be 4, rank MUST be <= 4
typedef struct{          // specific struct for 4D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  dim_desc dim[4] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_4d ;

// ndim MUST be 5, rank MUST be <= 5
typedef struct{          // specific struct for 5D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
#if defined(SMALL_ARRAY_STRUCT)
  // 16 | 8 | 4 | 4       esize, type, ndim, flags, rank
  uint32_t esize:16 ,    // size of array elements in bytes (1, 2, 4, 8, ..., )
           type : 4 ,    // element type, see rmn/data_kind.h
           ndim : 3 ,    // number of dimensions at creation time
           count: 1 ,    // not used
           flags: 5,     // flags
           rank : 3 ;    // rank (number of used dimensions) (MUST BE <= ndim)
#elif defined(LARGE_ARRAY_STRUCT)
  // 4 | 4 | 4 | 4 | 16 | 64  type, flags, rank, ndim, ref_count, esize
  uint32_t type : 4 ,
           flags: 5 ,
           rank : 3 ,
           ndim : 4 ,
           count:16 ;
  uint64_t esize ;
#endif
  dim_desc dim[5] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_5d ;

// invalid array descriptors (no dimmension initialization)
static const array_nd array_nd_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=0, .count=0 } ;
static const array_0d array_0d_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=0, .count=0 } ;
static const array_1d array_1d_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=1, .count=0 } ;
static const array_2d array_2d_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=2, .count=0 } ;
static const array_3d array_3d_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=3, .count=0 } ;
static const array_4d array_4d_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=4, .count=0 } ;
static const array_5d array_5d_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .rank=0, .ndim=5, .count=0 } ;

// blank array descriptors (almost valid, but with NULL data start and limit pointers, 0 element size, all dimensions 0)
static const array_nd array_nd_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=0, .ndim=0, .flags=0, .count=0 } ;
static const array_0d array_0d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=0, .ndim=0, .flags=0, .count=0 } ;
static const array_1d array_1d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=1, .ndim=1, .flags=0, .count=0,
                                       .dim = {DIM_ZERO} } ;
static const array_2d array_2d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=2, .ndim=2, .flags=0, .count=0,
                                       .dim = {DIM_ZERO, DIM_ZERO} } ;
static const array_3d array_3d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=3, .ndim=3, .flags=0, .count=0,
                                       .dim = {DIM_ZERO, DIM_ZERO, DIM_ZERO} } ;
static const array_4d array_4d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=4, .ndim=4, .flags=0, .count=0,
                                       .dim = {DIM_ZERO, DIM_ZERO, DIM_ZERO, DIM_ZERO} } ;
static const array_5d array_5d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=NO_DATA, .type=any_data, .rank=5, .ndim=5, .flags=0, .count=0,
                                       .dim = {DIM_ZERO, DIM_ZERO, DIM_ZERO, DIM_ZERO, DIM_ZERO} } ;

#if 0
// macro to help initialize a struct of type array_nd
#define ARRAY_ND(DATA,ESIZE,TYPE,NDIM) {.data = DATA, .limit = DATA, .esize = ESIZE, .type = TYPE, .rank = NDIM }

// static const seems to avoid inducing the warning
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-variable"

// blank array descriptors for 1/2/3/4/5 Dimensions
// defaults invalid data type of zero size
static const array_1d array_1d_null = ARRAY_ND(NULL, 0, 0, 1) ;
static const array_2d array_2d_null = ARRAY_ND(NULL, 0, 0, 2) ;
static const array_3d array_3d_null = ARRAY_ND(NULL, 0, 0, 3) ;
static const array_4d array_4d_null = ARRAY_ND(NULL, 0, 0, 4) ;
static const array_5d array_5d_null = ARRAY_ND(NULL, 0, 0, 5) ;
// #pragma GCC diagnostic pop
#endif

// static inline void array_1d_init(array_1d *a, void *data, ssize_t esize, int type){
//   *a = array_1d_null ; a->esize = esize ; a->type = type ; a->data = data ;
// }
// static inline void array_2d_init(array_2d *a, void *data, ssize_t esize, int type){
//   *a = array_2d_null ; a->esize = esize ; a->type = type ; a->data = data ;
// }
// static inline void array_3d_init(array_3d *a, void *data, ssize_t esize, int type){
//   *a = array_3d_null ; a->esize = esize ; a->type = type ; a->data = data ;
// }
// static inline void array_4d_init(array_4d *a, void *data, ssize_t esize, int type){
//   *a = array_4d_null ; a->esize = esize ; a->type = type ; a->data = data ;
// }
// static inline void array_5d_init(array_5d *a, void *data, ssize_t esize, int type){
//   *a = array_5d_null ; a->esize = esize ; a->type = type ; a->data = data ;
// }

typedef struct{   // struct containing 2 integers (array)
  int32_t i32[2] ;
}__i32__2__ ;

typedef struct{   // struct containing up to 5 integers (array)
  int32_t i32[5] ;
}__i32__5__ ;

typedef struct{   // struct containing up to 5 pairs of integers (array)
  int32_t i32[10] ;
}__i32__5x2__ ;

#if ! defined(__INTEL_COMPILER)
#if ! defined(__PGI)
// suppress an aliasing warning from gcc
#pragma GCC diagnostic ignored  "-Wstrict-aliasing"
#endif
#endif

// argument A is an array_nd type structure

// rank of array at allocation time
#define ARRAY_SYNTAX_RANK(A) \
  _Generic((A), \
  array_5d: 5, array_5d *: 5, \
  array_4d: 4, array_4d *: 4, \
  array_3d: 3, array_3d *: 3, \
  array_2d: 2, array_2d *: 2, \
  array_1d: 1, array_1d *: 1, \
  array_0d: 0, array_0d *: 0, \
  array_nd:-1, array_nd *:-1  \
  )

// rank of array at allocation time
#define ARRAY_ALLOC_RANK(A) \
  _Generic((A), \
  array_5d:(*(array_5d *)(&A)).ndim, array_5d *:( **( (array_5d **)(&A) ) ).ndim, \
  array_4d:(*(array_4d *)(&A)).ndim, array_4d *:( **( (array_4d **)(&A) ) ).ndim, \
  array_3d:(*(array_3d *)(&A)).ndim, array_3d *:( **( (array_3d **)(&A) ) ).ndim, \
  array_2d:(*(array_2d *)(&A)).ndim, array_2d *:( **( (array_2d **)(&A) ) ).ndim, \
  array_1d:(*(array_1d *)(&A)).ndim, array_1d *:( **( (array_1d **)(&A) ) ).ndim, \
  array_0d:(*(array_0d *)(&A)).ndim, array_0d *:( **( (array_0d **)(&A) ) ).ndim, \
  array_nd:(*(array_nd *)(&A)).ndim, array_nd *:( **( (array_nd **)(&A) ) ).ndim  \
  )

// effective rank of array ( <= ARRAY_ALLOC_RANK(ARRAY) )
#define ARRAY_RANK(A) _Generic((A), array_nd:(A).rank, array_5d:(A).rank, array_4d:(A).rank, array_3d:(A).rank, array_2d:(A).rank, array_1d:(A).rank )

// bottom of array in memory
#define ARRAY_DATA(A)  _Generic((A), array_nd:(A).data, array_5d:(A).data, array_4d:(A).data, array_3d:(A).data, array_2d:(A).data, array_1d:(A).data )

// top of array in memory + 1
#define ARRAY_LIMIT(A) _Generic((A), array_nd:(A).limit, array_5d:(A).limit, array_4d:(A).limit, array_3d:(A).limit, array_2d:(A).limit, array_1d:(A).limit )

#define reshape_array(ARRAY_PTR, ...) new_array((ARRAY_PTR), (ARRAY_PTR)->data, __VA_ARGS__)

// users should call the generic function new_array rather than new_array_nd
array_nd *new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t ndims, int32_t nlb5, __i32__5__ lb5);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// array_1d a1 ; new_array(a1, mem, esize, type, ni) ;
// array_5d a5 ; new_array(a5, mem, esize, type, ni, nj, nk, nl, nm) ;
#define new_array(ARRAY_PTR, MEM, ESIZE, TYP, ...) \
  _Generic((ARRAY_PTR), \
    array_nd *: new_array_nd((array_nd *)ARRAY_PTR,MEM,ESIZE,TYP,VA_ARGS_NUM(__VA_ARGS__),VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_5d *: new_array_nd((array_nd *)ARRAY_PTR,MEM,ESIZE,TYP,                       5,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_4d *: new_array_nd((array_nd *)ARRAY_PTR,MEM,ESIZE,TYP,                       4,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_3d *: new_array_nd((array_nd *)ARRAY_PTR,MEM,ESIZE,TYP,                       3,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_2d *: new_array_nd((array_nd *)ARRAY_PTR,MEM,ESIZE,TYP,                       2,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_1d *: new_array_nd((array_nd *)ARRAY_PTR,MEM,ESIZE,TYP,                       1,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }})  \
  )

// create a pointer to a n dimensional null array
array_nd *alloc_array_nd(int32_t ndim) ;

// users should call the generic function new_array rather than create_array_nd
array_nd *create_array_nd(uint32_t flags, int32_t esize, int8_t type, int32_t rank, int32_t ndm5, __i32__5__ dm5) ;

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// array_1d *ap1 ; create_array(ap1, esize, type, ni) ;
// array_5d *ap5 ; create_array(ap1, esize, type, ni, nj, nk, nl, nm) ;
#define create_array(ARRAY_PTR, FLAGS, ESIZE, TYP, ...) \
  ARRAY_PTR = _Generic((ARRAY_PTR), \
    array_nd *: (array_nd *) create_array_nd(FLAGS,ESIZE,TYP,VA_ARGS_NUM(__VA_ARGS__),VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_5d *: (array_5d *) create_array_nd(FLAGS,ESIZE,TYP,                       5,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_4d *: (array_4d *) create_array_nd(FLAGS,ESIZE,TYP,                       4,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_3d *: (array_3d *) create_array_nd(FLAGS,ESIZE,TYP,                       3,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_2d *: (array_2d *) create_array_nd(FLAGS,ESIZE,TYP,                       2,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_1d *: (array_1d *) create_array_nd(FLAGS,ESIZE,TYP,                       1,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }})  \
  )

// users should call the generic function array_gbounds rather than array_gbounds_nd
int set_array_gbounds_nd(array_nd *a, int32_t ndims, __i32__5__ lower_bounds);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// set global lower bounds for dimension indexing, dimension is not altered
// array_1d a1 ; array_gbounds(a1, glbi)
// array_5d a5 ; array_gbounds(a5, glbi, glbj, glbk, glbl, glbm)
#define set_array_gbounds(ARRAY_PTR, ...) \
  _Generic((ARRAY_PTR), \
    array_nd *: set_array_gbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_5d *: set_array_gbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_4d *: set_array_gbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_3d *: set_array_gbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_2d *: set_array_gbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_1d *: set_array_gbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } })  \
  )

// users should call the generic function array_lbounds rather than array_lbounds_nd
int set_array_lbounds_nd(array_nd *a, int32_t ndims, __i32__5x2__ bounds);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// set subarray bounds, global bounds are not altered
// array_1d a1 ; array_lbounds(a1, lbi, ubi)
// array_5d a5 ; array_lbounds(a5, lbi, ubi, ......, lbm, ubm)
// a 0, 0 pair is added at the end of the arguments as a validity marker
#define set_array_lbounds(ARRAY_PTR, ...) \
  _Generic((ARRAY_PTR), \
    array_nd *: set_array_lbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_5d *: set_array_lbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_4d *: set_array_lbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_3d *: set_array_lbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_2d *: set_array_lbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_1d *: set_array_lbounds_nd((array_nd *)ARRAY_PTR, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }})  \
  )

// users should call the generic function subarray_bounds rather than subarray_bounds_nd
__i32__2__ subarray_lbounds_nd(array_nd *a, int32_t dim, int32_t ndims);
// TODO: inline version
// check ndims == a->rank, dim < a->rank, dim > 0

#define subarray_lbounds(ARRAY_PTR, DIM) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_lbounds_nd((array_nd *)ARRAY_PTR, DIM, (ARRAY_PTR)->rank), \
    array_5d *: subarray_lbounds_nd((array_nd *)ARRAY_PTR, DIM,                 5), \
    array_4d *: subarray_lbounds_nd((array_nd *)ARRAY_PTR, DIM,                 4), \
    array_3d *: subarray_lbounds_nd((array_nd *)ARRAY_PTR, DIM,                 3), \
    array_2d *: subarray_lbounds_nd((array_nd *)ARRAY_PTR, DIM,                 2), \
    array_1d *: subarray_lbounds_nd((array_nd *)ARRAY_PTR, DIM,                 1)  \
  )

// users should call the generic function subarray_bounds rather than subarray_bounds_nd
__i32__2__ subarray_gbounds_nd(array_nd *a, int32_t dim, int32_t ndims);
// TODO: inline version
// check ndims == a->rank, dim < a->rank, dim > 0

#define subarray_gbounds(ARRAY_PTR, DIM) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_gbounds_nd((array_nd *)ARRAY_PTR, DIM, (ARRAY_PTR)->rank)), \
    array_5d *: subarray_gbounds_nd((array_nd *)ARRAY_PTR, DIM,                  5), \
    array_4d *: subarray_gbounds_nd((array_nd *)ARRAY_PTR, DIM,                  4), \
    array_3d *: subarray_gbounds_nd((array_nd *)ARRAY_PTR, DIM,                  3), \
    array_2d *: subarray_gbounds_nd((array_nd *)ARRAY_PTR, DIM,                  2), \
    array_1d *: subarray_gbounds_nd((array_nd *)ARRAY_PTR, DIM,                  1)  \
  )

// users should call the generic function subarray_get rather than subarray_get_nd
ssize_t subarray_get_nd(array_nd *a, void *address, size_t copy_size, block_properties *bp);

#define subarray_get(ARRAY_PTR, dest_address, dest_size, bp) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_get_nd((array_nd *)ARRAY_PTR,dest_address, dest_size, bp), \
    array_5d *: subarray_get_nd((array_nd *)ARRAY_PTR,dest_address, dest_size, bp), \
    array_4d *: subarray_get_nd((array_nd *)ARRAY_PTR,dest_address, dest_size, bp), \
    array_3d *: subarray_get_nd((array_nd *)ARRAY_PTR,dest_address, dest_size, bp), \
    array_2d *: subarray_get_nd((array_nd *)ARRAY_PTR,dest_address, dest_size, bp), \
    array_1d *: subarray_get_nd((array_nd *)ARRAY_PTR,dest_address, dest_size, bp)  \
  )

// users should call the generic function subarray_set rather than subarray_set_nd
ssize_t subarray_set_nd(array_nd *a, void *address, size_t copy_size);

#define subarray_set(ARRAY_PTR, dest_address, dest_size) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_set_nd((array_nd *)ARRAY_PTR,dest_address, dest_size), \
    array_5d *: subarray_set_nd((array_nd *)ARRAY_PTR,dest_address, dest_size), \
    array_4d *: subarray_set_nd((array_nd *)ARRAY_PTR,dest_address, dest_size), \
    array_3d *: subarray_set_nd((array_nd *)ARRAY_PTR,dest_address, dest_size), \
    array_2d *: subarray_set_nd((array_nd *)ARRAY_PTR,dest_address, dest_size), \
    array_1d *: subarray_set_nd((array_nd *)ARRAY_PTR,dest_address, dest_size)  \
  )

// users should use the macros rather than the xxx_nd function
int invalid_array_nd(array_nd *a);
#define invalid_array(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: invalid_array_nd((array_nd *)ARRAY_PTR), \
    array_5d *: invalid_array_nd((array_nd *)ARRAY_PTR), \
    array_4d *: invalid_array_nd((array_nd *)ARRAY_PTR), \
    array_3d *: invalid_array_nd((array_nd *)ARRAY_PTR), \
    array_2d *: invalid_array_nd((array_nd *)ARRAY_PTR), \
    array_1d *: invalid_array_nd((array_nd *)ARRAY_PTR)  \
    )

int valid_array_nd(array_nd *a);
#define valid_array(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: valid_array_nd((array_nd *)ARRAY_PTR), \
    array_5d *: valid_array_nd((array_nd *)ARRAY_PTR), \
    array_4d *: valid_array_nd((array_nd *)ARRAY_PTR), \
    array_3d *: valid_array_nd((array_nd *)ARRAY_PTR), \
    array_2d *: valid_array_nd((array_nd *)ARRAY_PTR), \
    array_1d *: valid_array_nd((array_nd *)ARRAY_PTR)  \
    )

array_nd *create_subarray(array_nd *a, array_nd *b);

// users should call the generic function subarray_address rather than subarray_address_nd
uint8_t  *subarray_address_nd(array_nd *a);
#define subarray_address(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_address_nd((array_nd *)ARRAY_PTR), \
    array_5d *: subarray_address_nd((array_nd *)ARRAY_PTR), \
    array_4d *: subarray_address_nd((array_nd *)ARRAY_PTR), \
    array_3d *: subarray_address_nd((array_nd *)ARRAY_PTR), \
    array_2d *: subarray_address_nd((array_nd *)ARRAY_PTR), \
    array_1d *: subarray_address_nd((array_nd *)ARRAY_PTR)  \
    )

// users should call the generic function array_address rather than array_address_nd
void  *array_address_nd(array_nd *a);
#define array_address(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: array_address_nd((array_nd *)ARRAY_PTR), \
    array_5d *: array_address_nd((array_nd *)ARRAY_PTR), \
    array_4d *: array_address_nd((array_nd *)ARRAY_PTR), \
    array_3d *: array_address_nd((array_nd *)ARRAY_PTR), \
    array_2d *: array_address_nd((array_nd *)ARRAY_PTR), \
    array_1d *: array_address_nd((array_nd *)ARRAY_PTR)  \
    )

// users should call the generic function subarray_bytes rather than subarray_bytes_nd
size_t subarray_bytes_nd(array_nd *a);
#define subarray_bytes(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_bytes_nd((array_nd *)ARRAY_PTR), \
    array_5d *: subarray_bytes_nd((array_nd *)ARRAY_PTR), \
    array_4d *: subarray_bytes_nd((array_nd *)ARRAY_PTR), \
    array_3d *: subarray_bytes_nd((array_nd *)ARRAY_PTR), \
    array_2d *: subarray_bytes_nd((array_nd *)ARRAY_PTR), \
    array_1d *: subarray_bytes_nd((array_nd *)ARRAY_PTR)  \
    )

// users should call the generic function array_strides rather than subarray_bytes_nd
void array_strides_nd(array_nd *a, __i32__5__ *strides) ;
#define array_strides(ARRAY_PTR, STRIDES) \
  _Generic((ARRAY_PTR), \
    array_nd *: array_strides_nd((array_nd *)ARRAY_PTR, STRIDES), \
    array_5d *: array_strides_nd((array_nd *)ARRAY_PTR, STRIDES), \
    array_4d *: array_strides_nd((array_nd *)ARRAY_PTR, STRIDES), \
    array_3d *: array_strides_nd((array_nd *)ARRAY_PTR, STRIDES), \
    array_2d *: array_strides_nd((array_nd *)ARRAY_PTR, STRIDES), \
    array_1d *: array_strides_nd((array_nd *)ARRAY_PTR, STRIDES)  \
    )

// users should call the generic function array_bytes rather than array_bytes_nd
size_t array_bytes_nd(array_nd *a);
#define array_bytes(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: array_bytes_nd((array_nd *)ARRAY_PTR), \
    array_5d *: array_bytes_nd((array_nd *)ARRAY_PTR), \
    array_4d *: array_bytes_nd((array_nd *)ARRAY_PTR), \
    array_3d *: array_bytes_nd((array_nd *)ARRAY_PTR), \
    array_2d *: array_bytes_nd((array_nd *)ARRAY_PTR), \
    array_1d *: array_bytes_nd((array_nd *)ARRAY_PTR)  \
    )

// users should call the generic function subarray_dimension rather than subarray_dimension_nd
int  subarray_dimension_nd(array_nd *a);
#define subarray_dimension(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: subarray_dimension_nd((array_nd *)ARRAY_PTR), \
    array_5d *: subarray_dimension_nd((array_nd *)ARRAY_PTR), \
    array_4d *: subarray_dimension_nd((array_nd *)ARRAY_PTR), \
    array_3d *: subarray_dimension_nd((array_nd *)ARRAY_PTR), \
    array_2d *: subarray_dimension_nd((array_nd *)ARRAY_PTR), \
    array_1d *: subarray_dimension_nd((array_nd *)ARRAY_PTR)  \
    )

// users should call the generic function array_dimension rather than array_dimension_nd
int       array_dimension_nd(array_nd *a);
#define array_dimension(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: array_dimension_nd((array_nd *)ARRAY_PTR), \
    array_5d *: array_dimension_nd((array_nd *)ARRAY_PTR), \
    array_4d *: array_dimension_nd((array_nd *)ARRAY_PTR), \
    array_3d *: array_dimension_nd((array_nd *)ARRAY_PTR), \
    array_2d *: array_dimension_nd((array_nd *)ARRAY_PTR), \
    array_1d *: array_dimension_nd((array_nd *)ARRAY_PTR)  \
    )

// users should call the generic function array_kind rather than array_kind_nd
static inline char *array_kind_nd(array_nd *a){
  int kind = a->type ;
  if(kind >=0 && kind < 7) return (char *) printable_type[kind] ;
  return "ERROR" ;
}
#define array_kind(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: array_kind_nd((array_nd *)ARRAY_PTR), \
    array_5d *: array_kind_nd((array_nd *)ARRAY_PTR), \
    array_4d *: array_kind_nd((array_nd *)ARRAY_PTR), \
    array_3d *: array_kind_nd((array_nd *)ARRAY_PTR), \
    array_2d *: array_kind_nd((array_nd *)ARRAY_PTR), \
    array_1d *: array_kind_nd((array_nd *)ARRAY_PTR)  \
  )

size_t fix_array_nd(array_nd *a);
#define fix_array(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: fix_array_nd((array_nd *)ARRAY_PTR), \
    array_5d *: fix_array_nd((array_nd *)ARRAY_PTR), \
    array_4d *: fix_array_nd((array_nd *)ARRAY_PTR), \
    array_3d *: fix_array_nd((array_nd *)ARRAY_PTR), \
    array_2d *: fix_array_nd((array_nd *)ARRAY_PTR), \
    array_1d *: fix_array_nd((array_nd *)ARRAY_PTR)  \
  )

// users should call the generic function free_array rather than free_array_nd
int32_t free_array_nd(array_nd *a);
#define free_array(ARRAY_PTR) \
  _Generic((ARRAY_PTR), \
    array_nd *: free_array_nd((array_nd *)ARRAY_PTR), \
    array_5d *: free_array_nd((array_nd *)ARRAY_PTR), \
    array_4d *: free_array_nd((array_nd *)ARRAY_PTR), \
    array_3d *: free_array_nd((array_nd *)ARRAY_PTR), \
    array_2d *: free_array_nd((array_nd *)ARRAY_PTR), \
    array_1d *: free_array_nd((array_nd *)ARRAY_PTR)  \
  )

#define ARRAY_BYTES  sizeof(int8_t)
#define ARRAY_HWORDS sizeof(int16_t)
#define ARRAY_WORDS  sizeof(int32_t)
// users should call the generic function set_array_value rather than set_array_value_nd
size_t set_array_value_nd(array_nd *a, int32_t v, uint32_t vlen);
#define set_array_value(ARRAY_PTR, V, VLEN) \
  _Generic((ARRAY_PTR), \
    array_nd *: set_array_value_nd((array_nd *)ARRAY_PTR, V, VLEN), \
    array_5d *: set_array_value_nd((array_nd *)ARRAY_PTR, V, VLEN), \
    array_4d *: set_array_value_nd((array_nd *)ARRAY_PTR, V, VLEN), \
    array_3d *: set_array_value_nd((array_nd *)ARRAY_PTR, V, VLEN), \
    array_2d *: set_array_value_nd((array_nd *)ARRAY_PTR, V, VLEN), \
    array_1d *: set_array_value_nd((array_nd *)ARRAY_PTR, V, VLEN)  \
  )

size_t array_copy_data_nd(array_nd *src, array_nd *dst);
// users should use copy_array_data rather than array_copy_data_nd
#define copy_array_data(SRC, DST) array_copy_data_nd((array_nd *)SRC, (array_nd *)DST)

#endif
