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
#include <string.h>

#include <rmn/data_kind.h>
// extra cpp macros (VA_ARGS related)
#include <rmn/cpp_extras.h>

// dimensionality description along a dimension
// global   dimension index space : gn0 -> gn0 + gnn - 1  ( gnn elements)
// subarray dimension index range : ln0 -> ln0 + lnn - 1  ( lnn elements)
// ln0 >= gn0 , ln0 + lnn - 1 <= gn0 + gnn - 1
typedef struct{
  int32_t  snn ;          // initial allocated dimension (at creation time)
  int32_t  gnn ;          // number of elements stored along dimension
  int32_t  gn0 ;          // global index of first point along dimension (usually 0 or 1)
  int32_t  lnn ;          // number of elements used along dimension (sub array)
  int32_t  ln0 ;          // index of first point along dimension (usually 0 or 1)
} dim_desc ;              // ln0 == gn0 , lnn == gnn : all elements along this dimension are used

// recent compilers seem not to care, some older compilers seem to need the define way
#if defined(__PGI)
// PGI compilers seem to need the define way
#define dim_null (dim_desc) {.gnn=0, .gn0 = 0, .ln0=0, .lnn=0 }
#else
// this is not a constant value according to some compilers
static const dim_desc  dim_null = (dim_desc) {.gnn=0, .gn0 = 0, .ln0=0, .lnn=0 } ;
#endif
#define DIM_NULL (dim_desc) {.gnn=0, .gn0 = 0, .ln0=0, .lnn=0 }

#define HAS_DATA 0xBEBEFADA
#define array_has_data(ARRAY) ( (ARRAY)->signature == HAS_DATA )
#define array_set_used(ARRAY) { (ARRAY)->signature = HAS_DATA ; }

#define IS_EMPTY 0xFADABEBE
#define array_is_empty(ARRAY) ( (ARRAY)->signature == IS_EMPTY )
#define array_set_empty(ARRAY) { (ARRAY)->signature = IS_EMPTY ; }

#define array_is_signed(ARRAY) ( ((ARRAY)->signature == IS_EMPTY) || ((ARRAY)->signature == HAS_DATA) )
#define array_signature(ARRAY) ((ARRAY)->signature)

// DATA_IS_INTERNAL set means that the array_nd struct contains both data and control information
#define DATA_IS_INTERNAL  1
// DATA_MAY_REALLOC set means that the data pointer may be freed/reallocated
#define DATA_MAY_REALLOC  2
// STRUCT_CAN_FREE means that the struct was malloced by create_array and can be freed
#define STRUCT_CAN_FREE   4

typedef struct{          // generic struct for array with n dimensions
  uint8_t *data ;        // starting address of array (byte pointer)
  uint8_t *limit ;       // pointer to 1 byte beyond array (byte pointer)
  uint32_t signature ;   // MUST be 0xBEBEFADA
  uint16_t esize ;       // size of array elements in bytes (1, 2, 4, 8, ..., )
  uint8_t  type ;        // element type, see rmn/data_kind.h
  uint8_t  flags:4,      // flags
           ndim:4 ;      // rank (number of dimensions)
  dim_desc dim[] ;       // dimension descriptor (flexible array member)
} array_nd ;

typedef struct{          // specific struct for 1D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
  uint16_t esize ;
  uint8_t  type ;
  uint8_t  flags:4,
           ndim:4 ;      // ndim MUST be 1
  dim_desc dim[1] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_1d ;

typedef struct{          // specific struct for 2D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
  uint16_t esize ;
  uint8_t  type ;
  uint8_t  flags:4,
           ndim:4 ;      // ndim MUST be 2
  dim_desc dim[2] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_2d ;

typedef struct{          // specific struct for 3D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
  uint16_t esize ;
  uint8_t  type ;
  uint8_t  flags:4,
           ndim:4 ;      // ndim MUST be 3
  dim_desc dim[3] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_3d ;

typedef struct{          // specific struct for 4D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
  uint16_t esize ;
  uint8_t  type ;
  uint8_t  flags:4,
           ndim:4 ;      // ndim MUST be 4
  dim_desc dim[4] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_4d ;

typedef struct{          // specific struct for 5D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t signature ;
  uint16_t esize ;
  uint8_t  type ;
  uint8_t  flags:4,
           ndim:4 ;      // ndim MUST be 5
  dim_desc dim[5] ;
  uint32_t w32[] ;       // usable only if created with create_array
} array_5d ;

// blank array descriptors (invalid type, element size = 0, no data)
static const array_nd array_nd_invalid = {.data=NULL, .limit=NULL, .signature=0, .esize=0, .type=0, .flags=0, .ndim=0 } ;

static const array_1d array_1d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=IS_EMPTY, .type=0, .ndim=1, .flags=0,
                                       .dim = {DIM_NULL} } ;
static const array_2d array_2d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=IS_EMPTY, .type=0, .ndim=2, .flags=0,
                                       .dim = {DIM_NULL, DIM_NULL} } ;
static const array_3d array_3d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=IS_EMPTY, .type=0, .ndim=3, .flags=0,
                                       .dim = {DIM_NULL, DIM_NULL, DIM_NULL} } ;
static const array_4d array_4d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=IS_EMPTY, .type=0, .ndim=4, .flags=0,
                                       .dim = {DIM_NULL, DIM_NULL, DIM_NULL, DIM_NULL} } ;
