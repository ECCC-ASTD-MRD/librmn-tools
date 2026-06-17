//
// Copyright (C) 2025, 2026  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025, 2026
//
#if ! defined(NULL_ARRAY_AXIS)

// split array along a dimension (axis)
typedef struct{
  int32_t nbk ;   // number of blocks along a dimension
  int16_t ln0 ;   // size of first block along a dimension
  int16_t ln1 ;   // size of all the following blocks along a dimension
} array_axis ;
// initializer for array_axis
#define NULL_ARRAY_AXIS (array_axis){ .nbk=0, .ln0=0, .ln1=0 }

typedef struct{
  array_axis x ;
} array_axis_1d ;

typedef struct{
  array_axis x ;
  array_axis y ;
} array_axis_2d ;

typedef struct{
  array_axis x ;
  array_axis y ;
  array_axis z ;
} array_axis_3d ;

// size pair for 2D array
typedef struct{
  int32_t i  ;
  int32_t j  ;
}size_pair ;

// size trio for 3D array
typedef struct{
  int32_t i  ;
  int32_t j  ;
  int32_t k  ;
}size_trio ;

// index pair for 2D array
typedef struct{
  int32_t i  ;
  int32_t j  ;
}index_pair ;            // 2D coordinate pair

// index trio for 3D array
typedef struct{
  int32_t i  ;
  int32_t j  ;
  int32_t k  ;
}index_trio ;            // 3D coordinate trio

// boundary elements of an array block along a dimension
typedef struct{
  int32_t ix0 ;   // index of first element in block along a dimension
  int32_t ixn ;   // index of last element in block along a dimension
} index_range ;
// invalid range
#define BAD_INDEX_RANGE (index_range) { .ix0=0, .ixn=-1 }

// 2D index range of coordinates
typedef struct{
  int32_t i0  ;   // index of first point along the first dimension
  int32_t in  ;   // index of last point along the first dimension
  int32_t j0  ;   // index of first point along the second dimension
  int32_t jn  ;   // index of last point along the second dimension
}ij_range ;
// 3D index range of coordinates
typedef struct{
  int32_t i0  ;   // index of first point along the first dimension
  int32_t in  ;   // index of last point along the first dimension
  int32_t j0  ;   // index of first point along the second dimension
  int32_t jn  ;   // index of last point along the second dimension
  int32_t k0  ;   // index of first point along the third dimension
  int32_t kn  ;   // index of last point along the third dimension
}ijk_range ;

// ==================== ordinal of a block along an axis ====================

// compute block position (along a dimension) using element index and sizes (unsafe)
// used by axis_b_index
// l   [IN] : index of array element along an array dimension (origin 0)
// ln1 [IN] : size of all blocks but first one along a dimension
// ln0 [IN] : size of first block along a dimension
// return block ordinal along that dimension (-1 if l < 0)
// no "in range" check is performed on l
static inline int32_t block_index(int32_t l, int32_t ln1, int32_t ln0){
  if(l < 0) return -1 ;                           // invalid index
  return (l < ln0) ? 0 : ((l + ln1 - ln0)/ln1) ;
}

// compute ordinal of block that contains element at position index along a dimension (unsafe)
// uses axis_b_index
// axis  [IN] : axis descriptor
// index [IN] : position of array element along a dimension (origin 0)
// return block ordinal containing requested element (-1 if error)
// no "in range" check is performed on index
static inline int32_t block_ordinal(int32_t index, array_axis axis){
  if(index < 0) return -1 ;                           // invalid index
  int ordinal = block_index(index, axis.ln1, axis.ln0) ;
  return (ordinal >= axis.nbk) ? -1 : ordinal ;       // check for ordinal out of range (beyond last block)
}

// ==================== index range of a block along an axis ====================

// compute index limits using block index and block sizes (along one dimension) (unsafe)
// used by block_limits
// bl  [IN] : block index along a dimension
// ln1 [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension (most of the time : ln1/2 <= ln0 < 2*ln1)
// return index limits along a dimension for this block
// BAD_INDEX_RANGE is returned in case of errror
// no "in range" check is performed on bl
static inline index_range index_limits(int32_t bl, int32_t ln1, int32_t ln0){
  if(bl < 0) return BAD_INDEX_RANGE ;  // return invalid range
  return (bl == 0) ? (index_range){.ix0 = 0 , .ixn = ln0-1} : (index_range){.ix0 = (bl-1)*ln1 + ln0, .ixn = bl*ln1 + ln0 -1 } ;
}

// is ordinal "out of range" according to axis description
// ordinal [IN] : block ordinal(position) along axis
// axis    [IN] : axis description
// return 1 if invalid, 0 if valid
static inline int invalid_index(int32_t ordinal, array_axis axis){
  return (ordinal >= axis.nbk) || (ordinal < 0) ;
}

