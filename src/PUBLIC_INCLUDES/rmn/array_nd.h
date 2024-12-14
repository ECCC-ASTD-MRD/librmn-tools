#if ! defined(ARRAY_ND)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
  int32_t  ni ;          // number of elements stored along dimension
  uint32_t stride ;      // distance between adjacent elements along dimension
  int32_t  i0 ;          // index of first point along dimension ( 0 -> ni - 1 )
  int32_t  lni ;         // number of elements used along dimension ( 0 -> ni - 1 - i0 )
} dim_desc ;             // i0 = 0 , lni = ni : all elements are used

// clang and intel compiler do ne seem to care
#if defined(__PGI) || defined(__INTEL_COMPILERx) || defined(__clang__x) || defined(__INTEL_LLVM_COMPILERx)
// initializer element is not constant according to gcc in the following line
#define dim_null (dim_desc) {.ni=0, .stride=0, .i0=0, .lni=0 }
#else
// what follows is not a constant value according to some compilers (PGI for now)
static const dim_desc  dim_null = {.ni=0, .stride=0, .i0=0, .lni=0 } ;
#endif

typedef struct{          // generic struct for array with n dimensions
  uint8_t *data ;        // starting address of array (byte pointer)
  uint8_t *limit ;       // pointer to 1 byte beyond array (byte pointer)
  uint32_t esize ;       // size of array elements in bytes (1, 2, 4, 8, ..., )
  uint16_t reserved ;
  uint8_t  type ;        // element type, float ('F'), signed integer ('I') , unsigned integer ('U'), ...)
  uint8_t  ndim ;        // number of dimensions
  dim_desc dim[] ;       // dimension descriptor (flexible array member)
} array_nd ;

typedef struct{          // 1D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 1
  dim_desc dim[1] ;
} array_1d ;
static const array_1d array_1d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type='\0', .ndim=1,
                                       .dim[0]=dim_null } ;

typedef struct{          // 2D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 2
  dim_desc dim[2] ;
} array_2d ;
static const array_2d array_2d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type='\0', .ndim=2,
                                       .dim[0]=dim_null, .dim[1]=dim_null } ;

typedef struct{          // 3D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 3
  dim_desc dim[3] ;
} array_3d ;
static const array_3d array_3d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type='\0', .ndim=3,
                                       .dim[0]=dim_null, .dim[1]=dim_null, .dim[2]=dim_null } ;

typedef struct{          // 4D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 4
  dim_desc dim[4] ;
} array_4d ;
static const array_4d array_4d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type='\0', .ndim=4,
                                       .dim[0]=dim_null, .dim[1]=dim_null, .dim[2]=dim_null, .dim[3]=dim_null } ;

typedef struct{          // 5D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 5
  dim_desc dim[5] ;
} array_5d ;
static const array_5d array_5d_null = {.data=NULL, .limit=NULL, .esize=0, .reserved=0, .type='\0', .ndim=5,
                                       .dim[0]=dim_null, .dim[1]=dim_null, .dim[2]=dim_null, .dim[3]=dim_null, .dim[4]=dim_null } ;

// macro to initialize a struct of type array_nd
#define ARRAY_ND(DATA,ESIZE,TYPE,NDIM,SIZE) {.data = DATA, .limit = NULL, .esize = ESIZE, .type = TYPE, .ndim = NDIM }

// static const seems not to induce the warning
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-variable"
// blank array descriptors for 1/2/3 Dimensions
// defaults to 32 bit unsigned type

static const array_1d array_1d_0 = ARRAY_ND(NULL, sizeof(int32_t), 'U', 1, 0) ;
static const array_2d array_2d_0 = ARRAY_ND(NULL, sizeof(int32_t), 'U', 2, 0) ;
static const array_3d array_3d_0 = ARRAY_ND(NULL, sizeof(int32_t), 'U', 3, 0) ;
static const array_4d array_4d_0 = ARRAY_ND(NULL, sizeof(int32_t), 'U', 4, 0) ;
// #pragma GCC diagnostic pop

static inline void array_1d_init(array_1d *a, void *data, ssize_t esize, int type){
  *a = (array_1d) ARRAY_ND(data, esize, type, 1, 0) ;
}

static inline void array_2d_init(array_2d *a, void *data, ssize_t esize, int type){
  *a = (array_2d) ARRAY_ND(data, esize, type, 2, 0) ;
}

static inline void array_3d_init(array_3d *a, void *data, ssize_t esize, int type){
  *a = (array_3d) ARRAY_ND(data, esize, type, 3, 0) ;
}

static inline void array_4d_init(array_4d *a, void *data, ssize_t esize, int type){
  *a = (array_4d) ARRAY_ND(data, esize, type, 4, 0) ;
}

static inline void array_5d_init(array_5d *a, void *data, ssize_t esize, int type){
  *a = (array_5d) ARRAY_ND(data, esize, type, 5, 0) ;
}

typedef struct{   // struct containing an array of 6 integers (the first 5 may get used)
  int32_t n0[6] ;
}__i32__5__ ;

void new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, __i32__5__ dim);

// generic version for 1/2/3/4/5 D arrays ... is 1/2/3/4/5 values, one per dimension
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
