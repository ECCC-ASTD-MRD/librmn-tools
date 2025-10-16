//
// Copyright (C) 2024-2025  Environnement Canada
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

#include <rmn/array_nd.h>

// is this array_nd invalid ?
// a  [IN] : pointer to array_nd struct
// return 1 if invalid, 0 otherwise
int invalid_array_nd(array_nd *a){
  int i ;
  if(a == NULL)                return 1 ;   // NULL array pointer
  if(a->signature != IS_ARRAY) return 1 ;   // wrong signature
  if(a->data == NULL)          return 1 ;   // NO data
  if(a->limit <= a->data)      return 1 ;   // data limit MUST be > start of data
  ssize_t size = a->esize ;                 // size of a single array element
  for(i = 0 ; i < a->ndim ; i++){
    if(a->dim[i].lnn <= 0 || a->dim[i].gnn <= 0)                      return 1 ; // bad size
    if(a->dim[i].ln0 < a->dim[i].gn0)                                 return 1 ; // lower local bound < lower global bound
    if(a->dim[i].ln0 + a->dim[i].lnn > a->dim[i].gn0 + a->dim[i].gnn) return 1 ; // upper local bound > upper global bound
    size *= a->dim[i].gnn ;    // * storage size of this dimension
  }
  if(a->limit - a->data < size) return 1 ;  // not enough memory to accomodate array
  return 0 ;  // likely valid array_nd struct
}

// is this array_nd invalid ?
// a  [IN] : pointer to array_nd struct
// return 1 if valid, 0 otherwise
int valid_array_nd(array_nd *a){
  return invalid_array_nd(a) ? 0 : 1 ;
}

// get address of the first element of a sub array of array a
// a  [IN] : pointer to array_nd struct
// return address of first element of sub array (NULL if error)
uint8_t *subarray_address_nd(array_nd *a){
  if(a == NULL) return NULL ;
  uint32_t i, esize = a->esize ;
  uint8_t *ptr = a->data ;                            // base address of array
  int32_t stride = 1 ;

  if(ptr == NULL) return NULL ;
  for(i = 0 ; i < a->ndim ; i++){
    int offset = a->dim[i].ln0 - a->dim[i].gn0 ;
    if(offset < 0) return NULL ;                                                // below global lower bound
    if(a->dim[i].ln0+a->dim[i].lnn > a->dim[i].gn0+a->dim[i].gnn) return NULL ; // above global upper bound
    ptr += esize * stride * offset ;                                 // add displacement for this dimension
    stride *= a->dim[i].gnn ;                                        // stride for next dimension
  }
  return ptr ;
}
// get address of the first element of array a
// a  [IN] : pointer to array_nd struct
// return address of first element of array (NULL if error)
void *array_address_nd(array_nd *a){
  if(a == NULL) return NULL ;
  return a->data ;                            // base address of full array
}

// initialize a new descriptor representing a sub-array of array a as a full array
// a  [IN] : pointer to existing array_nd struct
// b [OUT] : pointer to array_nd struct (may be NULL)
// return pointer to array_nd struct of result (b or new allocated array_nd struct)
// return NULL in case of error
// TODO copy data from a to b
array_nd *create_subarray(array_nd *a, array_nd *b){
  if(b == NULL){
    b = (array_nd *) malloc(sizeof(array_nd) + a->ndim * sizeof(dim_desc)) ;
    b->ndim  = a->ndim ;
    b->flags = 0 ;
  }
  if(a->ndim != b->ndim) return NULL ;
  b->esize = a->esize ;
  b->type  = a->type ;
  b->flags = 0 ;
  int i ;
  ssize_t size = 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    b->dim[i].snn     = a->dim[i].lnn ;      // initial storage dimension
    b->dim[i].gnn     = a->dim[i].lnn ;
    b->dim[i].lnn     = b->dim[i].gnn ;
    b->dim[i].ln0     = 0 ;
    b->dim[i].gn0     = 0 ;
    size   *= b->dim[i].gnn ;
  }
  b->data = malloc(b->esize * size) ;   // allocate data array
  // copy relevant data from a into b
  return b ;
}