// is ordinal "in range" according to axis description
// ordinal [IN] : block ordinal(position) along axis
// axis    [IN] : axis description
// return 0 if invalid, 1 if valid
static inline int valid_index(int32_t ordinal, array_axis axis){
  return (ordinal < axis.nbk) || (ordinal >= 0) ;
}

// compute index limits given block ordinal using axis descriptor
// uses axis_r_limits
// ordinal [IN] : block ordinal(position) along axis
// axis    [IN] : axis description
// return indexes of first and last element for this block
// BAD_INDEX_RANGE is returned in case of error
static inline index_range block_limits(int32_t ordinal, array_axis axis){
  if(invalid_index(ordinal, axis)) return BAD_INDEX_RANGE ;
  return index_limits(ordinal, axis.ln1, axis.ln0) ;
}

// ==================== split along an axis ====================

// split n into pieces preferably of size bsize
// n     [IN] : total number of items
// bsize [IN] : requested size of pieces (number of items in piece)
// the first piece may be smaller or larger than the requested size
// if size is even, pieces will be >= bsize/2 or <  bsize + bsize/2
// if size is odd,  pieces will be >  bsize/2 or <= bsize + bsize/2
// pieces will be smaller than the minimum size if n is smaller than the minimum size
// set bsize to default size (64) if <= 0
static inline array_axis split_axis(int n, int bsize){
  array_axis r ;
  if(bsize <= 0) bsize = 64 ;
  if(n > 0 && bsize > 0){
    r.nbk = (n + bsize/2) / bsize ;      // number of pieces
    r.nbk = (r.nbk == 0) ? 1 : r.nbk ;   // cannot be less than 1
    r.ln0 = n - (r.nbk - 1) * bsize ;    // size of first piece
    r.ln1 = (r.nbk == 1) ? 0 : bsize ;   // size of all subsequent pieces (0 if r.nbk == 1)
  }else{
    r = NULL_ARRAY_AXIS ;
  }
  return r ;
}

// split n into pieces preferably of size bsize (and a minimum size)
// n       [IN] : total number of items
// bsize   [IN] : requested size of pieces
// minsize [IN] : minimum size of pieces
// the first piece may be smaller or larger than the requested size
// pieces normally will be >= minsize and <=  bsize + minsize -1
// pieces will be smaller than minsize if n is smaller than minsize
// set bsize to default size (64) if <= 0
static inline array_axis split_axis_min(int n, int bsize, int minsize){
  array_axis r ;
  if(bsize <= 0) bsize = 64 ;
  if(n > 0 && bsize > 0){
    r.nbk = n / bsize ;                  // number of pieces (0 -> ...)
    r.ln1 = bsize ;                      // size of all but first piece
    r.ln0 = n - (bsize * r.nbk) ;        // residual

    if(r.nbk == 0){                      // n < bsize
      r.nbk = 1 ;                        // number of pieces must be at least 1
      r.ln1 = 0 ;                        // only 1 piece
      goto end ;                         // job done
    }

    if(r.ln0 >= minsize) {               // residual >= minsize
      r.nbk++ ;                          // bump number of pieces

    }else{                               // residual < minsize
      r.ln0 += bsize;                    // size of first piece = residual + bsize
    }
    if(r.nbk == 1) r.ln1 = 0 ;           // only one piece, size of all but first piece = 0

  }else{                                 // n or bsize <= 0
    r = NULL_ARRAY_AXIS ;                // invalid description
  }
end:
  return r ;
}

// ==================== 2/3 D split along axis with aspect ratio ====================

// gni    [IN] : global first dimension
// gnj    [IN] : global second dimension
// gnk    [IN] : global third dimension
// bszi   [IN] : desired blocking size along first dimension
// bszj   [IN] : desired blocking size along second dimension
// aspect ratio is expected to be 1/2/3/4
// blocking sizes < 16 not supported
// no splitting will be performed along third dimension
static inline array_axis_3d split_axis_3d(int gni, int gnj, int gnk, int bszi, int bszj){
  array_axis_3d r = { NULL_ARRAY_AXIS, NULL_ARRAY_AXIS, NULL_ARRAY_AXIS} ;
  bszi = (bszi <= 0) ? 64 : bszi ;                      // use default blocking size if <= 0
  bszj = (bszj <= 0) ? 64 : bszj ;                      // use default blocking size if <= 0
  bszi = (bszi < 16) ? 16 : bszi ;                      // blocking size < 16 not supported
  bszj = (bszj < 16) ? 16 : bszj ;                      // blocking size < 16 not supported
  if(gni > 0 && gnj > 0 && gnk > 0){
    r.x = split_axis(gni, bszi)  ;                      // split along x
    r.y = split_axis(gnj, bszj)  ;                      // split along y
    r.z.nbk = gnk ; r.z.ln0 = r.z.ln1 = 1 ;             // no splitting along z, 1 element per block
  }
  return r ;
}

static inline array_axis_3d split_axis_2d(int gni, int gnj, int bszi, int bszj){
  return split_axis_3d(gni, gnj, 1, bszi, bszj) ;
}

#endif
