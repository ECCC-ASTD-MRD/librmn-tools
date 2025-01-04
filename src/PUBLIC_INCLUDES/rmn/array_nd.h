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
#include <rmn/va_args_num.h>

// global   dimension index space : gn0 -> gn0 + gnn - 1  ( gnn elements)
// subarray dimension index range : ln0 -> ln0 + lnn - 1  ( lnn elements)
// ln0 >= gn0 , ln0 + lnn - 1 <= gn0 + gnn - 1
typedef struct{
  int32_t  gnn ;          // number of elements stored along dimension
  int32_t  gn0 ;          // global index of first point along dimension (usually 0)
//   int32_t  g1 ;          // global index of last point along dimension (usually gnn - 1)
//   uint32_t stride ;      // distance between adjacent elements along dimension
  int32_t  lnn ;          // number of elements used along dimension ( gnn - (ln0 - gn0) - 1 )
  int32_t  ln0 ;          // index of first point along dimension ( gn0 -> gn0 + gnn - 1 )
} dim_desc ;              // ln0 = 0 , lnn = gnn : all elements are used

// recent LLVM based compilers seem not to care, older compilers seem to need the define way
// PGI compilers seem to need the define way
// gcc seems to prefer the const way (emits a warning with the define way)
#if defined(__PGI) || defined(__INTEL_COMPILER) || defined(__clang__) || defined(__INTEL_LLVM_COMPILERx)
// initializer element is not constant according to gcc in the following line
// #define dim_null (dim_desc) {.gnn=0, .gn0 = 0, .g1 = -1, .stride=0, .ln0=0, .lnn=0 }
// #define dim_null (dim_desc) {.gnn=0, .gn0 = 0, .stride=0, .ln0=0, .lnn=0 }
#define dim_null (dim_desc) {.gnn=0, .gn0 = 0, .ln0=0, .lnn=0 }
#else
// what follows is not a constant value according to some compilers
// static const dim_desc  dim_null = {.gnn=0, .gn0 = 0, .g1 = -1, .stride=0, .ln0=0, .lnn=0 } ;
// static const dim_desc  dim_null = {.gnn=0, .gn0 = 0, .stride=0, .ln0=0, .lnn=0 } ;
static const dim_desc  dim_null = {.gnn=0, .gn0 = 0, .ln0=0, .lnn=0 } ;
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

static inline void array_1d_init(array_1d *a, void *data, ssize_t esize, int type){
  *a = array_1d_null ; a->esize = esize ; a->type = type ; a->data = data ;
}
static inline void array_2d_init(array_2d *a, void *data, ssize_t esize, int type){
  *a = array_2d_null ; a->esize = esize ; a->type = type ; a->data = data ;
}
static inline void array_3d_init(array_3d *a, void *data, ssize_t esize, int type){
  *a = array_3d_null ; a->esize = esize ; a->type = type ; a->data = data ;
}
static inline void array_4d_init(array_4d *a, void *data, ssize_t esize, int type){
  *a = array_4d_null ; a->esize = esize ; a->type = type ; a->data = data ;
}
static inline void array_5d_init(array_5d *a, void *data, ssize_t esize, int type){
  *a = array_5d_null ; a->esize = esize ; a->type = type ; a->data = data ;
}

typedef struct{   // struct containing an array of 2 integers
  int32_t i32[2] ;
}__i32__2__ ;

typedef struct{   // struct containing an array of 5 integers
  int32_t i32[5] ;
}__i32__5__ ;

typedef struct{   // struct containing 6 pairs of integers (only the first 5 are used)
  int32_t i32[10] ;
}__i32__5x2__ ;

// static __i32__5x2__ __i32__5x2__null = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 } ;
// static __i32__5x2__ __i32__5x2__null = {{ {{0, 1}} , {{0, 1}}, {{0, 1}}, {{0, 1}}, {{0, 1}} }} ;

// this will work if all arguments are integers and there is at least one argument
#define NuMvArG(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

// users should call the generic function new_array rather than new_array_nd
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t ndims, int32_t nlb5, __i32__5__ lb5);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// array_1d a1 ; new_array(a1, mem, esize, type, ni)
// array_5d a5 ; new_array(a5, mem, esize, type, ni, nj, nk, nl, nm)
#define new_array(ARRAY, MEM, ESIZE, TYP, ...) \
  _Generic((ARRAY), \
    array_1d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,1,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_2d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,2,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_3d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,3,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_4d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,4,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_5d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,5,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }})  \
  )

// users should call the generic function array_gbounds rather than array_gbounds_nd
int set_array_gbounds_nd(array_nd *a, int32_t ndims, __i32__5__ lower_bounds);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// set global lower bounds for dimension indexing, dimension is not altered
// array_1d a1 ; array_gbounds(a1, glbi)
// array_5d a5 ; array_gbounds(a5, glbi, glbj, glbk, glbl, glbm)
#define set_array_gbounds(ARRAY, ...) \
  _Generic((ARRAY), \
    array_1d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_2d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_3d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_4d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_5d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } })  \
  )

// users should call the generic function array_lbounds rather than array_lbounds_nd
int set_array_lbounds_nd(array_nd *a, int32_t ndims, __i32__5x2__ bounds);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// set subarray bounds, global bounds are not altered
// array_1d a1 ; array_lbounds(a1, lbi, ubi)
// array_5d a5 ; array_lbounds(a5, lbi, ubi, ......, lbm, ubm)
// a 0, 0 pair is added at the end of the arguments as a validity marker
#define set_array_lbounds(ARRAY, ...) \
  _Generic((ARRAY), \
    array_1d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_2d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_3d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_4d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_5d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }})  \
  )

// users should call the generic function subarray_bounds rather than subarray_bounds_nd
__i32__2__ subarray_lbounds_nd(array_nd *a, int32_t dim, int32_t ndims);
// TODO: inline version
// check ndims == a->ndim, dim < a->ndim, dim > 0

#define subarray_lbounds(ARRAY, DIM) \
  _Generic((ARRAY), \
    array_1d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 1), \
    array_2d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 2), \
    array_3d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 3), \
    array_4d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 4), \
    array_5d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 5)  \
  )

// users should call the generic function subarray_bounds rather than subarray_bounds_nd
__i32__2__ subarray_gbounds_nd(array_nd *a, int32_t dim, int32_t ndims);
// TODO: inline version
// check ndims == a->ndim, dim < a->ndim, dim > 0

#define subarray_gbounds(ARRAY, DIM) \
  _Generic((ARRAY), \
    array_1d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 1), \
    array_2d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 2), \
    array_3d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 3), \
    array_4d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 4), \
    array_5d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 5)  \
  )

int32_t invalid_array(array_nd *a);
array_nd *array_block(array_nd *a, array_nd *b);
uint8_t *subarray_address(array_nd *a);
int subarray_size(array_nd *a);
size_t subarray_copy(array_nd *a, void *copy_address, size_t copy_size);

#endif