static const array_5d array_5d_null = {.data=NULL, .limit=NULL, .esize=0, .signature=IS_EMPTY, .type=0, .ndim=5, .flags=0,
                                       .dim = {DIM_NULL, DIM_NULL, DIM_NULL, DIM_NULL, DIM_NULL} } ;

#if 0
// macro to help initialize a struct of type array_nd
#define ARRAY_ND(DATA,ESIZE,TYPE,NDIM) {.data = DATA, .limit = DATA, .esize = ESIZE, .type = TYPE, .ndim = NDIM }

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

typedef struct{   // struct containing 2 integers (array)
  int32_t i32[2] ;
}__i32__2__ ;

typedef struct{   // struct containing up to 5 integers (array)
  int32_t i32[5] ;
}__i32__5__ ;

typedef struct{   // struct containing up to 5 pairs of integers (array)
  int32_t i32[10] ;
}__i32__5x2__ ;

#define reshape_array(ARRAY, ...) new_array((ARRAY), (ARRAY)->data, __VA_ARGS__)

// users should call the generic function new_array rather than new_array_nd
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t ndims, int32_t nlb5, __i32__5__ lb5);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// array_1d a1 ; new_array(a1, mem, esize, type, ni) ;
// array_5d a5 ; new_array(a5, mem, esize, type, ni, nj, nk, nl, nm) ;
#define new_array(ARRAY, MEM, ESIZE, TYP, ...) \
  _Generic((ARRAY), \
    array_nd *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,VA_ARGS_NUM(__VA_ARGS__),VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_5d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,5,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_4d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,4,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_3d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,3,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_2d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,2,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_1d *: new_array_nd((array_nd *)ARRAY,MEM,ESIZE,TYP,1,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }})  \
  )

// users should call the generic function new_array rather than create_array_nd
array_nd *create_array_nd(uint32_t flags, int32_t esize, int8_t type, int32_t ndim, int32_t ndm5, __i32__5__ dm5) ;

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// array_1d *ap1 ; create_array(ap1, esize, type, ni) ;
// array_5d *ap5 ; create_array(ap1, esize, type, ni, nj, nk, nl, nm) ;
#define create_array(ARRAY, FLAGS, ESIZE, TYP, ...) \
  ARRAY = _Generic((ARRAY), \
    array_nd *: (array_nd *) create_array_nd(FLAGS,ESIZE,TYP,VA_ARGS_NUM(__VA_ARGS__),VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_5d *: (array_5d *) create_array_nd(FLAGS,ESIZE,TYP,5,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_4d *: (array_4d *) create_array_nd(FLAGS,ESIZE,TYP,4,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_3d *: (array_3d *) create_array_nd(FLAGS,ESIZE,TYP,3,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_2d *: (array_2d *) create_array_nd(FLAGS,ESIZE,TYP,2,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }}), \
    array_1d *: (array_1d *) create_array_nd(FLAGS,ESIZE,TYP,1,VA_ARGS_NUM(__VA_ARGS__),(__i32__5__){{ __VA_ARGS__ }})  \
  )

// users should call the generic function array_gbounds rather than array_gbounds_nd
int set_array_gbounds_nd(array_nd *a, int32_t ndims, __i32__5__ lower_bounds);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
// set global lower bounds for dimension indexing, dimension is not altered
// array_1d a1 ; array_gbounds(a1, glbi)
// array_5d a5 ; array_gbounds(a5, glbi, glbj, glbk, glbl, glbm)
#define set_array_gbounds(ARRAY, ...) \
  _Generic((ARRAY), \
    array_nd *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_5d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_4d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_3d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_2d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } }), \
    array_1d *: set_array_gbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5__) { { __VA_ARGS__ } })  \
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
    array_nd *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_5d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_4d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_3d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_2d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }}), \
    array_1d *: set_array_lbounds_nd((array_nd *)ARRAY, VA_ARGS_NUM(__VA_ARGS__), (__i32__5x2__) {{ __VA_ARGS__ }})  \
  )

// users should call the generic function subarray_bounds rather than subarray_bounds_nd
__i32__2__ subarray_lbounds_nd(array_nd *a, int32_t dim, int32_t ndims);
// TODO: inline version
// check ndims == a->ndim, dim < a->ndim, dim > 0

