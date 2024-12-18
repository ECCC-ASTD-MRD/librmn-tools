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
int32_t invalid_array(array_nd *a){
  int i ;
  if(a == NULL) return 1 ;
  if(a->data == NULL) return 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    if(a->dim[i].gnn     <= 0) return 1 ;
//     if(a->dim[i].stride <= 0) return 1 ;
    if(a->dim[i].ln0  <  0 || a->dim[i].ln0  >= a->dim[i].gnn) return 1 ;
    if(a->dim[i].lnn <= 0 || a->dim[i].lnn >  a->dim[i].gnn) return 1 ;
  }
  return 0 ;
}

// get address of the first element of a sub array
// a  [IN] : pointer to array_nd struct
// return address of first element of array (NULL if error)
uint8_t *subarray_address(array_nd *a){
  if(a == NULL) return NULL ;
  uint32_t i, esize = a->esize ;
  uint8_t *ptr = a->data ;                            // base address of array
  int32_t stride = 1 ;

  if(ptr == NULL) return NULL ;
  for(i = 0 ; i < a->ndim ; i++){
//     if(a->dim[i].stride <= 0) return NULL ;
    if(a->dim[i].ln0 < 0) return NULL ;
    if(a->dim[i].ln0 >= a->dim[i].gnn) return NULL ;
//     ptr += esize * a->dim[i].stride * a->dim[i].ln0 ;  // add displacement for this dimension
    ptr += esize * stride * a->dim[i].ln0 ;  // add displacement for this dimension
    stride *= a->dim[i].gnn ;                // stride for next dimension
  }
  return ptr ;
}

// initialize a new descriptor representing a sub-array of array a
// a  [IN] : pointer to existing array_nd struct
// b [OUT] : pointer to array_nd struct (may be NULL)
// return pointer to array_nd struct of result (b or new allocated array_nd struct)
// return NULL in case of error
// TODO copy data from a to b
// TODO conditional malloc of b->data
array_nd *array_block(array_nd *a, array_nd *b){
  if(b == NULL){
    b = (array_nd *) malloc(sizeof(array_nd) + a->ndim * sizeof(dim_desc)) ;
    b->ndim = a->ndim ;
  }
  if(a->ndim != b->ndim) return NULL ;
  b->esize = a->esize ;
  b->type  = a->type ;
  uint32_t stride = 1 ;
  int i ;
  ssize_t size = 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    b->dim[i].gnn     = a->dim[i].lnn ;
    b->dim[i].lnn    = b->dim[i].gnn ;
    b->dim[i].ln0     = 0 ;
//     b->dim[i].stride = stride ;
    stride *= b->dim[i].gnn ;
    size *= b->dim[i].gnn ;
  }
  b->data = malloc(b->esize * size) ;   // allocate data array
  return b ;
}

// fill array descriptor dimensional information (representing a FULL array)
// address of data, element size, element type are left untouched
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// mem     [IN] : in memory address for array. allocate automatically if NULL
// esize   [IN] : size of array elements in bytes
// type    [IN] : data type, see type in array_nd struct
// nd      [IN] : number of dimensions
// dm5[nd] [IN] : dimensions
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, __i32__5__ dm5){
  int32_t i, nelem, n ;
//   int32_t stride ;
  a->reserved = 0 ;
  a->type = type ;
  a->esize = esize ;
  nelem = 1 ;
//   stride = 1 ;
  for(i=0 ; i<5 ; i++){
    if(dm5.i32[i] <= 0) break ;
    n = (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ;
    nelem = nelem * n ;
    a->dim[i].gnn = n ;
//     a->dim[i].stride = stride ;
    a->dim[i].ln0 = 0 ;
    a->dim[i].lnn = n ;
//     stride = nelem ;
  }
  size_t size = esize ;
  size *= nelem ;
  if(mem == NULL) mem = malloc(size) ;
  a->ndim = i ;
  a->data = mem ;
  a->limit = a->data + size ;
fprintf(stderr, "%d dimensional array, size = %ld [", a->ndim, size/esize) ;
fprintf(stderr,"%d", a->dim[0].gnn) ;
for(i=1 ; i<a->ndim ; i++) fprintf(stderr,",%d", a->dim[i].gnn) ;
fprintf(stderr,"]\n");
}
