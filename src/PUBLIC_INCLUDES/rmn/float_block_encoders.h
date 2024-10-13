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

#if ! defined(IEEE_ENCODER_VERSION)

#define IEEE_ENCODER_VERSION 10

#include <stdint.h>
#include <rmn/ct_assert.h>

// float variable length array
typedef float float_vla[] ;
// pointer to variable length array
#define FLOAT_VLA_PTR (float_vla *)

// codes to identify encoding type
typedef enum { IEEE_4x4 = 0x44,
               IEEE_LIN1 = 0x01,
               IEEE_LIN2 = 0x02,
               IEEE_SEM  = 0x10,
               IEEE_FP16 = 0x20,
               IEEE_BF16 = 0x21
} tile_kind ;

typedef struct{                // compression parameters
  union{
    float   f ;                // largest absolute error (float)
    int32_t i ;                // largest absolute error (integer)
  } max_error ;
  int32_t nbits:  6 ,          // max number of bits to be used by quantizer
          dtype:  3 ,          // data type (int32/int16/int8/float/double)
          kind:   3 ,          // quantizer type (linear/log)
          resv : 20 ;          // provision for max rel error, exponent range, ...
} compression_rules ;

typedef struct{
  uint32_t gni, gnj ;         // dimensions of full data array
  void *map ;                 // optional data map (NULL if not used)
  uint32_t bin, bnj ;         // number of (possibly mapped) chunks along i / j
  uint32_t bszi, bszj ;       // dimensions of (possibly mapped) chunks (may differ from quantizing/predicting/encoding blocks/tiles)
  compression_rules r ;       // compression rules ;
  // may need to add :
  // nb_of_special_values     // 0 means no special values
  // special_values[nnn]      // list of 32 bit special values (used when data is restored)
} FieldHeader ;

// encoding info for a stream of 4x4 tiles (in core array compression)
// header, 16 codes, ..... , header, 16 codes
// the top row and last column may need expansion to full 4x4 size tiles
typedef struct{
  uint32_t kind ;        // identifier code for 4x4 blocks (0x01)
  uint32_t ni0 ;         // useful dimension along i of last block in rows (0 -> 4)
  uint32_t nj0 ;         // useful dimension along j of blocks in last(top) row (0 -> 4)
  uint32_t nbits ;       // number of bits kept for each value
  uint32_t bsize ;       // size of tiles (including optional 16 bit header) in 16 bit units
  // information found in tile header (16 bits) above info is the same for all tiles and found in global header)
  uint32_t emax ;        // largest IEEE exponent found in values (bias is kept)
  uint32_t sbits ;       // number of bits for sign (0 if all values have the same sign)
  uint32_t ebits ;       // number of bits used to encode exponent
  uint32_t sign ;        // sign of all values (ignored if sbits == 1)
  uint32_t noexp ;       // if 1, no exponent is stored, mantissa is normalized to largest absolute value
  uint32_t header ;      // encoded version of above emax:8, sbits:1, ebits:3, sign:1, noexp:1 (<= 16 bits)
} ieee_4x4 ;

// fused struct for ieee encoders/quantizers
// IEEE linear float quantizer type 1 (rounded bias removal , conversion to integers using scaling and rounding)
// IEEE linear float quantizer type 2 (mantissas normalized to largest absolute value)
// IEEE style encoding sign/exponent/mantissa (reduced exponent field width)
// IEEE 16 bit floating point encoding (fp16)
// 16 bit "brain float" point encoding (bf16)
typedef struct{
  uint32_t kind ;        // identifier code for encoding
                         // IEEE32 linear quantizer 1  (0x01)
                         // IEEE32 linear quantizer 2  (0x02)
                         // sign/exponent/mantissa     (0x10)
                         // zfp (not implemented yet)  (0x11)
                         // fp16                       (0x20)
                         // bf16                       (0x21)
  // information found in block headers (32/64 bits)
  // "ref" is used only by some quantizers/encoders
  union{                 // bias/offset/ref/... (32 bit value)
    float    f ;         // used by linear quantizer 1
    int32_t  i ;         // used by linear quantizer 2
    uint32_t u ;
  }ref ;
  uint32_t cnst ;        // range is 0, constant absolute values
  uint32_t allp ;        // all values are >= 0
  uint32_t allm ;        // all values are < 0
  uint32_t clip ;        // if 1, a quantized value of 0 will be restored as 0
  uint32_t nbits ;       // number of bits kept for each value
  uint32_t exp ;         // 8 bit IEEE exponent value (including bias of 127)
                         // exponent for quantization multiplier (linear quantizer 1)
                         // reference exponent for the tile (linear quantizer 2)
                         // scaling exponent (sign/exponent/mantissa reduced exponent)
                         // 127 (scale factor is 1) (fp16 and bf16)
  uint32_t snbits ;      // 5 bit value, number of bits / shift count
                         // bits (0 -> 31) per value (nbts == 0 : same value, same sign) (linear quantizer 1))
                         // shift count (0 -> 31) for mantissa scaling (linear quantizer 2)
                         // number of bits used to encode exponent (sign/exponent/mantissa encoding)
                         // 5 (fp16)
                         // 8 (bf16)
  uint32_t header ;      // encoded bit fields nbits:8, exp:8, snbits:5, cnst:1, allp:1, allm:1, clip:1 (<= 32 bits)
} ieee_encode ;

typedef union{
  ieee_4x4     b4x4 ;
  ieee_encode  ieee ;
}ieee_block ;

// virtual 2 dimensional encoded (compressed) float array
typedef struct{
  uint32_t version ;       // version indicator
  uint32_t bpi ;           // bits per item (redundant in some cases)
  size_t datasize ;        // size of current data contents ( <= maxsize )
  size_t datamax ;         // max size of data array
//   uint32_t gni, gnj ;      // dimensions of full data array   (already in field header)
//   uint32_t mode ;          // packing type (linear, log, normalized mantissa, s/e/m, ...) (found in q_info)
  ieee_block q ;           // quantization/encoding information
  FieldHeader h ;          // field header (some of this info may also be found encoded at the beginning of data[])
  uint8_t data[] ;         // encoded data stream (as an unsigned byte array)
}Vfloat_2d ;
// NOTE: this has to be made consistent with rmn/compress_data.h headers for field/chunk/blocks

// return index of first point of a 4x4 block along a dimension
static uint32_t block_2_indx_4x4(uint32_t block){
  return block * 4 ;
}
// return 4x4 block number containing index along a dimension
static uint32_t indx_2_block_4x4(uint32_t indx){
  return indx / 4 ;
}

Vfloat_2d *new_vfloat_2d(size_t size);
Vfloat_2d *vfloat_array(float *f, int lni, uint32_t ni, uint32_t nj, int nbits, tile_kind kind );
// Vfloat_2d *vfloat_4x4(float *f, int lni, uint32_t ni, uint32_t nj, int nbits);

uint16_t *float_block_encode_4x4(float *f, int lni, int nbits, uint16_t *stream, uint32_t ni, uint32_t nj, uint32_t *head);
uint16_t *float_block_decode_4x4(float *f, int nbits, uint16_t *stream16, uint32_t *head);

int32_t float_array_encode_4x4(float *f, int lni, int ni, int nj, uint16_t *stream, int nbits);
void *float_array_section_4x4(float *r, uint32_t gni, uint32_t lni, uint32_t ni, uint32_t nj, uint32_t ix0, uint32_t jx0, uint16_t *stream0, uint32_t nbits);
int32_t float_array_decode_4x4(float *r, int lni, int ni, int nj, uint16_t *stream, int nbits);

#endif