#define subarray_lbounds(ARRAY, DIM) \
  _Generic((ARRAY), \
    array_nd *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, (ARRAY)->ndim), \
    array_5d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 5), \
    array_4d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 4), \
    array_3d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 3), \
    array_2d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 2), \
    array_1d *: subarray_lbounds_nd((array_nd *)ARRAY, DIM, 1)  \
  )

// users should call the generic function subarray_bounds rather than subarray_bounds_nd
__i32__2__ subarray_gbounds_nd(array_nd *a, int32_t dim, int32_t ndims);
// TODO: inline version
// check ndims == a->ndim, dim < a->ndim, dim > 0

#define subarray_gbounds(ARRAY, DIM) \
  _Generic((ARRAY), \
    array_nd *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, (ARRAY)->ndim)), \
    array_5d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 5), \
    array_4d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 4), \
    array_3d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 3), \
    array_2d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 2), \
    array_1d *: subarray_gbounds_nd((array_nd *)ARRAY, DIM, 1)  \
  )

// users should call the generic function subarray_get rather than subarray_get_nd
size_t subarray_get_nd(array_nd *a, void *address, size_t copy_size);

#define subarray_get(ARRAY, dest_address, dest_size) \
  _Generic((ARRAY), \
    array_4d *: subarray_get_nd((array_nd *)ARRAY,dest_address, dest_size), \
    array_3d *: subarray_get_nd((array_nd *)ARRAY,dest_address, dest_size), \
    array_2d *: subarray_get_nd((array_nd *)ARRAY,dest_address, dest_size), \
    array_1d *: subarray_get_nd((array_nd *)ARRAY,dest_address, dest_size)  \
  )

// users should call the generic function subarray_set rather than subarray_set_nd
size_t subarray_set_nd(array_nd *a, void *address, size_t copy_size);

#define subarray_set(ARRAY, dest_address, dest_size) \
  _Generic((ARRAY), \
    array_4d *: subarray_set_nd((array_nd *)ARRAY,dest_address, dest_size), \
    array_3d *: subarray_set_nd((array_nd *)ARRAY,dest_address, dest_size), \
    array_2d *: subarray_set_nd((array_nd *)ARRAY,dest_address, dest_size), \
    array_1d *: subarray_set_nd((array_nd *)ARRAY,dest_address, dest_size)  \
  )

// users should use the macros rather than the xxx_nd function
int invalid_array_nd(array_nd *a);
#define invalid_array(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: invalid_array_nd((array_nd *)ARRAY), \
    array_5d *: invalid_array_nd((array_nd *)ARRAY), \
    array_4d *: invalid_array_nd((array_nd *)ARRAY), \
    array_3d *: invalid_array_nd((array_nd *)ARRAY), \
    array_2d *: invalid_array_nd((array_nd *)ARRAY), \
    array_1d *: invalid_array_nd((array_nd *)ARRAY)  \
    )

int valid_array_nd(array_nd *a);
#define valid_array(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: valid_array_nd((array_nd *)ARRAY), \
    array_5d *: valid_array_nd((array_nd *)ARRAY), \
    array_4d *: valid_array_nd((array_nd *)ARRAY), \
    array_3d *: valid_array_nd((array_nd *)ARRAY), \
    array_2d *: valid_array_nd((array_nd *)ARRAY), \
    array_1d *: valid_array_nd((array_nd *)ARRAY)  \
    )

array_nd *create_subarray(array_nd *a, array_nd *b);

// users should call the generic function subarray_address rather than subarray_address_nd
uint8_t  *subarray_address_nd(array_nd *a);
#define subarray_address(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: subarray_address_nd((array_nd *)ARRAY), \
    array_5d *: subarray_address_nd((array_nd *)ARRAY), \
    array_4d *: subarray_address_nd((array_nd *)ARRAY), \
    array_3d *: subarray_address_nd((array_nd *)ARRAY), \
    array_2d *: subarray_address_nd((array_nd *)ARRAY), \
    array_1d *: subarray_address_nd((array_nd *)ARRAY)  \
    )

// users should call the generic function array_address rather than array_address_nd
void  *array_address_nd(array_nd *a);
#define array_address(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: array_address_nd((array_nd *)ARRAY), \
    array_5d *: array_address_nd((array_nd *)ARRAY), \
    array_4d *: array_address_nd((array_nd *)ARRAY), \
    array_3d *: array_address_nd((array_nd *)ARRAY), \
    array_2d *: array_address_nd((array_nd *)ARRAY), \
    array_1d *: array_address_nd((array_nd *)ARRAY)  \
    )

