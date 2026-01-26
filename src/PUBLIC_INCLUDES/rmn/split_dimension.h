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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#if ! defined(AXIS_NULL)

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

// array split descriptor along a dimension (axis)
typedef struct{
  int32_t nbk ;   // number of blocks along a dimension
  int16_t ln0 ;   // size of first block along a dimension
  int16_t ln1 ;   // size of all the following blocks along a dimension
} array_axis ;
#define AXIS_NULL (array_axis){ .nbk=0, .ln0=0, .ln1=0 }

// elements of an array block along a dimension
typedef struct{
  int32_t ix0 ;   // index of first element in block along a dimension
  int32_t ixn ;   // index of last element in block along a dimension
} index_range ;

typedef struct{
  int32_t i0  ;   // index of first point along first dimension
  int32_t in  ;   // index of last point along first dimension
  int32_t j0  ;   // index of first point along second dimension
  int32_t jn  ;   // index of last point along second dimension
}ij_range ;       // 2D index range of coordinates

// block ordinal from index and sizes (along one dimension) (unsafe)
// used by block_ordinal
// l   [IN] : index along an array dimension (origin 0)
// ln1 [IN] : size of all blocks but first one along a dimension
// ln0 [IN] : size of first block along a dimension
// return block ordinal along that dimension
static inline int32_t b_index(int32_t l, int32_t ln1, int32_t ln0){
  return (l < ln0) ? 0 : ((l + ln1 - ln0)/ln1) ;
}
// block ordinal from index and axis descriptor (uses b_index)
// l    [IN] : index along an array dimension (origin 0)
// axis [IN] : axis descriptor
// return block ordinal along that dimension
static inline int32_t axis_b_index(int32_t l, array_axis axis){
  return b_index(l, axis.ln1, axis.ln0) ;
}

// get ordinal of block that contains element number index along a dimension
// axis  [IN] : axis descriptor
// index [IN] : position of array element along a dimension (origin 0)
// return block ordinal containing requested element (-1 if error)
static inline int32_t block_ordinal(array_axis axis,int32_t index){
  if(index < 0) return -1 ;                           // invalid index
  int ordinal = axis_b_index(index, axis) ;
  return (ordinal >= axis.nbk) ? -1 : ordinal ;       // chek for ordinal out of range (beyond last block)
}

// index range from block index and sizes (along one dimension) (unsafe)
// bl  [IN] : block index along a dimension
// ln1 [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension (ln/2 <= ln0 < 2*ln)
// return index limits along a dimension for this block
static inline index_range r_limits(int32_t bl, int32_t ln1, int32_t ln0){
  if(bl < 0) return (index_range){.ix0 = 0 , .ixn = -1} ;  // return invalid range
  return (bl == 0) ? (index_range){.ix0 = 0 , .ixn = ln0-1} : (index_range){.ix0 = (bl-1)*ln1 + ln0, .ixn = bl*ln1 + ln0 -1 } ;
}
// index range from block index and axis descriptor (along one dimension) (unsafe)
// bl  [IN] : block index along a dimension
// axis [IN] : axis descriptor
// return index limits along a dimension for this block
static inline index_range axis_r_limits(int32_t bl, array_axis axis){
  if(bl < 0 || bl > (axis.nbk-1)) return (index_range){.ix0 = 0 , .ixn = -1} ;  // return invalid range
  return r_limits(bl, axis.ln1, axis.ln0) ;
}
// static inline index_pair b_limits(int32_t bl, int32_t ln1, int32_t ln0){
//   return (bl == 0) ? (index_pair){.i   = 0 , .j   = ln0-1} :  (index_pair){.i    = (bl-1)*ln1 + ln0, .j   = bl*ln1 + ln0 -1 } ;
// }

// get index limits for block number index along a dimension
// axis    [IN] : axis description
// ordinal [IN] : block ordinal along axis
// return first index and last index for requested block ordinal
// { 0, -1 } is returned in case of errror
static inline index_range block_limits(array_axis axis,int32_t ordinal){
  if((ordinal >= axis.nbk) || (ordinal < 0)) return (index_range) { .ix0 = 0, .ixn = -1 } ;
  return axis_r_limits(ordinal, axis) ;
//   return r_limits(ordinal, axis.ln1 , axis.ln0) ;
}

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
    r.nbk = n / bsize ;                  // number of pieces
    r.ln0 = n - (bsize * r.nbk) ;        // residual

    if(r.ln0 >= minsize) {               // residual >= minsize
      r.nbk++ ;                          // bump number of pieces

    }else{                               // residual < minsize
      if(r.nbk == 0){
        r.nbk = 1 ;                      // number of pieces must be at least 1
      }else{
        r.ln0 += bsize;                  // size of first piece = residual + bsize
      }
    }
    r.ln1 = (r.nbk == 1) ? 0 : bsize ;   // size of all subsequent pieces (0 if r.nbk == 1)
  }else{
    r = AXIS_NULL ;
  }
  return r ;
}

#endif
