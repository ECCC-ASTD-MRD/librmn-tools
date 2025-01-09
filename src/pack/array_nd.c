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

#include <rmn/array_nd.h>

// is this array_nd invalid ?
// a  [IN] : pointer to array_nd struct
// return 1 if invalid, 0 otherwise
int invalid_array(array_nd *a){
  int i ;
  if(a == NULL)                return 1 ;   // NULL array pointer
  if(a->data == NULL)          return 1 ;   // NO data
  if(a->signature != IS_ARRAY) return 1 ;   // wrong signature
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

// get address of the first element of a sub array of arrray a
// a  [IN] : pointer to array_nd struct
// return address of first element of array (NULL if error)
uint8_t *subarray_address(array_nd *a){
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

// initialize a new descriptor representing a sub-array of array a as a full array
// a  [IN] : pointer to existing array_nd struct
// b [OUT] : pointer to array_nd struct (may be NULL)
// return pointer to array_nd struct of result (b or new allocated array_nd struct)
// return NULL in case of error
// TODO copy data from a to b
// TODO conditional malloc of b->data
array_nd *create_subarray(array_nd *a, array_nd *b){
  if(b == NULL){
    b = (array_nd *) malloc(sizeof(array_nd) + a->ndim * sizeof(dim_desc)) ;
    b->ndim = a->ndim ;
  }
  if(a->ndim != b->ndim) return NULL ;
  b->esize = a->esize ;
  b->type  = a->type ;
  int i ;
  ssize_t size = 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    b->dim[i].gnn     = a->dim[i].lnn ;
    b->dim[i].lnn     = b->dim[i].gnn ;
    b->dim[i].ln0     = 0 ;
    b->dim[i].gn0     = 0 ;
    size   *= b->dim[i].gnn ;
  }
  b->data = malloc(b->esize * size) ;   // allocate data array
  return b ;
}

// fill array descriptor dimensional information (representing a FULL array)
// address of data, element size, element type are left untouched
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// mem     [IN] : memory address for array. allocate automatically if NULL
// esize   [IN] : size of array elements in bytes
// type    [IN] : data type, see type in array_nd struct
// ndim    [IN] : number of dimensions
// dm5[nd] [IN] : dimensions
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t ndim, int32_t ndm5, __i32__5__ dm5){
  int32_t i, nelem, n ;
//   int32_t stride ;
  if(ndim != ndm5){
    fprintf(stderr, "new_array_nd ERROR: %d dimensions, %d sizes\n", ndim, ndm5) ;
    a->ndim = 0 ;
    return ;
  }
  a->signature = IS_ARRAY ;
  a->type      = type ;
  a->esize     = esize ;
  a->ndim      = ndim ;
  nelem = 1 ;
// fprintf(stderr, "ndim = %d, ", ndim) ;
// for(i = 0 ; i < ndim ; i++){
//   fprintf(stderr, "dm5.i32[%d] = %d ", i, dm5.i32[i]);
// }
// fprintf(stderr, "\n");
//   stride = 1 ;
  for(i = 0 ; i < ndim ; i++){
//     if(dm5.i32[i] <= 0) break ;
    n = (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ;
    nelem = nelem * n ;
    a->dim[i].gnn = n ;      // number of elements stored along this dimension
    a->dim[i].gn0 = 0 ;      // default lower bound for indexing
    a->dim[i].lnn = n ;      // number of elements used along this dimension
    a->dim[i].ln0 = 0 ;      // default lower bound for indexing ( >= a->dim[i].gn0)
//     stride = nelem ;
  }
  size_t size = esize ;
  size *= nelem ;
  if(mem == NULL) mem = malloc(size) ;
  a->data = mem ;
  a->limit = a->data + size ;
fprintf(stderr, "%d dimensional array, size = %ld [", a->ndim, size/esize) ;
fprintf(stderr,"%d", a->dim[0].gnn) ;
for(i=1 ; i<a->ndim ; i++) fprintf(stderr,",%d", a->dim[i].gnn) ;
fprintf(stderr,"]\n");
}

// set global indexing lower bounds for all dimensions of array
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// ndim    [IN] : number of values in lb5 (MUST match number of dimensions)
// lb5[nd] [IN] : lower bounds for dimensions
// return number of dimensions if O.K., 0 if ERROR
// normally called via generic macro  set_array_gbounds
int set_array_gbounds_nd(array_nd *a, int32_t ndim, __i32__5__ lb5){
  int32_t i ;

  if(ndim != a->ndim) return 0 ;    // wrong number of dimensions

  for(i = 0 ; i < ndim ; i++){
    if(lb5.i32[i] <= 0) return 0 ;    // invalid bound
    a->dim[i].gn0 = lb5.i32[i] ;
  }
  return ndim ;
}

// set subarray indexing bounds for all dimensions of array
// a      [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// narg      [IN] : number of values in lb5 (MUST match 2 x number of dimensions)
// lb5[narg] [IN] : bound pairs for all dimensions (lower_bound_0 upper_bound_0 ... lower_bound_n upper_bound_n)
// return number of dimensions if O.K., 0 if ERROR
// normally called via generic macro  set_array_lbounds
int set_array_lbounds_nd(array_nd *a, int32_t narg, __i32__5x2__ lb5){
  int32_t i, j, ndim = narg/2 ;

  if(narg & 1) return 0 ;  // narg MUST be EVEN
  if(ndim != a->ndim){
    fprintf(stderr, "array_lbounds_nd, ndim = %d, a->ndim = %d\n", ndim, a->ndim) ;
    return 0 ;    // wrong number of dimensions
  }

  for(i=j=0 ; i < narg ; i+=2, j++){
    if(lb5.i32[i+1] < lb5.i32[i])                        return 0 ;  // upper bound < lower bound
    if(lb5.i32[i+1] > a->dim[j].gn0 + a->dim[j].gnn - 1) return 0 ;  // upper bound beyond limits
// fprintf(stderr, "[%d] gbounds = %d %d, lbounds= %d %d\n", j, a->dim[j].gn0, a->dim[j].gnn - 1, lb5.i32[i], lb5.i32[i+1]) ;
  }

  for(i=j=0 ; i < narg ; i+=2, j++){
    a->dim[i/2].ln0 = lb5.i32[i] ;                       // lower bound
    a->dim[i/2].lnn = lb5.i32[i+1] - lb5.i32[i] + 1 ;    // number of values from upper bound
  }
// fprintf(stderr, "array_lbounds_nd, narg = %d, ndim = %d\n", narg, ndim) ;
  return ndim ;
}

// get size of sub array from array a
// a   [IN] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// return size in bytes of sub array
int subarray_size(array_nd *a){
  int i ;
  int size = 0 ;

  if(a == NULL) goto fail ;
  size = a->esize ;
  for(i = 0 ; i < a->ndim ; i++){
    size *= a->dim[i].lnn ;
  }
  return size ;
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
  if(data_size > dest_size) goto fail ;  // block size smaller than sub array

  if(invalid_array(a)) goto fail ;
  if(a->ndim > 3) goto fail ;            // 1D/2D/3D array copy only for now

  uint32_t esize = a->esize ;
  if(esize & 0x3) goto fail ;            // esize == multiple of 4 only for now
  esize /= 4 ;

  uint32_t *data_address = (void *)subarray_address(a) ;

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
  int lni, lnj, lnk, gni, gnj ;
  if(src_size > data_size) goto fail ;  // sub array too small

  if(invalid_array(a)) goto fail ;
  if(a->ndim > 3) goto fail ;            // 1D/2D/3D array copy only for now

  uint32_t esize = a->esize ;
  if(esize & 0x3) goto fail ;            // esize == multiple of 4 only for now
  esize /= 4 ;

  uint32_t *data_address = (void *)subarray_address(a) ;

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

  default:
    goto fail ;
  }

fail:
  return 0 ;
}