// allocate both array descriptor and space to accomodate array data
// esize   [IN] : size of array elements in bytes
// type    [IN] : data type, see type in rmn/data_kind.h
// ndim    [IN] : number of dimensions
// ndm5    [IN] : dimension of dm5 (must match ndim)
// dm5[]   [IN] : dimensions
// return pointer to filled descriptor (NULL in case of error)
array_nd *create_array_nd(uint32_t flags, int32_t esize, int8_t type, int32_t ndim, int32_t ndm5, __i32__5__ dm5){
  size_t sizea = sizeof(array_nd) + ndim * sizeof(dim_desc) ;  // size of descriptor structure
  size_t sizem, nelem = 1 ;
  int32_t i ;
  array_nd *r ;
  uint8_t *data, local_flags = 0 ;

  if(ndim != ndm5){
    fprintf(stderr, "make_array_nd ERROR: %d dimensions, %d sizes\n", ndim, ndm5) ;
    return NULL ;
  }

  for(i = 0 ; i < ndim ; i++){  // compute number of elements in array
    nelem = nelem * ( (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ) ;
  }
  sizem = nelem * esize ;
  if(flags & DATA_IS_INTERNAL){                     // monolithic struct+data
    r    = (array_nd *) malloc(sizea + sizem) ;     // size of descriptor structure + data size
    if(r == NULL) return NULL ;                     // malloc failed
    data = (uint8_t *)r ;                           // start of array_nd struct
    data += sizea ;                                 // address following dimensional descriptors
    local_flags |= DATA_IS_INTERNAL ;               // flag struct as being malloc(ed)
  }else{                                            // 2 calls to malloc, 1 for struct, 1 for data
    r    = (array_nd *) malloc(sizea) ;             // allocate descriptor struct
    if(r == NULL) return NULL ;                     // malloc failed
    data = (uint8_t *)malloc(sizem) ;               // allocate data
    if(data == NULL){                               // malloc failed
      free(r) ;                                     // free previously allocated struct
      return NULL ;
    }
    local_flags |= DATA_MAY_REALLOC ;                  // data may be reallocated if need be
  }
  local_flags |= STRUCT_CAN_FREE ;

// fprintf(stderr, "make_array_nd DEBUG, r = %p, data = %p, overhead = %ld, nelem = %d, esize = %ld\n", r, data, (uint8_t *)data-(uint8_t *)r, nelem, sizem/nelem) ;
  new_array_nd(r, data, esize, type, ndim, ndm5, dm5) ;    // fill array descriptor information
  r->flags |= local_flags ;
  return r ;
}

// fill array descriptor dimensional information (representing a FULL array)
// address of data, element size, element type are left untouched
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// mem     [IN] : memory address for array. allocate automatically if NULL
// esize   [IN] : size of array elements in bytes
// type    [IN] : data type, see type in rmn/data_kind.h
// ndim    [IN] : number of dimensions (0 < ndim <= 5)
// ndm5    [IN] : dimension of dm5 (must match ndim)
// dm5[]   [IN] : dimensions
// TODO : if mem == a->data, implement a dimension reshape operation
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t ndim, int32_t ndm5, __i32__5__ dm5){
  int32_t i, nelem = 1, reshape = 0 ;

  if(valid_array(a) ){                                    // a is a valid array
    reshape = (a->data == mem) ;                          // reshape conditions are met
//     if(reshape) exit(1) ;                                 // reshape is not implemented yet
  }
  if(! reshape) *a = (array_nd) array_nd_invalid ;        // precondition to fail
  if(ndim != ndm5){
    fprintf(stderr, "new_array_nd ERROR: %d dimensions, %d sizes\n", ndim, ndm5) ;
    return ;
  }

  for(i = 0 ; i < ndim ; i++){  // compute number of elements in array
    nelem = nelem * ( (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ) ;
  }
  ssize_t size = esize ;
  size *= nelem ;                    // data array size in bytes

  if(reshape){
    if(size > (a->limit - a->data)){ // OOPS, not enough space
      a->signature = 0 ;             // remove signature, leave the rest intact
      return ;
    }
  }

  a->flags     = 0 ;
  if(mem == NULL){
    mem = malloc(size) ;             // allocate data
    if(mem == NULL) return ;         // error allocating memory for data array
    a->flags |= DATA_MAY_REALLOC ;   // data may be reallocated if need be
  }

  a->signature = IS_ARRAY ;
  a->type      = type ;
  a->esize     = esize ;
  a->ndim      = ndim ;
  for(i = 0 ; i < ndim ; i++){
    int32_t n = (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ;
    a->dim[i].snn = n ;      // initial storage dimension
    a->dim[i].gnn = n ;      // number of elements stored along this dimension
    a->dim[i].gn0 = 0 ;      // default lower bound for indexing
    a->dim[i].lnn = n ;      // number of elements used along this dimension
    a->dim[i].ln0 = 0 ;      // default lower bound for indexing ( >= a->dim[i].gn0)
  }
  a->data = mem ;
  // if it was a reshape operation, leave limit as it was
  if( ! reshape ) a->limit = (mem != NULL) ? a->data + size : a->data ;
//   a->limit = a->data ;
//   if(mem != NULL) a->limit = a->data + size ;
// fprintf(stderr, "%d dimensional array, size = %ld [", a->ndim, size/esize) ;
// fprintf(stderr,"%d", a->dim[0].gnn) ;
// for(i=1 ; i<a->ndim ; i++) fprintf(stderr,",%d", a->dim[i].gnn) ;
// fprintf(stderr,"]\n");
}

// fill array with value
// a    [INOUT] : pointer to nD array descriptor
// v       [IN] : value used as filler
// vlen    [IN] : length in bytes of value (1/2/4)
size_t set_array_value_nd(array_nd *a, int32_t v, uint32_t vlen){
  if(invalid_array(a)) return 0 ;
  size_t size = array_size_nd(a) ;
  void *address = array_address_nd(a) ;
  int16_t *w16 = (int16_t *) address ;
  int32_t *w32 = (int32_t *) address ;
  uint32_t i ;
  switch(vlen){
    case 1  : memset(address, v, size) ; break ;
    case 2  : for(i = 0 ; i < vlen/2 ; i++) w16[i] = v ; break ;
    case 4  : for(i = 0 ; i < vlen/4 ; i++) w32[i] = v ; break ;
    default : return 0 ;      // invalid value, no fill
  }
  return size ;
}

// fix array storage according to dimensions and esize
// a    [INOUT] : pointer to nD array descriptor
// return array size, 0 in case of error
size_t fix_array_nd(array_nd *a){
  if(a == NULL) goto fail ;
  int32_t i, ndim = a->ndim ;
  ssize_t sz = 1 ;

  a->signature = IS_ARRAY ;
  a->flags     = 0 ;
  for(i = 0 ; i < ndim ; i++){            // number of elements in array
    if(a->dim[i].gnn <= 0) goto fail ;
    sz *= a->dim[i].gnn ;
  }
  sz *= a->esize ;                        // * element size
  if(a->data == NULL){                    // allocate memory if data pointer is NULL
    a->data = malloc(a->esize * sz) ;
    if(a->data == NULL) goto fail ;
    a->limit = a->data + sz ;
  }
  for(i = 0 ; i < ndim ; i++){            // set local indexes to global values
    a->dim[i].ln0 = a->dim[i].gn0 = 0 ;
    a->dim[i].lnn = a->dim[i].gnn ;
  }
  return sz ;

fail:
  return 0 ;
}

// set global indexing lower bounds for all dimensions of array
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// ndim    [IN] : number of values in lb5 (MUST match number of dimensions)
// lb5[nd] [IN] : lower bounds for dimensions
// return number of dimensions if O.K., 0 if ERROR
// normally called via generic macro  set_array_gbounds
int set_array_gbounds_nd(array_nd *a, int32_t ndim, __i32__5__ lb5){
  int32_t i ;

  if(invalid_array(a)) goto fail ;
  if(ndim != a->ndim) goto fail ;    // wrong number of dimensions

  for(i = 0 ; i < ndim ; i++){
    if(lb5.i32[i] <= 0) goto fail ;    // invalid bound
    a->dim[i].gn0 = lb5.i32[i] ;
    a->dim[i].ln0 = a->dim[i].gn0 ;   // reset subarray bounds to cover full range
    a->dim[i].lnn = a->dim[i].gnn ;
  }
  return ndim ;
fail:
  return 0 ;
}

// set subarray indexing bounds for all dimensions of array
// a      [INOUT] : pointer to nD array descriptor
// narg      [IN] : number of values in lb5 (MUST match 2 x number of dimensions)
// lb5[narg] [IN] : bound pairs for all dimensions (lower_bound_0 upper_bound_0 ... lower_bound_n upper_bound_n)
// return number of dimensions if O.K., 0 if ERROR
// normally called via generic macro  set_array_lbounds
int set_array_lbounds_nd(array_nd *a, int32_t narg, __i32__5x2__ lb5){
  int32_t i, j, ndim = narg/2 ;

  if(invalid_array(a)) goto fail ;
  if(narg & 1) goto fail ;  // narg MUST be EVEN
  if(ndim != a->ndim){
    fprintf(stderr, "array_lbounds_nd, ndim = %d, a->ndim = %d\n", ndim, a->ndim) ;
    goto fail ;    // wrong number of dimensions
  }

  for(i=j=0 ; i < narg ; i+=2, j++){
    if(lb5.i32[i+1] < lb5.i32[i])                        goto fail ;  // upper bound < lower bound
    if(lb5.i32[i+1] > a->dim[j].gn0 + a->dim[j].gnn - 1) goto fail ;  // upper bound beyond limits
// fprintf(stderr, "[%d] gbounds = %d %d, lbounds= %d %d\n", j, a->dim[j].gn0, a->dim[j].gnn - 1, lb5.i32[i], lb5.i32[i+1]) ;
  }

  for(i=j=0 ; i < narg ; i+=2, j++){
    a->dim[j].ln0 = lb5.i32[i] ;                       // lower bound
    a->dim[j].lnn = lb5.i32[i+1] - lb5.i32[i] + 1 ;    // number of values used in sub array
  }
// fprintf(stderr, "array_lbounds_nd, narg = %d, ndim = %d\n", narg, ndim) ;
  return ndim ;
fail:
  return 0 ;
}

// get size of sub array from array a
// a   [IN] : pointer to nD array descriptor
// return size in bytes of sub array
size_t subarray_size_nd(array_nd *a){
  int i ;
  size_t size = 0 ;

  if(invalid_array(a)) goto fail ;
  size = a->esize ;
  for(i = 0 ; i < a->ndim ; i++){
    size *= a->dim[i].lnn ;
  }
  return size ;
fail:
  return 0 ;
}
// get size of array a
// a   [IN] : pointer to nD array descriptor
// return size in bytes of array
size_t array_size_nd(array_nd *a){
  int i ;
  size_t size = 0 ;

  if(invalid_array(a)) goto fail ;
  size = a->esize ;
  for(i = 0 ; i < a->ndim ; i++){
    size *= a->dim[i].gnn ;
  }
  return size ;
fail:
  return 0 ;
}

// get number of elements of sub array from array a
// a   [IN] : pointer to nD array descriptor
// return number of elements in sub array
int subarray_dimension_nd(array_nd *a){
  int i ;
  int nelem = 0 ;

  if(invalid_array(a)) goto fail ;
  nelem = 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    nelem *= a->dim[i].lnn ;
  }
  return nelem ;
fail:
  return 0 ;
}
// get number of elements in array a
// a   [IN] : pointer to nD array descriptor
// return number of elements in array
int array_dimension_nd(array_nd *a){
  int i ;
  int nelem = 0 ;

  if(invalid_array(a)) goto fail ;
  nelem = 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    nelem *= a->dim[i].gnn ;
  }
  return nelem ;
fail:
  return 0 ;
}

