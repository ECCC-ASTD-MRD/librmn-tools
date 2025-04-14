// Hopefully useful code for C
// Copyright (C) 2024-2025  Recherche en Prevision Numerique
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
//     M. Valin,   Recherche en Prevision Numerique, 2025

#if ! defined(NULL_PROPERTIES)

#include <rmn/data_kind.h>

// // transform a float into a fake signed integer (comparison order preserving)
// static inline int32_t fake_int(float f){
//   iuf32_t iuf ;
//   iuf.f = f ;
//   return (iuf.i & 0x7FFFFFFF) ^ (iuf.i >> 31) ;
// }
// 
// // restore float from fake integer representing float
// static inline float unfake_float(int32_t fake){
//   iuf32_t iuf ;
//   iuf.i = ((fake >> 31) ^ fake) | (fake & 0x80000000) ;
//   return iuf.f ;
// }

// basic block block properties
typedef struct{
  iuf32_t  maxs ;      // max signed value in block
  iuf32_t  mins ;      // min signed value in block
  iuf32_t  minu ;      // min unsigned value in block
  iuf32_t  maxu ;      // max unsigned value in block (needed for uint_data)
  int32_t  zeros ;     // number of ZERO values in block (-1 if unknown)
  data_kind kind ;     // data type (signed / unsigned / float / unknown / ... )
} block_properties ;

#define NULL_PROPERTIES (block_properties) {.maxs = 0, .mins = 0, .minu = 0, .maxu = 0, .zeros = -1 , .kind = bad_data }

#endif
