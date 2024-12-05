// Hopefully useful code for C
// Copyright (C) 2024  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// data zblocks layout example
//
// zblocks along i (x) : 10   (ZNI)
// zblocks along j (y) : 11   (ZNJ)
// stripe factor : 4        (SF0)
// top stripe factor        (SF1)  (may be smaller than SF0)
//
// the number (ZI) in the zblocks is the sequential position in the data map (Z index)
//
// SF1 = MODULO(ZNJ , SF0)
// if(SF1 == 0) then SF1 = SF0
// STJ = J / SF0                ( stripe number for row J )
// J0  = STJ * SF0              ( J index of lower row in stripe )
// if(J0 + SF0 > ZNJ) then SF = SF1 else SF = SF0    ( stripe factor for this row )
// ZI = (J0 * ZNI) + (J - J0) + (SF1 * I)            ( Z index of tile[I,J] )
//
// row (J)                                                           stripe
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//  10 |  82 |  85 |  88 |  91 |  94 |  97 | 100 | 103 | 106 | 109 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   9 |  81 |  84 |  87 |  90 |  93 |  96 |  99 | 102 | 105 | 108 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   8 |  80 |  83 |  86 |  89 |  92 |  95 |  98 | 101 | 104 | 107 |
//     |     |     |     |     |     |     |     |     |     |     |  [2]
//     +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=======
//     |     |     |     |     |     |     |     |     |     |     |
//   7 |  43 |  47 |  51 |  55 |  59 |  63 |  67 |  71 |  75 |  79 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   6 |  42 |  46 |  50 |  54 |  58 |  62 |  66 |  70 |  74 |  78 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     | ****************|     |     |     |     |     |     |     |
//   5 | *41 |  45 |  49*|  53 |  57 |  61 |  65 |  69 |  73 |  77 |
//     | *   |     |    *|     |     |     |     |     |     |     |
//     +-*---+-----+----*+-----+-----+-----+-----+-----+-----+-----+
//     | *   |     |    *|     |     |     |     |     |     |     |
//   4 | *40 |  44 |  48*|  52 |  56 |  60 |  64 |  68 |  72 |  76 |
//     | *   |     |    *|     |     |     |     |     |     |     |  [1]
//     +=*===+=====+====*+=====+=====+=====+=====+=====+=====+=====+=======
//     | *   |     |    *|     |     |     |     | ##########|     |
//   3 | * 3 |   7 |  11*|  15 |  19 |  23 |  27 | #31 |  35#|  39 |
//     | *   |     |    *|     |     |     |     | #   |    #|     |
//     +-*---+-----+----*+-----+-----+-----+-----+-#---+----#+-----+
//     | *   |     |    *| %%%%%%%%%%%%%%%%|     | #   |    #|     |
//   2 | * 2 |   6 |  10*| %14 |  18 |  22%|  26 | #30 |  34#|  38 |
//     | ****************| %   |     |    %|     | #   |    #|     |
//     +-----+-----+-----+-%---+-----+----%+-----+-#---+----#+-----+
//     |     |     |     | %   |     |    %|     | #   |    #|     |
//   1 |   1 |   5 |   9 | %13 |  17 |  21%|  25 | #29 |  33#|  37 |
//     |     |     |     | %%%%%%%%%%%%%%%%|     | #   |    #|     |
//     +-----+-----+-----+-----+-----+-----+-----+-#---+----#+-----+
//     |     |     |     |     |     |     |     | #   |    #|     |
//   0 |   0 |   4 |   8 |  12 |  16 |  20 |  24 | #28 |  32#|  36 |
//     |     |     |     |     |     |     |     | ##########|     |  [0]
//     +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=======
//        0     1     2     3     4     5     6     7     8     9    column (I)
//
// stripe delimiter =
//
// * delimited region, 12 zblocks
// option 1 : ( probably slowest )
//   read zblocks 2->3, 6->7, 10->11, 40->41, 44->45, 48->49 [ 12 zblocks read, 6 IO requests ]
// option 2 :
//   read zblocks 2->11 and 40->49 [ 20 zblocks read, 2 IO requests ]
// option 3 : ( probably fastest)
//   read zblocks 2->49 [ 48 zblocks read, 1 IO request ]
//
// % delimited region, 6 zblocks
//  option 1 : ( probably slower )
//    read zblocks 13->14, 17->18, 21->22 [ 6 zblocks read, 3 IO requests ]
//  option 2 : ( probably fastest )
//    read zblocks 13->22 [ 10 zblocks read, 1 IO request ]
//
// # delimited region, 8 zblocks
//  option 1 : ( ideal case )
//    read zblocks 28->35 [ 8 zblocks read, 1 IO request ]
# if ! defined(Z_DATA_MAP_VERSION)