// move 1D sub array into block
// src  [IN] : points to starting address of sub array
// lni  [IN] : first dimension of block and sub array
// dst [OUT] : points to contiguous block
// returns number of elements transferred
static size_t subarray_get_1d(int lni, uint32_t src[lni], uint32_t dst[lni]){
  int i ;
  for(i = 0 ; i < lni ; i++){
    dst[i] = src[i] ;
  }
  return lni ;
}

// move 1D block back into sub array
// src  [IN] : points to contiguous block
// lni  [IN] : first dimension of block and sub array
// dst [OUT] : points to starting address of sub array
// returns number of elements transferred
static size_t subarray_set_1d(int lni, uint32_t dst[lni], uint32_t src[lni]){
  int i ;
  for(i = 0 ; i < lni ; i++){
    dst[i] = src[i] ;
  }
  return lni ;
}

// move 2D sub array into contiguous block
// src  [IN] : points to starting address of sub array
// gni  [IN] : first storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// dst [OUT] : points to contiguous block
// returns number of elements transferred
static size_t subarray_get_2d(int gni, int lni, int lnj, uint32_t src[lnj][gni], uint32_t dst[lnj][lni]){
  int i, j ;
  for(j = 0 ; j < lnj ; j++){
    for(i = 0 ; i < lni ; i++){
      dst[j][i] = src[j][i] ;
    }
  }
  return lni * lnj ;
}

