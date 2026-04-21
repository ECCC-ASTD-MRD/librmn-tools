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
#include <rmn/tee_print.h>

#define ARRAY_PKG 0177
// TODO : add a function to get the "effective rank" (ignore upper dimensions == 1)

char *invalid_array_msg[] = { 
  "", 
  "NULL array pointer", 
  "invalid array signature", 
  "NO data in array", 
  "data limit MUST be > start of data", 
  "array rank > max dimensions", 
  "a dimension == 0" , 
  "lower local bound < lower global bound" ,
  "upper local bound > upper global bound" ,
  "array storage too small for dimensions" ,
  "local dimension > global dimension"
};

// is this array_nd invalid ?
// a  [IN] : pointer to array_nd struct
// return > 0 if invalid, 0 otherwise
int invalid_array_nd(array_nd *a_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  if(a == NULL)                return 1 ;   // NULL array pointer
  if(a->signature != HAS_DATA && a->signature != NO_DATA) return 2 ;   // invalid signature
  if(a->data == NULL)          return 3 ;   // NO data
  if(a->limit <= a->data)      return 4 ;   // data limit MUST be > start of data
  if(a->rank > a->ndim)        return 5 ;   // rank larger than max dimensions
  ssize_t size = a->esize ;                 // size of a single array element
  for(i = 0 ; i < a->rank ; i++){
    if(a->dim[i].lnn == 0 || a->dim[i].gnn == 0)                      return  6 ; // invalid dimension
    if(a->dim[i].lnn > a->dim[i].gnn)                                 return 10 ; // local dimension > global dimension
    if(a->dim[i].ln0 < a->dim[i].gn0)                                 return  7 ; // lower local bound < lower global bound
    if(a->dim[i].ln0 + a->dim[i].lnn > a->dim[i].gn0 + a->dim[i].gnn) return  8 ; // upper local bound > upper global bound
    size *= a->dim[i].gnn ;                                                      // size * storage size of this dimension
  }
  if(a->limit - a->data < size) return 9 ;  // not enough memory to accomodate array
  return 0 ;                                // probably valid array_nd struct
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
uint8_t *subarray_address_nd(array_nd *a_){
  if(a_ == NULL) return NULL ;
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  uint32_t i, esize = a->esize ;
  uint8_t *ptr = a->data ;                            // base address of array
  int32_t stride = 1 ;

  if(ptr == NULL) return NULL ;
  for(i = 0 ; i < a->rank ; i++){
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
array_nd *create_subarray(array_nd *a_, array_nd *b_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  array_5d *b = (array_5d *) b_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  if(b == NULL){
    b = (array_5d *) malloc(sizeof(array_nd) + a->rank * sizeof(dim_desc)) ;
    b->rank  = a->rank ;
    b->ndim  = a->rank ;
    b->flags = 0 ;
  }
  if(a->rank != b->rank) return NULL ;
  b->esize = a->esize ;
  b->type  = a->type ;
  b->flags = 0 ;
  int i ;
  ssize_t size = 1 ;
  for(i = 0 ; i < a->rank ; i++){
    b->dim[i].gnn     = a->dim[i].lnn ;
    b->dim[i].lnn     = b->dim[i].gnn ;
    b->dim[i].ln0     = 0 ;
    b->dim[i].gn0     = 0 ;
    size   *= b->dim[i].gnn ;
  }
  b->data = malloc(b->esize * size) ;        // allocate data array
  b->limit = b->data + (b->esize * size) ;   // set array limit
  // copy relevant data from a into b
  return (array_nd *) b ;
}

// free array
// ap    [IN] : pointer to nD array descriptor
// if monolithic, return 2, if data was malloc(ed), return 1
// if both struct and data were malloc(ed), return 3
// if nothing can be freed, return 0.
// if ap is invalid, return -1
int32_t free_array_nd(array_nd *ap){
  int32_t status = 0 ;

  if(invalid_array_nd(ap)) return -1 ;
  if(ap->flags & DATA_MAY_REALLOC){
    if(ap->data) free(ap->data) ;
    ap->data = NULL ;
    status |= 1 ;
  }
  if(ap->flags & STRUCT_CAN_FREE){ free(ap) ; status |= 2 ; }
  return status ;
}
// create a pointer to a n dimensional basic array descriptor
// return pointer to a basic array descriptor with ndim dimensions
array_nd *allocate_array_nd(int32_t ndim){
  array_nd *a = (array_nd *)malloc(sizeof(array_nd) + ndim * sizeof(dim_desc)) ;
  if(a != NULL){
    *a = (array_nd) ARRAY_ZERO ;   // signature is the only valid field
    a->rank = ndim ;               // set rank and ndim fields to proper value
    a->ndim = ndim ;
  }
  return a ;
}

// allocate both array descriptor and space to accomodate array data
// esize   [IN] : size of array elements in bytes
// type    [IN] : data type, see type in rmn/data_kind.h
// rank    [IN] : number of dimensions
// ndm5    [IN] : dimension of dm5 (must match rank)
// dm5[]   [IN] : dimensions
// return pointer to filled descriptor (NULL in case of error)
array_nd *create_array_nd(uint32_t flags, int32_t esize, int8_t type, int32_t rank, int32_t ndm5, __i32__5__ dm5){
  size_t sizea = sizeof(array_nd) + rank * sizeof(dim_desc) ;  // size of descriptor structure
  size_t sizem, nelem = 1 ;
  int32_t i ;
  array_nd *r ;
  uint8_t *data, local_flags = 0 ;
// TODO : adjust esize according to type if necessary : size_of_type[type] / 8 if 1/2/4/8
//        if tuples (check for appropriate multiple)
  if(rank != ndm5 && rank != 0){
    TEE_PKG_MSG(TEE_ERROR, "create_array_nd", ARRAY_PKG, "\001%dD array, %d dimensions", rank, ndm5) ;
//     fprintf(stderr, "make_array_nd ERROR: %d dimensions, %d sizes\n", rank, ndm5) ;
    return NULL ;
  }

  for(i = 0 ; i < rank ; i++){                      // compute number of elements in array
    nelem = nelem * ( (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ) ;
  }
  sizem = nelem * esize ;
  if(flags & DATA_IS_INTERNAL){                     // monolithic struct+data
    r    = (array_nd *) malloc(sizea + sizem) ;     // size of descriptor structure + data size
    if(r == NULL) return NULL ;                     // malloc failed
    data = (uint8_t *)r ;                           // start of array_nd struct
    data += sizea ;                                 // address following dimensional descriptors
    local_flags |= DATA_IS_INTERNAL ;               // flag whole struct + data as being malloc(ed) in one piece
  }else{                                            // 2 calls to malloc, 1 for struct, 1 for data
    r = allocate_array_nd(rank) ;                      // allocate descriptor struct
    if(r == NULL) return NULL ;                     // malloc failed
    data = (uint8_t *)malloc(sizem) ;               // allocate data
    if(data == NULL){                               // malloc failed
      free(r) ;                                     // free previously allocated struct
      return NULL ;
    }
    local_flags |= DATA_MAY_REALLOC ;               // data may be reallocated if need be
  }
  local_flags |= STRUCT_CAN_FREE ;                  // the array descriptor pointer can be freed
  r->ndim = rank ;

// fprintf(stderr, "make_array_nd DEBUG, r = %p, data = %p, overhead = %ld, nelem = %d, esize = %ld\n", r, data, (uint8_t *)data-(uint8_t *)r, nelem, sizem/nelem) ;
  new_array_nd(r, data, esize, type, rank, ndm5, dm5) ;    // fill array descriptor information
  r->flags |= local_flags ;
  return r ;
}

// fill array descriptor dimensional information (representing a FULL array)
// address of data, element size, element type are left untouched
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// mem     [IN] : memory address for array. allocate automatically if NULL
// esize   [IN] : size of array elements in bytes
// type    [IN] : data type, see type in rmn/data_kind.h
// rank    [IN] : number of dimensions (0 < rank <= 5)
// ndm5    [IN] : dimension of dm5 (must match rank if not resizing)
// dm5[]   [IN] : dimensions
// if mem == a->data, a dimension reshape operation will be performed
// return pointer to array descriptor if O.K., NULL in case of error
array_nd *new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t rank, int32_t ndm5, __i32__5__ dm5){
  int32_t i, nelem = 1, reshape ;
// TODO : adjust esize according to type if necessary : size_of_type[type] / 8 if 1/2/4/8
  if(rank != ndm5){
    TEE_PKG_MSG(TEE_ERROR, "new_array_nd", ARRAY_PKG, "\002 %dD array, %d dimensions", rank, ndm5) ;
//     fprintf(stderr, "new_array_nd ERROR: %d dimensions, %d size arguments\n", rank, ndm5) ;
    return  NULL ;
  }
  if(a == NULL){
    a  = allocate_array_nd(rank) ;                        // allocate an array descriptor
    if(a == NULL) return NULL ;                           // failed to allocate
    *a = (array_nd) array_nd_null ;                    // initialize to invalid values (n0 reshape possible)
  }

  reshape = 0 ;
  if(valid_array(a) ){                                    // a MUST be a valid array for reshaping
    reshape = (a->data == mem) ;                          // reshape conditions are met
  }
  for(i = 0 ; i < rank ; i++){                            // compute number of elements in array
    nelem = nelem * ( (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ) ;
  }
  ssize_t size = esize ;
  size *= nelem ;                                         // data array size in bytes

  if( ! reshape ){
    *a = (array_nd) array_nd_null ;                    // precondition to fail, invalidate metadata
    a->ndim = rank ;                                      // set max number of dimensions
  }else{
    if(rank > a->ndim) return NULL ;                      // cannot reshape, rank > available dimensions
// fprintf(stderr, "DEBUG new_array_nd : trying to reshape from size %ld to size %ld, %d dimensions\n", a->limit - a->data, size, a->rank) ;
    if(size > (a->limit - a->data)){                      // OOPS, not enough space
      a->signature = 0 ;                                  // remove signature, leave the rest intact
      return  NULL ;
    }
  }
  array_5d *b = (array_5d *) a ;     // enable dim[] indexing, array_nd has 0 dimension dim[]

  if(mem == NULL){
    mem = malloc(size) ;             // allocate data
    if(mem == NULL) return  NULL ;   // error allocating memory for data array
    b->flags |= DATA_MAY_REALLOC ;   // data may be reallocated if need be
  }

  b->signature = NO_DATA ;           // there is no valid data in valid array
  b->type      = type ;              // (re)set type
  b->esize     = esize ;             // (re)set esize
  b->rank      = rank ;              // (re)set rank
  for(i = 0 ; i < rank ; i++){
    int32_t n = (dm5.i32[i] <= 0) ? 1 : dm5.i32[i] ;
    b->dim[i].gnn = n ;      // number of elements stored along this dimension
    b->dim[i].gn0 = 0 ;      // default lower bound for indexing
    b->dim[i].lnn = n ;      // number of elements used along this dimension
    b->dim[i].ln0 = 0 ;      // default lower bound for indexing ( >= b->dim[i].gn0)
  }
  b->data = mem ;
  // if it was a reshape operation, leave limit as it was
  if( ! reshape ) b->limit = b->data + size ;

  return (array_nd *)b ;
}

// fill array with value
// a    [INOUT] : pointer to nD array descriptor
// v       [IN] : value used as filler
// vlen    [IN] : length in bytes of value (1/2/4)
size_t set_array_value_nd(array_nd *a, int32_t v, uint32_t vlen){
  if(invalid_array(a)) return 0 ;
  size_t size = array_bytes_nd(a) ;
  void *address = array_address_nd(a) ;
  int16_t *w16 = (int16_t *) address ;
  int32_t *w32 = (int32_t *) address ;
  uint32_t i ;
  switch(vlen){
    case 1  : memset(address, v&0xFF, size) ; break ;                       // byte fill
    case 2  : for(i = 0 ; i < size/2 ; i++) w16[i] = v & 0xFFFF ; break ;   // double_byte fill (16 bits)
    case 4  : for(i = 0 ; i < size/4 ; i++) w32[i] = v ; break ;            // quad_byte fill (32 bits)
    default : return 0 ;      // invalid value, no fill
  }
  return size ;
}

// fix array descriptor according to dimensions and esize
// a    [INOUT] : pointer to nD array descriptor
// return array size in bytes, 0 in case of error
// if data pointer of a is NULL, alllocate memory as needed
// if available memory is too small and data pointer is not NULL, fail
size_t fix_array_nd(array_nd *a_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  char *msg = "\003 NULL array pointer" ;
  if(a == NULL) goto fail ;
  uint32_t i, rank = a->rank ;

  a->signature = NO_DATA ;
  a->flags     = 0 ;                      // reset flags
  ssize_t sz = 1 ;
  msg = "\004 a dimension is 0" ;
  for(i = 0 ; i < rank ; i++){            // compute number of elements in array
    if(a->dim[i].gnn == 0) goto fail ;    // invalid dimension
    sz *= a->dim[i].gnn ;
  }
  if(sz == 0) goto fail ;
  sz *= a->esize ;                        // multiply number of elements by element size

  if(a->data == NULL){                    // allocate memory if data pointer is NULL
    a->data = malloc(sz) ;
    msg = "\005 failed to allocate memory for array" ;
    if(a->data == NULL) goto fail ;       // malloc failed
    a->limit = a->data + sz ;
    a->flags |= DATA_MAY_REALLOC ;
// fprintf(stderr, "fix_array_nd : allocated %ld bytes, %ld elements, type = %d, flags = %d\n", sz, sz / a->esize, a->type, a->flags) ;
  }
  msg = "\006 not enough available space to accomodate request" ;
  if(sz > a->limit - a->data) goto fail ; // OOPS, not enough available space to accomodate dimensions/type/esize
  for(i = 0 ; i < rank ; i++){            // set local indexes to global values
    a->dim[i].ln0 = a->dim[i].gn0 = 0 ;
    a->dim[i].lnn = a->dim[i].gnn ;
  }
  return sz ;

fail:
    TEE_PKG_MSG(TEE_ERROR, "fix_array_nd", ARRAY_PKG, "%s", msg) ;
  return 0 ;
}

// set global indexing lower bounds for all dimensions of array
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// rank    [IN] : number of values in lb5 (MUST match number of dimensions)
// lb5[nd] [IN] : lower bounds for dimensions
// return number of dimensions if O.K., 0 if ERROR
// normally called via generic macro  set_array_gbounds
int set_array_gbounds_nd(array_nd *a_, int32_t rank, __i32__5__ lb5){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int32_t i ;
  char *msg = "\001invalid array" ;

  if(invalid_array(a)) goto fail ;
  msg = "\002rank mismatch" ;
  if(rank != a->rank) goto fail ;    // wrong number of dimensions

  for(i = 0 ; i < rank ; i++){
//     if(lb5.i32[i] <= 0){
//       snprintf(buf, sizeof(buf), "\003invalid bound %d, array rank = %d\n", 1, a->rank) ;
//       goto fail ;    // invalid bound
//     }
    a->dim[i].gn0 = lb5.i32[i] ;
    a->dim[i].ln0 = a->dim[i].gn0 ;   // reset subarray bounds to cover full range
    a->dim[i].lnn = a->dim[i].gnn ;
  }
  return rank ;
fail:
    TEE_PKG_MSG(TEE_ERROR, "set_array_gbounds_nd", ARRAY_PKG, "%s", msg) ;
  return 0 ;
}

// set subarray indexing bounds for all dimensions of array
// a      [INOUT] : pointer to nD array descriptor
// narg      [IN] : number of values in lb5 (MUST match 2 x number of dimensions)
// lb5[narg] [IN] : bound pairs for all dimensions (lower_bound_0 upper_bound_0 ... lower_bound_n upper_bound_n)
// return number of dimensions if O.K., 0 if ERROR
// normally called via generic macro  set_array_lbounds
int set_array_lbounds_nd(array_nd *a_, int32_t narg, __i32__5x2__ lb5){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int32_t i, j, rank = narg/2 ;
  char *msg = "", buf[1024] ;

//   msg = "\010invalid array" ; if(invalid_array(a)) goto fail ;
  if((i = invalid_array(a))){
    snprintf(buf, sizeof(buf), "\010invalid array[%s]", invalid_array_msg[i]) ;
    msg = buf ;
    goto fail ;
  }
  msg = "\011number of bounds is odd, MUST be EVEN" ; if(narg & 1) goto fail ;  // narg MUST be EVEN
  if(rank != a->rank){
    snprintf(buf, sizeof(buf), "\012invalid bounds, requested rank = %d, array rank = %d\n", rank, a->rank) ;
    msg = buf ;
    goto fail ;    // wrong number of dimensions
  }

  for(i=j=0 ; i < narg ; i+=2, j++){
    msg = "\013local upper bound < lower bound" ;
    if(lb5.i32[i+1] < lb5.i32[i])                        goto fail ;  // upper bound < lower bound
    msg = "\014local upper bound beyond limits" ;
    int32_t limit = a->dim[j].gn0 + a->dim[j].gnn - 1 ;
    if(lb5.i32[i+1] > limit) goto fail ;                              // upper bound beyond limits
// fprintf(stderr, "[%d] gbounds = %d %d, lbounds= %d %d\n", j, a->dim[j].gn0, a->dim[j].gnn - 1, lb5.i32[i], lb5.i32[i+1]) ;
  }

  for(i=j=0 ; i < narg ; i+=2, j++){
    a->dim[j].ln0 = lb5.i32[i] ;                       // lower bound
    a->dim[j].lnn = lb5.i32[i+1] - lb5.i32[i] + 1 ;    // number of values used in sub array
  }
// fprintf(stderr, "array_lbounds_nd, narg = %d, rank = %d\n", narg, rank) ;
  return rank ;
fail:
  TEE_PKG_MSG(TEE_ERROR, "set_array_lbounds_nd", ARRAY_PKG, "%s", msg) ;
//   fprintf(stderr, "%s\n", msg) ;
  return 0 ;
}

// get size of sub array from array a
// a   [IN] : pointer to nD array descriptor
// return size in bytes of sub array
size_t subarray_bytes_nd(array_nd *a_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  size_t size = 0 ;

  if(invalid_array(a)) goto fail ;
  size = a->esize ;
  for(i = 0 ; i < a->rank ; i++){
    size *= a->dim[i].lnn ;
  }
  return size ;
fail:
  return 0 ;
}
// get size of array a
// a   [IN] : pointer to nD array descriptor
// return size in bytes of array
size_t array_bytes_nd(array_nd *a_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  size_t size = 0 ;

  if(invalid_array(a)) goto fail ;
  size = a->esize ;
  for(i = 0 ; i < a->rank ; i++){
    size *= a->dim[i].gnn ;
  }
  return size ;
fail:
  return 0 ;
}

// get number of elements of sub array from array a
// a   [IN] : pointer to nD array descriptor
// return number of elements in sub array
int subarray_dimension_nd(array_nd *a_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  int nelem = 0 ;

  if(invalid_array(a)) goto fail ;
  nelem = 1 ;
  for(i = 0 ; i < a->rank ; i++){
    nelem *= a->dim[i].lnn ;
  }
  return nelem ;
fail:
  return 0 ;
}
// get number of elements in array a
// a   [IN] : pointer to nD array descriptor
// return number of elements in array
int array_dimension_nd(array_nd *a_){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  int nelem = 0 ;

  if(invalid_array(a)) goto fail ;
  nelem = 1 ;
  for(i = 0 ; i < a->rank ; i++){
    nelem *= a->dim[i].gnn ;
  }
  return nelem ;
fail:
  return 0 ;
}

void array_strides_nd(array_nd *a_, __i32__5__ *strides){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  for(i=0 ; i<5 ; i++){ strides->i32[i] = 0 ; }
  if(invalid_array(a)) goto fail ;
  strides->i32[0] = 1 ;
  for(i=1 ; i<a->rank ; i++) { strides->i32[i] = strides->i32[i-1] * a->dim[i].gnn ; }
  return ;

fail :
  return ;
}

// move 5D sub array into contiguous block
// src  [IN] : points to starting address of sub array [>=lnm][gnl][gnk][gnj][gni]
// gni  [IN] : first storage dimension of sub array
// gnj  [IN] : second storage dimension of sub array
// gnk  [IN] : third storage dimension of sub array
// gnl  [IN] : fourth storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// lnk  [IN] : third dimension of block
// lnl  [IN] : fourth dimension of block
// lnm  [IN] : fifth dimension of block
// dst [OUT] : points to contiguous 5D block [lnm][lnl][lnk][lnj][lni]
// returns number of elements transferred
static size_t subarray_get_5d(int gni, int gnj, int gnk, int gnl, int lni, int lnj, int lnk, int lnl, int lnm,
                               uint32_t src[][gnl][gnk][gnj][gni], uint32_t dst[lnm][lnl][lnk][lnj][lni]){
  int k, l, m ;
  uint32_t *dst_, *src_ ;
// fprintf(stderr, "subarray_get_5d : global %d,%d,%d,%d,%d  local = %d,%d,%d,%d,%d\n", gni,gnj,gnk,gnl,lnm, lni,lnj,lnk,lnl,lnm) ;
  for(m = 0 ; m < lnm ; m++){
    for(l = 0 ; l < lnl ; l++){
      for(k = 0 ; k < lnk ; k++){
        src_ = &src[m][l][k][0][0] ;
        dst_ = &dst[m][l][k][0][0] ;
        move_w32_block(src_, gni, dst_, lni, lni, lnj, NULL) ;
      }
    }
  }
  return lni * lnj * lnk * lnl * lnm ;
}

// move 5D contiguous block back into sub array
// src  [IN] : points to contiguous 5D block [lnm][lnl][lnk][lnj][lni]
// gni  [IN] : first storage dimension of sub array
// gnj  [IN] : second storage dimension of sub array
// gnk  [IN] : third storage dimension of sub array
// gnl  [IN] : fourth storage dimension of sub array
// lni  [IN] : first dimension of block
// lnj  [IN] : second dimension of block
// lnk  [IN] : third dimension of block
// lnl  [IN] : fourth dimension of block
// lnm  [IN] : fifth dimension of block
// dst [OUT] : points to starting address of sub array [>=lnm][gnl][gnk][gnj][gni]
// returns number of elements transferred
static size_t subarray_set_5d(int gni, int gnj, int gnk, int gnl, int lni, int lnj, int lnk, int lnl, int lnm,
                              uint32_t dst[lnm][gnl][gnk][gnj][gni], uint32_t src[lnm][lnl][lnk][lnj][lni]){
  int k, l, m ;
  uint32_t *src_, *dst_ ;
// fprintf(stderr, "subarray_set_5d : global %d,%d,%d,%d,%d  local = %d,%d,%d,%d,%d\n", gni,gnj,gnk,gnl,lnm, lni,lnj,lnk,lnl,lnm) ;
  for(m = 0 ; m < lnm ; m++){
    for(l = 0 ; l < lnl ; l++){
      for(k = 0 ; k < lnk ; k++){
        src_ = &src[m][l][k][0][0] ;
        dst_ = &dst[m][l][k][0][0] ;
        move_w32_block(src_, lni, dst_, gni, lni, lnj, NULL) ;
      }
    }
  }
  return lni * lnj * lnk * lnl * lnm ;
}

// store contents of sub array into a contiguous block
// a             [IN] : pointer to existing array_nd struct
// dest_address [OUT] : pointer to block address
// dest_size     [IN] : size in bytes of block
// returns number of elements transferred
// dest_size MUST be large enough to receive data
ssize_t subarray_get_nd(array_nd *a_, void *dest_address, size_t dest_size, block_properties *bp){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  size_t    data_size    = subarray_bytes(a) ;
  int lni, lnj, lnk, lnl, lnm, gni, gnj, gnk, gnl ;
  uint32_t esize ;
  void *src_address ;
// fprintf(stderr, "subarray_get_nd : dest_size = %ld, data_size = %ld\n", dest_size, data_size);
  if(data_size > dest_size) goto fail ;  // block size smaller than sub array

  if(invalid_array(a)) goto fail ;
  if(a->rank > 5) goto fail ;            // 1D/2D/3D/4D/5D array copy only for now

  esize = a->esize ;
  if(esize & 0x3) goto fail ;            // esize == multiple of 4 only for now
  esize /= 4 ;

  src_address = (void *)subarray_address(a) ;

  switch(a->rank){
  case 1:
    lni = esize*a->dim[0].lnn ;
    return move_data32_block(src_address, lni, dest_address, lni, lni, 1, bp) ;

  case 2:
    gni = esize*a->dim[0].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    return move_data32_block(src_address, gni, dest_address, lni, lni, lnj, bp) ;

  case 3:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    return subarray_get_5d(gni, gnj, lnk, 1, lni, lnj, lnk, 1, 1,
                           (uint32_t (*)[1][1][gnj][gni])src_address, (uint32_t (*)[1][lnk][lnj][lni])dest_address) ;

  case 4:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    gnk = a->dim[2].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    lnl = a->dim[3].lnn ;
    return subarray_get_5d(gni, gnj, gnk, lnl, lni, lnj, lnk, lnl, 1,
                           (uint32_t (*)[1][gnk][gnj][gni])src_address, (uint32_t (*)[lnl][lnk][lnj][lni])dest_address) ;

  case 5:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    gnk = a->dim[2].gnn ;
    gnl = a->dim[3].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    lnl = a->dim[3].lnn ;
    lnm = a->dim[4].lnn ;
    return subarray_get_5d(gni, gnj, gnk, gnl, lni, lnj, lnk, lnl, lnm,
                           (uint32_t (*)[gnl][gnk][gnj][gni])src_address, (uint32_t (*)[lnl][lnk][lnj][lni])dest_address) ;

  default:
    goto fail ;
  }

fail:
  return 0 ;
}

// set sub array contents from contiguous block
// a            [OUT] : pointer to existing array_nd struct
// src_address   [IN] : pointer to block address
// src_size      [IN] : size in bytes of block
// returns number of elements transferred, <= 0 error code if error
// src_size MUST be the same size as the subarray size
ssize_t subarray_set_nd(array_nd *a_, void *src_address, size_t src_size){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  size_t    data_size    = subarray_bytes(a) ;
  int lni, lnj, lnk, lnl, lnm, gni, gnj, gnk, gnl, error = 0 ;
  uint32_t esize ;
  void *dest_address ;
// fprintf(stderr, "subarray_set_nd : src_size = %ld, data_size = %ld\n", src_size/4, data_size/4);
  error = -1 ;
  if(src_size != data_size) goto fail ;  // sub array and data not the same size

  error = -2 ;
  if(invalid_array(a)) goto fail ;       // invalid array struct
  error = -3 ;
  if(a->rank > 5) goto fail ;            // 1D/2D/3D/4D/5D array copy only for now

  esize = a->esize ;
  error = -4 ;
  if(esize & 0x3) goto fail ;            // esize == multiple of 4 only for now
  esize /= 4 ;

  dest_address = (void *)subarray_address(a) ;

  switch(a->rank){
  case 1:
    lni = esize*a->dim[0].lnn ;
    return move_w32_block(src_address, lni, dest_address, lni, lni, 1) ;

  case 2:
    gni = esize*a->dim[0].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    return move_w32_block(src_address, lni, dest_address, gni, lni, lnj) ;

  case 3:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    return subarray_set_5d(gni, gnj, lnk, 1, lni, lnj, lnk, 1, 1,
                           (uint32_t (*)[1][1][gnj][gni])dest_address, (uint32_t (*)[1][lnk][lnj][lni])src_address) ;

  case 4:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    gnk = a->dim[2].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    lnl = a->dim[3].lnn ;
    return subarray_set_5d(gni, gnj, gnk, lnl, lni, lnj, lnk, lnl, 1,
                           (uint32_t (*)[1][gnk][gnj][gni])dest_address, (uint32_t (*)[lnl][lnk][lnj][lni])src_address) ;

  case 5:
    gni = esize*a->dim[0].gnn ;
    gnj = a->dim[1].gnn ;
    gnk = a->dim[2].gnn ;
    gnl = a->dim[3].gnn ;
    lni = esize*a->dim[0].lnn ;
    lnj = a->dim[1].lnn ;
    lnk = a->dim[2].lnn ;
    lnl = a->dim[3].lnn ;
    lnm = a->dim[4].lnn ;
    return subarray_set_5d(gni, gnj, gnk, gnl, lni, lnj, lnk, lnl, lnm,
                           (uint32_t (*)[gnl][gnk][gnj][gni])dest_address, (uint32_t (*)[lnl][lnk][lnj][lni])src_address) ;

  default:
    goto fail ;
  }

fail:
  return error ;
}

// copy data from array src into array dst
// src  [IN] : pointer to existing array_nd struct
// dst  [IN] : pointer to existing array_nd struct
size_t array_copy_data_nd(array_nd *src, array_nd *dst){

  if(invalid_array(src)) goto fail ;              // both arrays must be valid
  if(invalid_array(dst)) goto fail ;

  size_t src_size = array_bytes(src) ;
  size_t dst_size = array_bytes(dst) ;
  if(src_size   != dst_size)   goto fail ;        // and have the same data size
  if(src->esize != dst->esize) goto fail ;

  memcpy(dst->data, src->data, src_size) ;        // copy contents of src into dst
  return src_size / src->esize ;                  // number of elements copied

fail:
  return 0 ;
}

// print array dimensions from array descriptor
// a_    [IN] : pointer to existing array_nd struct
// msg   [IN] : diagnostic message
void print_dims_nd(array_nd *a_, char *msg){
  array_5d *a = (array_5d *) a_ ;     // enable dim[] indexing, array_nd has 0 dimension dim[]
  int i ;
  fprintf(stdout, valid_array(a) ? "[" : "<") ;
  for(i=0 ; i<a->rank ; i++){
    fprintf(stdout, "%3d(%3d:%3d)", a->dim[i].gnn, a->dim[i].ln0, a->dim[i].ln0+a->dim[i].lnn-1) ;
  }
  fprintf(stdout, "%s{%18p}(%ld)%s", valid_array(a) ? "]" : ">", a->data, ARRAY_SIZE(*a), msg) ;
}

// print meta information from array descriptor
// a_    [IN] : pointer to existing array_nd struct
// msg   [IN] : diagnostic message
void print_meta_nd(array_nd *a, char *msg){
  fprintf(stdout, "ndim = %d, rank = %d, flags = %d, type = %d[%s], esize = %lu, s = %8.8x %s",
          a->ndim, a->rank, a->flags, a->type, printable_type[a->type],(uint64_t)a->esize, a->signature, msg) ;
}

// print flags from array descriptor
// a_    [IN] : pointer to existing array_nd struct
// msg   [IN] : diagnostic message
void print_flags_nd(array_nd *a, char *msg){
  uint8_t flags = a->flags ;
  fprintf(stdout, "%s flags = %2.2x, %s%s%s%s%s%s\n",msg, flags,
                  (flags & DATA_IS_INTERNAL) ?  " MONOLITHIC" : " SPLIT_STRUCT" ,
                  (flags & DATA_MAY_REALLOC) ?  " MAY_REALLOC_DATA    " : " MAY_NOT_REALLOC_DATA" ,
                  (flags & STRUCT_CAN_FREE)  ?  " STRUCT_CAN_BE_FREED" : "",
                  array_is_signed(a)        ?  " SIGNATURE_FOUND" : "",
                  array_no_data(a)          ?  " EMPTY" : "" ,
                  array_has_data(a)         ?  " VALID_DATA" : ""
         ) ;
}

// print extended description from array descriptor
// a     [IN] : pointer to existing array_nd struct
// msg   [IN] : diagnostic message
void print_array_description_nd(array_nd *a, char *msg){
  fprintf(stdout, "%s", msg) ;
   print_dims(a, ", ") ;
   print_meta(a, "") ;
   print_flags(a, "\n  ") ;
}
