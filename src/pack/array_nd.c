#include <rmn/array_nd.h>

int32_t invalid_array(array_nd *a){
  int i ;
  if(a == NULL) return 1 ;
  if(a->data == NULL) return 1 ;
  for(i = 0 ; i < a->ndim ; i++){
    if(a->dim[i].ni     <= 0) return 1 ;
    if(a->dim[i].stride <= 0) return 1 ;
    if(a->dim[i].i0  <  0 || a->dim[i].i0  >= a->dim[i].ni) return 1 ;
    if(a->dim[i].lni <= 0 || a->dim[i].lni >  a->dim[i].ni) return 1 ;
  }
  return 0 ;
}

// get address of the first element of an array block
uint8_t *array_element(array_nd *a){
  if(a == NULL) return NULL ;
  uint32_t i, esize = a->esize ;
  uint8_t *ptr = a->data ;                            // base address of array
  if(ptr == NULL) return NULL ;
  for(i = 0 ; i < a->ndim ; i++){
    if(a->dim[i].stride <= 0) return NULL ;
    if(a->dim[i].i0 < 0) return NULL ;
    if(a->dim[i].i0 >= a->dim[i].ni) return NULL ;
    ptr += esize * a->dim[i].stride * a->dim[i].i0 ;  // add displacement for this dimension
  }
  return ptr ;
}

// initialize a new descriptor representing a sub-array of array a
// TODO copy data from a to b
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
    b->dim[i].ni     = a->dim[i].lni ;
    b->dim[i].lni    = b->dim[i].ni ;
    b->dim[i].i0     = 0 ;
    b->dim[i].stride = stride ;
    stride *= b->dim[i].ni ;
    size *= b->dim[i].ni ;
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
// dim[nd] [IN] : dimensions
void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, __i32__5__ dim){
  int32_t i, nelem, stride, n ;
  a->reserved = 0 ;
  a->type = type ;
  a->esize = esize ;
  nelem = 1 ;
  stride = 1 ;
  for(i=0 ; i<5 ; i++){
    if(dim.n0[i] <= 0) break ;
    n = (dim.n0[i] <= 0) ? 1 : dim.n0[i] ;
    nelem = nelem * n ;
    a->dim[i].ni = n ;
    a->dim[i].stride = stride ;
    a->dim[i].i0 = 0 ;
    a->dim[i].lni = n ;
    stride = nelem ;
  }
  size_t size = esize ;
  size *= nelem ;
  if(mem == NULL) mem = malloc(size) ;
  a->ndim = i ;
  a->data = mem ;
  a->limit = a->data + size ;
fprintf(stderr, "%d dimensional array, size = %ld [", a->ndim, size/esize) ;
fprintf(stderr,"%d", a->dim[0].ni) ;
for(i=1 ; i<a->ndim ; i++) fprintf(stderr,",%d", a->dim[i].ni) ;
fprintf(stderr,"]\n");
}