// move 2D contiguous block back into sub array
// src  [IN] : points to contiguous block
// gni  [IN] : first storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// dst [OUT] : points to starting address of sub array
// returns number of elements transferred
static size_t subarray_set_2d(int gni, int lni, int lnj, uint32_t dst[lnj][gni], uint32_t src[lnj][lni]){
  int i, j ;
  for(j = 0 ; j < lnj ; j++){
    for(i = 0 ; i < lni ; i++){
      dst[j][i] = src[j][i] ;
    }
  }
  return lni * lnj ;
}

// move 3D sub array into contiguous block
// src  [IN] : points to starting address of sub array
// gni  [IN] : first storage dimension of sub array
// gnj  [IN] : second storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// lnk  [IN] : third dimension of block
// dst [OUT] : points to contiguous block
// returns number of elements transferred
static size_t subarray_get_3d(int gni, int gnj, int lni, int lnj, int lnk,
                               uint32_t src[lnk][gnj][gni], uint32_t dst[lnk][lnj][lni]){
  int i, j, k ;
  for(k = 0 ; k < lnk ; k++){
    for(j = 0 ; j < lnj ; j++){
      for(i = 0 ; i < lni ; i++){
        dst[k][j][i] = src[k][j][i] ;
      }
    }
  }
  return lni * lnj * lnk ;
}