# define Z_DATA_MAP_VERSION  10

#include <stdint.h>
#include <stdlib.h>

#include <rmn/ct_assert.h>

// packed data representation (as in buffer read from file or in memory)
//
//   <----------------------------- in memory ----------------------------->
//   <----------- sizeof(zmap) ---------->
//   <- sizeof(mhead) ->                 <- 2*zni*znj ->
//   +-----------------+-----------------+-------------+--------------------+
//   |                 |                 |             |                    |
//   |  memory header  | data map header |    size     | packed data stream |
//   |                 |                 |  [zni*znj]  |                    |
//   +-----------------+-----------------+-------------+--------------------+
//                     |                 |             |
//                     |data_head        |size[]       |data stream
//                     <--------------------- in file ---------------------->
//
// data map can be mapped directly to the beginning of the packed data representation
// uint8_t buffer[buffer_size]
// zmap *data_map = (zmap *) buffer ;
// read(fd, buffer, buffer_size)
// zij = zmap->zni * zmap->znj : number of blocks
// zmap->size[zi]        : size of block with zigzag index zi
// zmap->mem = malloc(zij * sizeof(void *))
//    table of data block addresses, starting at data_ptr
//    data_ptr = data_map + sizeof(zmap) + zmap->zni * zmap->znj * sizeof(int16_t)
//    zmap->mem[0] = data_ptr, zmap->mem[i] = zmap->mem[i-1] + zmap->size[i-1]
//    zmap->mem[zi] is the address of block with zigzag index zi
//
// (lix,ljx) may differ from(li,lj)  (half size to size and a half - 1)
//   li/2 <= lix < li + li/2
//   lj/2 <= ljx < lj + lj/2
// either
// - the first block along a dimension will be larger or smaller
//   block[0,0] : (lix,ljx)          (first block of first row)
//   block[i,0] : ( li,ljx)  (i > 0) (first row)
//   block[0,j] : (lix, lj)  (j > 0) (first column)
// - all blocks have the same dimension 
//   block[i,j] : (li,lj)
//
typedef uint32_t *zblocks ;   // zblocks[zi] is address of block[ zindex(i,j) ]

typedef struct{
  uint32_t signature ;   // 0xBEBEFADA
  uint32_t version:8 ,   // same as file header
            spare:8 ,
            flags:16 ;   // freeable pointers flags
  zblocks *mem ;         // table[zni*znj] : memory addresses of encoded blocks in memory
  uint8_t *options ;     // same dimension as size, options associated with each encoded block
  uint32_t *first ;      // start of compressed data stream
  uint32_t *limit ;      // one past the end of compressed data stream
} mhead ;
CT_ASSERT(sizeof(mhead) == 5 * sizeof(void *) , "unexpected size for in memory header")

typedef struct{          // data map
  // ---------------- start of in memory header ----------------
  mhead mh ;
  // ---------------- start of in file header ----------------
  union{
   uint32_t data_head ;  // target for & operator to get address of start of in file header
   uint32_t signature ;
  } ;
  struct {
    uint32_t version: 8, // version marker
            stripe  : 8, // stripe width (last/top stripe may be narrower)
            flags   :16; // reserved for flags
  } ;
  int32_t gni ;          // first dimension of data array   = lix + (zni - 1) * lni (row size)
  int32_t gnj ;          // second dimension of data array  = ljx + (znj - 1) * lnj (column size)
//   uint32_t nk ;          // third dimension of data array and block array
  int32_t zni ;          // number of blocks in a row
  int32_t znj ;          // number of block rows
  int32_t lni:16 ,       // first dimension of all but first block (number of values)
          lnj:16 ;       // second dimension of all but first block (number of values)
  int32_t lix:16 ,       // first dimension of the first block in row
          ljx:16 ;       // second dimension of blocks in the first (bottom) row
  // ---------------- end of header ----------------
  uint16_t size[] ;      // size (in 32 bit units) of encoded blocks ( size[znj*zni] )
}zmap ;
CT_ASSERT(sizeof(zmap) == sizeof(mhead) + 8 * sizeof(int32_t) , "unexpected size for in memory header")

// block index from index and sizes (along one dimension)
// l   [IN] : index along a dimension of global array
// ln  [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension (ln/2 <= ln0 < 2*ln -1 )
// return block index along that dimension
static inline int32_t b_index(int32_t l, int32_t ln, int32_t ln0){
  return (l + ln - ln0)/ln ;
}

