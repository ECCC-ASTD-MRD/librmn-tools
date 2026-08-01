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
#if !defined(_FST98_PACK_)
#define _FST98_PACK_
//
#include <stdint.h>
//
#include <rmn/mem_range.h>
// import as little as possible from fstd 98 code
#include <rmn/fst98.h>
#include <rmn/fst_missing.h>

typedef void *(*PackFunctionPointer)(
    const void * const unpackedArrayOfFloat,
    void * const packedHeader,
    void * const packedArrayOfInt,
    const int elementCount,
    const int packedTokenBitSize,
    const int offset,
    const int stride,
    const int hasMissing,
    const void * const missingTag,
    void * const min,
    void * const max
);

typedef void *(*UnpackFunctionPointer)(
    void * const unpackedArrayOfFloat,
    const void * const packedHeader,
    const void * const packedArrayOfInt,
    const int elementCount,
    const int packedTokenBitSize,
    const int offset,
    const int stride,
    const int hasMissing,
    const void * const missingTag,
    void * const min,
    void * const max
);

int compact_p_char(
    const void * const unpackedArrayOfBytes,
    void * const packedHeader,
    void * const packedArrayOfInt,
    int intCount,
    int bitSizeOfPackedToken,
    const int offset,
    const int stride
);

int compact_u_char(
    void * const unpackedArrayOfBytes,
    const void * const packedHeader,
    const void * const packedArrayOfInt,
    int intCount,
    int bitSizeOfPackedToken,
    const int offset,
    const int stride
);

int compact_p_short(
    const void * const unpackedArray,
    void * const packedHeader,
    void * const packedArray,
    int intCount,
    const int bitSizeOfPackedToken,
    const int offset,
    const int stride
);

int compact_u_short(
    void * const unpackedArray,
    void * const packedHeader,
    const void * const packedArray,
    int intCount,
    const int bitSizeOfPackedToken,
    const int offset,
    const int stride
);

int compact_p_integer(
    const void * const unpackedArrayOfInt,
    void * const packedHeader,
    void * const packedArrayOfInt,
    int intCount,
    int bitSizeOfPackedToken,
    int offset,
    int stride,
    const int sign
);

int compact_u_integer(
    void * const unpackedArrayOfInt,
    const void * const packedHeader,
    const void * const packedArrayOfInt,
    int intCount,
    int bitSizeOfPackedToken,
    int offset,
    int stride,
    const int sign
);

void * compact_p_float(
    const void * const unpackedArrayOfFloat,
    void * const packedHeader,
    void * const packedArrayOfInt,
    const int elementCount,
    const int packedTokenBitSize,
    const int offset,
    const int stride,
    const int hasMissing,
    const void * const missingTag,
    void * const min,
    void * const max
);

void * compact_u_float(
    void * const unpackedArrayOfFloat,
    const void * const packedHeader,
    const void * const packedArrayOfInt,
    const int elementCount,
    const int packedTokenBitSize,
    const int offset,
    const int stride,
    const int hasMissing,
    const void * const missingTag,
    void * const min,
    void * const max
);

void * compact_p_double(
    const void * const unpackedArrayOfFloat,
    void * const packedHeader,
    void * const packedArrayOfInt,
    const int elementCount,
    const int packedTokenBitSize,
    const int offset,
    const int stride,
    const int hasMissing,
    const void * const missingTag,
    void * const min,
    void * const max
);

void * compact_u_double(
    void * const unpackedArrayOfFloat,
    const void * const packedHeader,
    const void * const packedArrayOfInt,
    const int elementCount,
    const int packedTokenBitSize,
    const int offset,
    const int stride,
    const int hasMissing,
    const void * const missingTag,
    void * const min,
    void * const max
);

// borrow some function prototypes
void c_float_packer_params(int32_t *header_size, int32_t *stream_size, int32_t *p1, int32_t *p2, int32_t npts);
int32_t c_float_packer(float *source, int32_t nbits, int32_t *header, int32_t *stream, int32_t npts);
int32_t c_float_unpacker(float *dest, int32_t *header, int32_t *stream, int32_t npts, int32_t *nbits );

void f77name(ieeepak)(int32_t *IFLD, int32_t *IPK, const int32_t *NI, const int32_t *NJ, const int32_t *NPAK, const int32_t *serpas, const int32_t *mode);

int c_armn_compress32(unsigned char *, float *, int, int, int, int);
int  c_armn_uncompress32(float *fld, unsigned char *zstream, int ni, int nj, int nk, int nchiffres_sign);
int armn_compress(unsigned char *fld, int ni, int nj, int nk, int nbits, int op_code, const int swap_stream);

void resize_int(
    void* restrict dest,        //!< Destination array
    const int dest_size,        //!< Element size of destination array (in bits)
    const void* restrict src,   //!< Source array
    const int src_size,         //!< Element size of source array (in bits)
    const int64_t num_elem      //!< Number of elements to convert
);
void upgrade_size(
    void* dest,             //!< [out] Destination array, into which the items are copied
    const int dest_size,    //!< [in] Size of items in destination array, in bits
    void* src,              //!< [in] Source array, from which the items are copied
    const int src_size,     //!< [in] Size of items in source array, in bits
    const int64_t num_elem, //!< [in] Number of items to copy
    const int is_integer    //!< [in] Whether we are copying integers (or float)
);

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
  //! [in] Data type of elements
  const int in_datyp_ori,
  int *data_kind,
  const int xdf_double,
  const int xdf_short,
  const int xdf_byte,
  const int xdf_stride
) ;

//! XDF version
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
  int data_kind,
//   int datyp,
//   int nbits_in,
  int downgrade_32,
  int xdf_double,
  int xdf_short,
  int xdf_byte,
  int xdf_stride
) ;

#endif
