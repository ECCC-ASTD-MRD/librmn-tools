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
#if ! defined(SPLIT_DIMENSION_H)
#define SPLIT_DIMENSION_H

// generic signed integer pair
typedef struct{
  int32_t i1 ;
  int32_t i2 ;
}int_pair ;              // pair of signed integers

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

// description of a split array dimension (axis)
typedef struct{
  int32_t nbk ;   // number of blocks along a dimension
  int16_t ln0 ;   // size of first block along a dimension
  int16_t ln1 ;   // size of next blocks along a dimension
} array_axis ;

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
// l   [IN] : index along an array dimension
// ln  [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension
// return block ordinal along that dimension
static inline int32_t b_index(int32_t l, int32_t ln1, int32_t ln0){
  return (l + ln1 - ln0)/ln1 ;
}

// index range from block index and sizes (along one dimension)
// bl  [IN] : block index along a dimension
// ln  [IN] : size of all but first block along a dimension
// ln0 [IN] : size of first block along a dimension (ln/2 <= ln0 < 2*ln)
// return index limits along a dimension for this block
static inline index_pair b_limits(int32_t bl, int32_t ln1, int32_t ln0){
  if(bl == 0){
    return (index_pair){.i = 0 , .j = ln0-1} ;
  }else{
    return (index_pair){.i = (bl-1)*ln1 + ln0, .j = bl*ln1 + ln0 -1 } ;
  }
}

// compute first block dimension and number of blocks from size and desired block size
// array_dimension [IN] : size of array along one dimension
// block_size      [IN] : desired block size
// return integer pair, .i1 = number of blocks, .i2 = dimension of first block
// static inline int_pair split_array_dimension(int32_t array_dimension, int32_t block_size){
//   int_pair ijp = {.i1 = 0 , .i2 = 0 } ;   // in case of failure
//   ijp.i1 = array_dimension / block_size ;
//   int extra = array_dimension - ijp.i1  * block_size ;
//   if(extra < block_size/2){
//     ijp.i2 = block_size + extra ;    // first block will be longer than block_size
//   }else{
//     ijp.i1++ ;                       // one more block, shorter than block_size
//     ijp.i2 = extra ;
//   }
//   return ijp ;
// }

// get block ordinal that contains element number index along a dimension
// axis  [IN] : axis description
// index [IN] : index of array element along a dimension
// return block ordinal containing requested element (-1 if error)
static inline int32_t block_ordinal(array_axis axis,int32_t index){
  if(index < 0) return -1 ;                           // invalid index
//   int ordinal = (index + axis.ln1 - axis.ln0) / axis.ln1 ;
  int ordinal = b_index(index, axis.ln1, axis.ln0) ;
  return (ordinal >= axis.nbk) ? -1 : ordinal ;       // chek for ordinal out of range (beyond last block)
}

// get index limits for block number index along a dimension
// axis    [IN] : axis description
// ordinal [IN] : block ordinal along axis
// return first index and last index for requested block ordinal
// { 0, -1 } is returned in case of errror
static inline index_range block_limits(array_axis axis,int32_t ordinal){
  if((ordinal >= axis.nbk) || (ordinal < 0)) return (index_range) { .ix0 = 0, .ixn = -1 } ;
  if(ordinal == 0){
    return (index_range) { .ix0 = 0,
                          .ixn = axis.ln0 - 1 } ;
  }else{
    return (index_range) { .ix0 = axis.ln0 + (ordinal - 1) * axis.ln1 ,
                           .ixn = axis.ln0 + (ordinal * axis.ln1) -1 } ;
  }
}

// split n into pieces preferably of size bsize
// n     [IN] : total number of pieces
// bsize [IN] : requested size of pieces
// the first piece may be smaller or larger than the requested size
// if size is even, pieces will be >= bsize/2 or <  bsize + bsize/2
// if size is odd,  pieces will be >  bsize/2 or <= bsize + bsize/2
// pieces will smaller than the minimum only if n is also smaller
static inline array_axis split_axis(int n, int bsize){
  array_axis r ;
  r.nbk = (n + bsize/2) / bsize ;      // number of pieces
  r.nbk = (r.nbk == 0) ? 1 : r.nbk ;
  r.ln0 = n - (r.nbk - 1) * bsize ;    // size of first piece
  r.ln1 = bsize ;
  return r ;
}

#endif
