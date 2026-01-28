//
// Copyright (C) 2025  Environnement Canada
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
#if ! defined(AXIS_NULL)

// array descriptor split along a dimension (axis)
typedef struct{
  int32_t nbk ;   // number of blocks along a dimension
  int16_t ln0 ;   // size of first block along a dimension
  int16_t ln1 ;   // size of all the following blocks along a dimension
} array_axis ;
// initializer for array_axis
#define AXIS_NULL (array_axis){ .nbk=0, .ln0=0, .ln1=0 }

typedef struct{
  array_axis x ;
  array_axis y ;
} array_axis_2d ;

typedef struct{
  array_axis x ;
  array_axis y ;
  array_axis z ;
} array_axis_3d ;

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
#define INDEX_RANGE_BAD (index_range) { .ix0=0, .ixn=-1 }

typedef struct{
  int32_t i0  ;   // index of first point along first dimension
  int32_t in  ;   // index of last point along first dimension
  int32_t j0  ;   // index of first point along second dimension
  int32_t jn  ;   // index of last point along second dimension
}ij_range ;       // 2D index range of coordinates

// ==================== ordinal of a block along an axis ====================

// block ordinal (along one dimension) from element index and sizes (unsafe)
// used by axis_b_index
// l   [IN] : index of array element along an array dimension (origin 0)
// ln1 [IN] : size of all blocks but first one along a dimension
// ln0 [IN] : size of first block along a dimension
// return block ordinal along that dimension (-1 if l < 0)
static inline int32_t b_index(int32_t l, int32_t ln1, int32_t ln0){
  if(l < 0) return -1 ;                           // invalid index
  return (l < ln0) ? 0 : ((l + ln1 - ln0)/ln1) ;
}

// get ordinal of block that contains element with position index along a dimension
// uses axis_b_index
// axis  [IN] : axis descriptor
// index [IN] : position of array element along a dimension (origin 0)
// return block ordinal containing requested element (-1 if error)
static inline int32_t block_ordinal(int32_t index, array_axis axis){
  if(index < 0) return -1 ;                           // invalid index
  int ordinal = b_index(index, axis.ln1, axis.ln0) ;
  return (ordinal >= axis.nbk) ? -1 : ordinal ;       // check for ordinal out of range (beyond last block)
}

// ==================== index range of a block along an axis ====================

// index limits from block index and sizes (along one dimension) (unsafe)
// used by block_limits
// bl  [IN] : block index along a dimension
// ln1 [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension (ln/2 <= ln0 < 2*ln)
// return index limits along a dimension for this block
// INDEX_RANGE_BAD is returned in case of errror
static inline index_range r_limits(int32_t bl, int32_t ln1, int32_t ln0){
  if(bl < 0) return INDEX_RANGE_BAD ;  // return invalid range
  return (bl == 0) ? (index_range){.ix0 = 0 , .ixn = ln0-1} : (index_range){.ix0 = (bl-1)*ln1 + ln0, .ixn = bl*ln1 + ln0 -1 } ;
}

// index limits for block ordinal from axis descriptor
// uses axis_r_limits
// ordinal [IN] : block ordinal along axis
// axis    [IN] : axis description
// return first index and last index for requested block ordinal
// INDEX_RANGE_BAD is returned in case of errror
static inline index_range block_limits(int32_t ordinal, array_axis axis){
  if((ordinal >= axis.nbk) || (ordinal < 0)) return INDEX_RANGE_BAD ;
  return r_limits(ordinal, axis.ln1, axis.ln0) ;
}

// ==================== split along an axis ====================

// split n into pieces preferably of size bsize
// n     [IN] : total number of items
// bsize [IN] : requested size of pieces
// the first piece may be smaller or larger than the requested size
// if size is even, pieces will be >= bsize/2 or <  bsize + bsize/2
// if size is odd,  pieces will be >  bsize/2 or <= bsize + bsize/2
// pieces will be smaller than the minimum only if n is also smaller
static inline array_axis split_axis(int n, int bsize){
  array_axis r ;
  if(n > 0 && bsize > 0){
    r.nbk = (n + bsize/2) / bsize ;      // number of pieces
    r.nbk = (r.nbk == 0) ? 1 : r.nbk ;   // cannot be less than 1
    r.ln0 = n - (r.nbk - 1) * bsize ;    // size of first piece
    r.ln1 = (r.nbk == 1) ? 0 : bsize ;   // size of all subsequent pieces (0 if r.nbk == 1)
  }else{
    r = AXIS_NULL ;
  }
  return r ;
}

// split n into pieces preferably of size bsize (and a minimum size)
// n       [IN] : total number of items
// bsize   [IN] : requested size of pieces
// minsize [IN] : minimum size of pieces
// the first piece may be smaller or larger than the requested size
// pieces will be >= minsize or <  bsize + minsize -1
// pieces will be smaller than minsize only if n is also smaller
static inline array_axis split_axis_min(int n, int bsize, int minsize){
  array_axis r ;
  if(n > 0 && bsize > 0){
    r.nbk = n / bsize ;                  // number of pieces (0 -> ...)
    r.ln0 = n - (bsize * r.nbk) ;        // residual

    if(r.nbk == 0){                      // n < bsize
      r.nbk = 1 ;                        // number of pieces must be at least 1
      r.ln1 = 0 ;                        // only 1 piece
      return r ;
    }else{
      r.ln1 = bsize ;                    // size of all subsequent pieces
    }

    if(r.ln0 >= minsize) {               // residual >= minsize
      r.nbk++ ;                          // bump number of pieces

    }else{                               // residual < minsize
      r.ln0 += bsize;                    // size of first piece = residual + bsize
    }
    if(r.nbk == 1) r.ln1 = 0 ;
  }else{                                 // n or bsize <= 0
    r = AXIS_NULL ;
  }
  return r ;
}

// ==================== 2/3 D split along axis ====================

static inline array_axis_2d split_axis_2d(int gni, int gnj, int bszi, int aspect){
  array_axis_2d r = { AXIS_NULL, AXIS_NULL} ;
  if(gni > 0 && gnj > 0){
    bszi = (bszi <= 0) ? 64 : bszi ;
    aspect = (aspect <= 0) ? 1 : aspect ;
    r.x = split_axis(gni, bszi)  ;
    r.y = split_axis_min(gnj, bszi*aspect, bszi/2)  ;
  }
  return r ;
}

static inline array_axis_3d split_axis_3d(int gni, int gnj, int gnk, int bszi, int aspect){
  array_axis_3d r = { AXIS_NULL, AXIS_NULL, AXIS_NULL} ;
  if(gni > 0 && gnj > 0 && gnk > 0){
    bszi = (bszi <= 0) ? 64 : bszi ;
    aspect = (aspect <= 0) ? 1 : aspect ;
    r.x = split_axis(gni, bszi)  ;
    r.y = split_axis_min(gnj, bszi*aspect, bszi/2)  ;
    r.z.nbk = gnk ; r.z.ln0 = r.z.ln1 = 1 ;
  }
  return r ;
}

#endif
