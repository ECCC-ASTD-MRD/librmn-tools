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
//     M. Valin,   Recherche en Prevision Numerique, 2024
//

#if ! defined(ARRAY_ND)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rmn/data_kind.h>

// global   dimension index space : gn0 -> gn0 + gni - 1  ( gni elements)
// subarray dimension index range : ln0 -> ln0 + lni - 1  ( lni elements)
// ln0 >= gn0 , ln0 + lni - 1 <= gn0 + gni - 1
typedef struct{
  int32_t  gni ;          // number of elements stored along dimension
  int32_t  gn0 ;          // global index of first point along dimension (usually 0)
//   int32_t  g1 ;          // global index of last point along dimension (usually gni - 1)
//   uint32_t stride ;      // distance between adjacent elements along dimension
  int32_t  ln0 ;          // index of first point along dimension ( gn0 -> gn0 + gni - 1 )
  int32_t  lni ;         // number of elements used along dimension ( gni - (ln0 - gn0) - 1 )
} dim_desc ;             // ln0 = 0 , lni = gni : all elements are used

// clang and intel compiler do not seem to care
#if defined(__PGI) || defined(__INTEL_COMPILERx) || defined(__clang__x) || defined(__INTEL_LLVM_COMPILERx)
// initializer element is not constant according to gcc in the following line
// #define dim_null (dim_desc) {.gni=0, .gn0 = 0, .g1 = -1, .stride=0, .ln0=0, .lni=0 }
// #define dim_null (dim_desc) {.gni=0, .gn0 = 0, .stride=0, .ln0=0, .lni=0 }
#define dim_null (dim_desc) {.gni=0, .gn0 = 0, .ln0=0, .lni=0 }
#else
// what follows is not a constant value according to some compilers (PGI for now)
// static const dim_desc  dim_null = {.gni=0, .gn0 = 0, .g1 = -1, .stride=0, .ln0=0, .lni=0 } ;
// static const dim_desc  dim_null = {.gni=0, .gn0 = 0, .stride=0, .ln0=0, .lni=0 } ;
static const dim_desc  dim_null = {.gni=0, .gn0 = 0, .ln0=0, .lni=0 } ;
#endif

typedef struct{          // generic struct for array with n dimensions
  uint8_t *data ;        // starting address of array (byte pointer)
  uint8_t *limit ;       // pointer to 1 byte beyond array (byte pointer)
  uint32_t esize ;       // size of array elements in bytes (1, 2, 4, 8, ..., )
  uint16_t reserved ;
  uint8_t  type ;        // element type, see rmn/move_blocks.h
  uint8_t  ndim ;        // number of dimensions
  dim_desc dim[] ;       // dimension descriptor (flexible array member)
} array_nd ;

typedef struct{          // specific struct for 1D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 1
  dim_desc dim[1] ;
} array_1d ;
static const array_1d array_1d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type=0, .ndim=1,
                                       .dim[0]=dim_null } ;

typedef struct{          // specific struct for 2D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 2
  dim_desc dim[2] ;
} array_2d ;
static const array_2d array_2d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type=0, .ndim=2,
                                       .dim[0]=dim_null, .dim[1]=dim_null } ;

typedef struct{          // specific struct for 3D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 3
  dim_desc dim[3] ;
} array_3d ;
static const array_3d array_3d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type=0, .ndim=3,
                                       .dim[0]=dim_null, .dim[1]=dim_null, .dim[2]=dim_null } ;

typedef struct{          // specific struct for 4D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 4
  dim_desc dim[4] ;
} array_4d ;
static const array_4d array_4d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type=0, .ndim=4,
                                       .dim[0]=dim_null, .dim[1]=dim_null, .dim[2]=dim_null, .dim[3]=dim_null } ;

typedef struct{          // specific struct for 5D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 5
  dim_desc dim[5] ;
} array_5d ;
static const array_5d array_5d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type=0, .ndim=5,
                                       .dim[0]=dim_null, .dim[1]=dim_null, .dim[2]=dim_null, .dim[3]=dim_null, .dim[4]=dim_null } ;

// macro to initialize a struct of type array_nd
#define ARRAY_ND(DATA,ESIZE,TYPE,NDIM,SIZE) {.data = DATA, .limit = NULL, .esize = ESIZE, .type = TYPE, .ndim = NDIM }

// static const seems to avoid inducing the warning
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-variable"

// blank array descriptors for 1/2/3 Dimensions
// defaults to 32 bit unsigned type
static const array_1d array_1d_0 = ARRAY_ND(NULL, sizeof(int32_t), 0, 1, 0) ;
static const array_2d array_2d_0 = ARRAY_ND(NULL, sizeof(int32_t), 0, 2, 0) ;
static const array_3d array_3d_0 = ARRAY_ND(NULL, sizeof(int32_t), 0, 3, 0) ;
static const array_4d array_4d_0 = ARRAY_ND(NULL, sizeof(int32_t), 0, 4, 0) ;
static const array_5d array_5d_0 = ARRAY_ND(NULL, sizeof(int32_t), 0, 5, 0) ;
// #pragma GCC diagnostic pop

static inline void array_1d_init(array_1d *a, void *data, ssize_t esize, int type){ *a = array_1d_null ; }
static inline void array_2d_init(array_2d *a, void *data, ssize_t esize, int type){ *a = array_2d_null ; }
static inline void array_3d_init(array_3d *a, void *data, ssize_t esize, int type){ *a = array_3d_null ; }
static inline void array_4d_init(array_4d *a, void *data, ssize_t esize, int type){ *a = array_4d_null ; }
static inline void array_5d_init(array_5d *a, void *data, ssize_t esize, int type){ *a = array_5d_null ; }

typedef struct{   // struct containing an array of 6 integers (only the first 5 may get used)
  int32_t i32[6] ;
}__i32__5__ ;

// users should call the generic function new_array rather than new_array_nd
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, __i32__5__ dim);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// array_1d a1 ; new_array(a1, mem, esize, type, ni)
// array_5d a5 ; new_array(a5, mem, esize, type, ni, nj, nk, nl, nm)
#define new_array(ARRAY, MEM, ESIZE, TYPE, ...) \
  _Generic((ARRAY), \
    array_1d *: new_array_nd((array_nd *)ARRAY, MEM, ESIZE, TYPE, (__i32__5__) { { __VA_ARGS__ , 0 } }), \
    array_2d *: new_array_nd((array_nd *)ARRAY, MEM, ESIZE, TYPE, (__i32__5__) { { __VA_ARGS__ , 0 } }), \
    array_3d *: new_array_nd((array_nd *)ARRAY, MEM, ESIZE, TYPE, (__i32__5__) { { __VA_ARGS__ , 0 } }), \
    array_4d *: new_array_nd((array_nd *)ARRAY, MEM, ESIZE, TYPE, (__i32__5__) { { __VA_ARGS__ , 0 } }), \
    array_5d *: new_array_nd((array_nd *)ARRAY, MEM, ESIZE, TYPE, (__i32__5__) { { __VA_ARGS__ , 0 } })  \
  )

int32_t invalid_array(array_nd *a);
array_nd *array_block(array_nd *a, array_nd *b);
uint8_t *array_element(array_nd *a);

#endif