// move 3D contiguous block back into sub array
// src  [IN] : points to contiguous block
// gni  [IN] : first storage dimension of sub array
// gnj  [IN] : second storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// lnk  [IN] : third dimension of block
// dst [OUT] : points to starting address of sub array
// returns number of elements transferred
static size_t subarray_set_3d(int gni, int gnj, int lni, int lnj, int lnk,
                               uint32_t dst[lnk][gnj][gni], uint32_t src[lnk][lnj][lni]){
  int i, j, k ;
  for(k = 0 ; k < lnk ; k++){
    for(j = 0 ; j < lnj ; j++){
      for(i = 0 ; i < lni ; i++){
        dst[k][j][i] = src[k][j][i] ;
      }
    }
  }
  return lni * lnj * lnk ;
}

// move 4D sub array into contiguous block
// src  [IN] : points to starting address of sub array
// gni  [IN] : first storage dimension of sub array
// gnj  [IN] : second storage dimension of sub array
// gnk  [IN] : third storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// lnk  [IN] : third dimension of block
// lnl  [IN] : fourth dimension of block
// dst [OUT] : points to contiguous block
// returns number of elements transferred
static size_t subarray_get_4d(int gni, int gnj, int gnk, int lni, int lnj, int lnk, int lnl,
                               uint32_t src[lnl][gnk][gnj][gni], uint32_t dst[lnl][lnk][lnj][lni]){
  int i, j, k, l ;
  for(l = 0 ; l < lnl ; l++){
    for(k = 0 ; k < lnk ; k++){
      for(j = 0 ; j < lnj ; j++){
        for(i = 0 ; i < lni ; i++){
          dst[l][k][j][i] = src[l][k][j][i] ;
        }
      }
    }
  }
  return lni * lnj * lnk ;
}

// move 4D contiguous block back into sub array
// src  [IN] : points to contiguous block
// gni  [IN] : first storage dimension of sub array
// gnj  [IN] : second storage dimension of sub array
// gnk  [IN] : third storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// lnk  [IN] : third dimension of block
// lnl  [IN] : fourth dimension of block
// dst [OUT] : points to starting address of sub array
// returns number of elements transferred
static size_t subarray_set_4d(int gni, int gnj, int gnk, int lni, int lnj, int lnk, int lnl,
                               uint32_t dst[lnl][gnk][gnj][gni], uint32_t src[lnl][lnk][lnj][lni]){
  int i, j, k, l ;
  for(l = 0 ; l < lnl ; l++){
    for(k = 0 ; k < lnk ; k++){
      for(j = 0 ; j < lnj ; j++){
        for(i = 0 ; i < lni ; i++){
          dst[l][k][j][i] = src[l][k][j][i] ;
        }
      }
    }
  }
  return lni * lnj * lnk ;
}

