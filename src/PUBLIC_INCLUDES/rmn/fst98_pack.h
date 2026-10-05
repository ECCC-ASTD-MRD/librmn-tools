// Hopefully useful code for C
// Copyright (C) 2026  Recherche en Prevision Numerique
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
//     M. Valin,   Environnement Canada, 2026
//
#if !defined(SRC_DOUBLE)
//
#include <stdint.h>
//
// import as little as possible from fstd 98 code
#include <rmn/fst98.h>
// source / destination flags (upper 8 bits in word)
#define FST_DOUBLE   8
#define FST_WORD     4
#define FST_SHORT    2
#define FST_BYTE     1
// src/dst size from compound flag
#define SRC_SIZE(FLAG)  ( (FLAG)       & 0xF)
#define DST_SIZE(FLAG)  (((FLAG) >> 4) & 0xF)

#define SRC_DOUBLE    (8)
#define SRC_WORD      (4)
#define SRC_SHORT     (2)
#define SRC_BYTE      (1)
#define DST_DOUBLE    (8 << 4)
#define DST_WORD      (4 << 4)
#define DST_SHORT     (2 << 4)
#define DST_BYTE      (1 << 4)
// disable turbo (for types where turbo is on by default)
#define FST_NO_TURBOPACK  0x0100
// new data types
// floating point with absolute error criteria
#define FST_TYPE_REAL_ABS_ERR 12
// floating point with relative error criteria
#define FST_TYPE_REAL_REL_ERR 11
// signed integers, new encoders and prediction
#define FST_TYPE_SIGNED_NG    10
// unsigned integers, new encoders and prediction
#define FST_TYPE_UNSIGNED_NG   9

// use Big Endian stream encoding
#include <rmn/be_stream.h>
#include <rmn/tile_encoders.h>

#include <rmn/fst_missing.h>
#include <rmn/data_map.h>
#include <rmn/ct_assert.h>

extern  int downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride ; 

// 3D block[nk][nj][ni] containing data to encode
// typedef struct{
//   union{
//     uint8_t  *byte;                 // address of block (byte address)
//     uint32_t *word ;                // address of block (word address)
//   } ;
//   uint16_t ni ;                     // first dimension
//   uint16_t nj ;                     // second dimension (1 if block is 1D)
//   uint16_t nk ;                     // third dimension (1 if block is 1D or 2D)
//   uint8_t  etype ;                  // data element type, see rmn/data_kind.h
//   uint8_t  esize ;                  // element size in bytes -1  (1 <= element size <= 256)
// }block_3d ;

typedef struct{
  uint16_t type ;
  uint8_t  size ;
  uint8_t  nbits ;
  float maxerr ;
  float minabs ;
  float zval ;
} fst_datyp ;
CT_ASSERT(sizeof(fst_datyp) == 16, "ERROR: sizeof(fst_datyp) MUST BE 16")
static const fst_datyp fst_datyp_null = {.type = 0, .size = 0, .nbits = 0, .maxerr = 0.0f, .minabs = 0.0f, .zval = 0.0f } ;

// TODO : eliminate npak, replace datyp with fst_encoding
//! legacy encoders (data types 0,1,2,3,4,5,6,7,8), including turbo and missing values options
int32_t fst98_encode(
  //! [in] Field to encode
  const void * const field_in,
  //! [out] encoded stream
  bitstream *stream_out,
  //! [in] First dimension of the data field
  int ni,
  //! [in] Second dimension of the data field
  int nj,
  //! [in] Third dimension of the data field
  int nk,
  //! [in] Data type of elements (including flags used to control xdf_double/xdf_short/xdf_byte and encoding behavior)
  fst_datyp dtypef,
  //! [out] effective data type and nbits
  int *data_kind) ;

// TODO : replace data_kind with 64 bit metadata datyp:8, control:8, nbits:8, maxerr:8, minabs:8, zval:8, spare:16 ;
//! legacy decoders (data types 0,1,2,3,4,5,6,7,8), including turbo and missing values options
int fst98_decode(
  //! [out] Pointer to where the data read will be placed.  Must be already allocated!
  void * const data_out,
  //! [in] encoded stream
  bitstream *stream_in,
  //! [in] Dimension 1 of the data field
  int ni,
  //! [in] Dimension 2 of the data field
  int nj,
  //! [in] Dimension 3 of the data field
  int nk,
  //! [in] datyp , nbits
  int data_kind,
  //! [in] control for XdfDouble/XdfShort/XdfByte
  int data_control
) ;

#endif
