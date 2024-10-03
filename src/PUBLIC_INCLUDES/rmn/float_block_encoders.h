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

#if ! defined(FLOAT_VLA_PTR)

#include <stdint.h>
#include <rmn/ct_assert.h>

// float variable length array
typedef float float_vla[] ;
// pointer to variable length array
#define FLOAT_VLA_PTR (float_vla *)

typedef struct{
  uint32_t vrs1:     8 ,            // versioning indicator
           spare1:   8 ,
           spare2:   8 ,
           spare3:   8 ,
           mode:    12 ,            // packing mode (s/e/m, normalized mantissa, ...)
           nbits:    6 ,            // number of bits per item in packed 4x4 block (0 -> 31)
           bsize:    6 ,            // size of packed 4x4 blocks (including optional block header) in 16 bit units
           ni0:      4 ,            // useful dimension along i of last block in row (0 -> 4)
           nj0:      4 ;            // useful dimension along j of blocks in last(top) row (0 -> 4)
} vprop_2d ;
CT_ASSERT(sizeof(vprop_2d) == 2 * sizeof(uint32_t), "wrong size of struct vprop_2d")

typedef struct{
  vprop_2d desc ;             // virtual array properties
  uint32_t gni, gnj ;         // original float array dimensions
  uint16_t data[] ;           // packed data, bsize * gni * gni * 16 bits
}virtual_float_2d ;

// return index of first point of a 4x4 block along a dimension
static uint32_t block_2_indx_4x4(uint32_t block){
  return block * 4 ;
}
// return 4x4 block number containing index along a dimension
static uint32_t indx_2_block_4x4(uint32_t indx){
  return indx / 4 ;
}

uint16_t *float_block_encode_4x4(float *f, int lni, int nbits, uint16_t *stream, uint32_t ni, uint32_t nj, uint32_t *head);
uint16_t *float_block_decode_4x4(float *f, int nbits, uint16_t *stream16, uint32_t *head);

int32_t float_array_encode_4x4(float *f, int lni, int ni, int nj, uint16_t *stream, int nbits);
void *float_array_section_4x4(float *r, uint32_t gni, uint32_t lni, uint32_t ni, uint32_t nj, uint32_t ix0, uint32_t jx0, uint16_t *stream0, uint32_t nbits);
int32_t float_array_decode_4x4(float *r, int lni, int ni, int nj, uint16_t *stream, int nbits);

#endif