typedef struct{
  int32_t i  ;
  int32_t j  ;
}ij_pair ;               // 2D coordinate pair

// index range from block index and sizes (along one dimension)
// bl  [IN] : block index along a dimension
// ln  [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension (ln/2 <= ln0 < 2*ln -1 )
// return index limits along a dimension for this block
static inline ij_pair b_limits(int32_t bl, int32_t ln, int32_t ln0){
  if(bl == 0){
    return (ij_pair){.i = 0 , .j = ln0-1} ;
  }else{
    return (ij_pair){.i = (bl-1)*ln + ln0, .j = bl*ln + ln0 -1 } ;
  }
}

typedef struct{
  int32_t i0  ;          // index of first point along first dimension
  int32_t in  ;          // index of last point along first dimension
  int32_t j0  ;          // index of first point along second dimension
  int32_t jn  ;          // index of last point along second dimension
}ij_range ;              // 2D index range of coordinates

typedef struct{
  int32_t  ni ;          // number of elements stored along dimension
  uint32_t stride ;      // distance between adjacent elements along dimension
  int32_t  i0 ;          // index of first point along dimension ( 0 -> ni - 1 )
  int32_t  lni ;         // number of elements used along dimension ( 0 -> ni - 1 - i0 )
} dim_desc ;             // i0 = 0 , lni = ni : all elements are used

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

typedef struct{          // 2D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 2
  dim_desc dim[2] ;
} array_2d ;

typedef struct{          // 3D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 3
  dim_desc dim[3] ;
} array_3d ;

typedef struct{          // 4D array
  uint8_t *data ;
  uint8_t *limit ;
  uint32_t esize ;
  uint16_t reserved ;
  uint8_t  type ;
  uint8_t  ndim ;        // better be 4
  dim_desc dim[4] ;
} array_4d ;

// static const seems not to induce the warning
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-variable"
// blank array descriptors for 1/2/3 Dimensions
// defaults to 32 bit unsigned type

// macro to initialize a struct of type array_nd
#define ARRAY_ND(DATA,ESIZE,TYPE,NDIM,SIZE) {.data = DATA, .limit = NULL, .esize = ESIZE, .type = TYPE, .ndim = NDIM }

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

static inline int32_t invalid_array(array_nd *a){
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
static inline uint8_t *array_element(array_nd *a){
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

// initialize a new descriptor representing a sub-array of a
// TODO copy data from a to b
static inline array_nd *array_block(array_nd *a, array_nd *b){
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

// fill array descriptor dimensional information
// address of data, element size, element type are left untouched
// a    [INOUT] : pointer to nD array descriptor (if NULL a new descriptor will be created)
// nd      [IN] : number of dimensions
// dim[nd] [IN] : dimensions
static inline array_nd *new_array_nd(array_nd *a, int nd, int32_t dim[nd]){
  int i ;
  if(nd <= 0) return NULL ;
  for(i = 0 ; i < nd ; i++) { if(dim[i] <= 0) return NULL ; }
  if(a == NULL) {
    a = (array_nd *)malloc(sizeof(array_nd) + nd * sizeof(dim_desc)) ;
  }else{
    if(nd != a->ndim) return NULL ;  // number of dimension mismatch
  }
  if(a == NULL) return NULL ;
  uint32_t stride = 1 ;          // first dimension has stride 1
  a->ndim = nd ;
  for(i = 0 ; i < nd ; i++) {
    a->dim[i].ni = dim[i] ;      // this dimension
    a->dim[i].i0 = 0 ;           // start at index 0
    a->dim[i].lni = dim[i] ;     // full span along this dimension
    a->dim[i].stride = stride ;  // distance to next element along this dimension
    stride *= a->dim[i].ni ;     // stride for next dimension
  }
  return a ;
}

int32_t Zindex_from_i_j(int32_t i, int32_t j, int32_t nti, int32_t ntj, int32_t sf0);
ij_pair Zindex_to_i_j(int32_t zij, int32_t nti, int32_t ntj, int32_t sf0);

int32_t  Z_block_index(zmap *map, int32_t i, int32_t j);
ij_pair  block_index(zmap *map, int32_t i, int32_t j);
ij_range block_limits(zmap *map, int32_t i, int32_t j);

zmap    *new_zmap(int32_t gni, int32_t gnj, int32_t stripe, size_t esize);
zblocks *mem_zmap(zmap *map, uint32_t *data);
ssize_t repack_map(zmap *map);
int     free_zmap(zmap *map, int full);

#endif