// store contents of sub array into a contiguous block
// a             [IN] : pointer to existing array_nd struct
// dest_address [OUT] : pointer to block address
// dest_size     [IN] : size in bytes of block
// returns number of elements transferred
size_t subarray_get_nd(array_nd *a, void *dest_address, size_t dest_size){
  size_t    data_size    = subarray_size(a) ;
  int lni, lnj, lnk, lnl, gni, gnj, gnk ;
  uint32_t esize, *data_address ;
  if(data_size > dest_size) goto fail ;  // block size smaller than sub array

  if(invalid_array(a)) goto fail ;
  if(a->ndim > 3) goto fail ;            // 1D/2D/3D array copy only for now

  esize = a->esize ;
  if(esize & 0x3) goto fail ;            // esize == multiple of 4 only for now
  esize /= 4 ;

  data_address = (void *)subarray_address(a) ;

  switch(a->ndim){
  case 1:
    lni = esize*a->dim[0].lnn ;
    return subarray_get_1d(lni, 
                           (uint32_t *)data_address, (uint32_t *)dest_address) ;

  case 2:
    gni = esize*a->dim[0].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    return subarray_get_2d(gni, lni, lnj,
                           (uint32_t (*)[gni])data_address, (uint32_t (*)[lni])dest_address) ;

  case 3:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    return subarray_get_3d(gni, gnj, lni, lnj, lnk,
                           (uint32_t (*)[gnj][gni])data_address, (uint32_t (*)[lnj][lni])dest_address) ;

  case 4:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    gnk = a->dim[2].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    lnl = a->dim[3].lnn ;
    return subarray_get_4d(gni, gnj, gnk, lni, lnj, lnk, lnl,
                           (uint32_t (*)[gnk][gnj][gni])data_address, (uint32_t (*)[lnk][lnj][lni])dest_address) ;

  default:
    goto fail ;
  }

fail:
  return 0 ;
}

// set sub array contents from contiguous block
// a            [OUT] : pointer to existing array_nd struct
// dest_address  [IN] : pointer to block address
// dest_size     [IN] : size in bytes of block
// returns number of elements transferred
size_t subarray_set_nd(array_nd *a, void *src_address, size_t src_size){
  size_t    data_size    = subarray_size(a) ;
  int lni, lnj, lnk, lnl, gni, gnj, gnk ;
  uint32_t esize, *data_address ;

  if(src_size > data_size) goto fail ;  // sub array too small

  if(invalid_array(a)) goto fail ;
  if(a->ndim > 3) goto fail ;            // 1D/2D/3D array copy only for now

  esize = a->esize ;
  if(esize & 0x3) goto fail ;            // esize == multiple of 4 only for now
  esize /= 4 ;

  data_address = (void *)subarray_address(a) ;

  switch(a->ndim){
  case 1:
    lni = esize*a->dim[0].lnn ;
    return subarray_set_1d(lni, 
                           (uint32_t *)data_address, (uint32_t *)src_address) ;

  case 2:
    gni = esize*a->dim[0].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    return subarray_set_2d(gni, lni, lnj,
                           (uint32_t (*)[gni])data_address, (uint32_t (*)[lni])src_address) ;

  case 3:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    return subarray_set_3d(gni, gnj, lni, lnj, lnk,
                           (uint32_t (*)[gnj][gni])data_address, (uint32_t (*)[lnj][lni])src_address) ;

  case 4:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    gnk = a->dim[2].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    lnl = a->dim[3].lnn ;
    return subarray_set_4d(gni, gnj, gnk, lni, lnj, lnk, lnl,
                           (uint32_t (*)[gnk][gnj][gni])data_address, (uint32_t (*)[lnk][lnj][lni])src_address) ;

  default:
    goto fail ;
  }

fail:
  return 0 ;
}

// copy data from array src into array dst
// src  [IN] : pointer to existing array_nd struct
// dst  [IN] : pointer to existing array_nd struct
size_t array_copy_data_nd(array_nd *src, array_nd *dst){

  if(invalid_array(src)) goto fail ;              // both arrays must be valid
  if(invalid_array(dst)) goto fail ;

  size_t src_size = array_size(src) ;
  size_t dst_size = array_size(dst) ;
  if(src_size   != dst_size)   goto fail ;        // and have the same data size
  if(src->esize != dst->esize) goto fail ;

  memcpy(dst->data, src->data, src_size) ;        // copy contents of src into dst
  return src_size / src->esize ;                  // number of elements copied

fail:
  return 0 ;
}