// users should call the generic function subarray_size rather than subarray_size_nd
size_t subarray_size_nd(array_nd *a);
#define subarray_size(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: subarray_size_nd((array_nd *)ARRAY), \
    array_5d *: subarray_size_nd((array_nd *)ARRAY), \
    array_4d *: subarray_size_nd((array_nd *)ARRAY), \
    array_3d *: subarray_size_nd((array_nd *)ARRAY), \
    array_2d *: subarray_size_nd((array_nd *)ARRAY), \
    array_1d *: subarray_size_nd((array_nd *)ARRAY)  \
    )

// users should call the generic function array_size rather than array_size_nd
size_t array_size_nd(array_nd *a);
#define array_size(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: array_size_nd((array_nd *)ARRAY), \
    array_5d *: array_size_nd((array_nd *)ARRAY), \
    array_4d *: array_size_nd((array_nd *)ARRAY), \
    array_3d *: array_size_nd((array_nd *)ARRAY), \
    array_2d *: array_size_nd((array_nd *)ARRAY), \
    array_1d *: array_size_nd((array_nd *)ARRAY)  \
    )

// users should call the generic function subarray_dimension rather than subarray_dimension_nd
int  subarray_dimension_nd(array_nd *a);
#define subarray_dimension(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: subarray_dimension_nd((array_nd *)ARRAY), \
    array_5d *: subarray_dimension_nd((array_nd *)ARRAY), \
    array_4d *: subarray_dimension_nd((array_nd *)ARRAY), \
    array_3d *: subarray_dimension_nd((array_nd *)ARRAY), \
    array_2d *: subarray_dimension_nd((array_nd *)ARRAY), \
    array_1d *: subarray_dimension_nd((array_nd *)ARRAY)  \
    )

// users should call the generic function array_dimension rather than array_dimension_nd
int       array_dimension_nd(array_nd *a);
#define array_dimension(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: array_dimension_nd((array_nd *)ARRAY), \
    array_5d *: array_dimension_nd((array_nd *)ARRAY), \
    array_4d *: array_dimension_nd((array_nd *)ARRAY), \
    array_3d *: array_dimension_nd((array_nd *)ARRAY), \
    array_2d *: array_dimension_nd((array_nd *)ARRAY), \
    array_1d *: array_dimension_nd((array_nd *)ARRAY)  \
    )

// users should call the generic function array_kind rather than array_kind_nd
static inline char *array_kind_nd(array_nd *a){
  int kind = a->type ;
  if(kind >=0 && kind < 7) return (char *) printable_type[kind] ;
  return "ERROR" ;
}
#define array_kind(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: array_kind_nd((array_nd *)ARRAY), \
    array_5d *: array_kind_nd((array_nd *)ARRAY), \
    array_4d *: array_kind_nd((array_nd *)ARRAY), \
    array_3d *: array_kind_nd((array_nd *)ARRAY), \
    array_2d *: array_kind_nd((array_nd *)ARRAY), \
    array_1d *: array_kind_nd((array_nd *)ARRAY)  \
  )

size_t fix_array_nd(array_nd *a);
#define fix_array(ARRAY) \
  _Generic((ARRAY), \
    array_nd *: fix_array_nd((array_nd *)ARRAY), \
    array_5d *: fix_array_nd((array_nd *)ARRAY), \
    array_4d *: fix_array_nd((array_nd *)ARRAY), \
    array_3d *: fix_array_nd((array_nd *)ARRAY), \
    array_2d *: fix_array_nd((array_nd *)ARRAY), \
    array_1d *: fix_array_nd((array_nd *)ARRAY)  \
  )

#define ARRAY_BYTES  sizeof(int8_t)
#define ARRAY_HWORDS sizeof(int16_t)
#define ARRAY_WORDS  sizeof(int32_t)
// users should call the generic function set_array_value rather than set_array_value_nd
size_t set_array_value_nd(array_nd *a, int32_t v, uint32_t vlen);
#define set_array_value(ARRAY, V, VLEN) \
  _Generic((ARRAY), \
    array_nd *: set_array_value_nd((array_nd *)ARRAY, V, VLEN), \
    array_5d *: set_array_value_nd((array_nd *)ARRAY, V, VLEN), \
    array_4d *: set_array_value_nd((array_nd *)ARRAY, V, VLEN), \
    array_3d *: set_array_value_nd((array_nd *)ARRAY, V, VLEN), \
    array_2d *: set_array_value_nd((array_nd *)ARRAY, V, VLEN), \
    array_1d *: set_array_value_nd((array_nd *)ARRAY, V, VLEN)  \
  )

size_t array_copy_data_nd(array_nd *src, array_nd *dst);
// users should use copy_array_data rather than array_copy_data_nd
#define copy_array_data(SRC, DST) array_copy_data_nd((array_nd *)SRC, (array_nd *)DST)

#endif
