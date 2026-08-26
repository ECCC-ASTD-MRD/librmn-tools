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
#define SRC_DOUBLE    ( 8 << 24)
#define SRC_WORD      ( 4 << 24)
#define SRC_SHORT     ( 2 << 24)
#define SRC_BYTE      ( 1 << 24)
#define DST_DOUBLE    ( 8 << 28)
#define DST_WORD      ( 4 << 28)
#define DST_SHORT     ( 2 << 28)
#define DST_BYTE      ( 1 << 28)
// length from flag
#define SRC_LENGTH(FLAG) ((FLAG >> 24) & 0xF)
#define DST_LENGTH(FLAG) ((FLAG >> 28) & 0xF)

#include <rmn/fst_missing.h>
#include <rmn/data_map.h>
// already included by rmn/data_map.h
// #include <rmn/mem_range.h>

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

//! legacy encoders (data types 0,1,2,3,4,5,6,7,8), including turbo and missing values options
RANGE(int32_t) fst98_encode(
  //! [in] Field to encode
  const void * const field_in,
  //! [out] encoded field
  RANGE(int32_t) field_out,
  //! [in] Number of bits kept for the elements of the field
  int npak,
  //! [in] First dimension of the data field
  int ni,
  //! [in] Second dimension of the data field
  int nj,
  //! [in] Third dimension of the data field
  int nk,
  //! [in] Data type of elements (including flags used to control xdf_double/xdf_short/xdf_byte)
  const int datyp,
  //! [out] effective datyp + nbits
  int *data_kind) ;

//! legacy decoders (data types 0,1,2,3,4,5,6,7,8), including turbo and missing values options
int fst98_decode(
  //! [out] Pointer to where the data read will be placed.  Must be already allocated!
  void * const data_out,
  //! [in] Pointer to the encoded data
  void * const data_in,
  //! [in] Dimension 1 of the data field
  int ni,
  //! [in] Dimension 2 of the data field
  int nj,
  //! [in] Dimension 3 of the data field
  int nk,
  //! [in] datyp + nbits + control for XdfDouble/XdfShort/XdfByte
  int data_kind
) ;

#endif
