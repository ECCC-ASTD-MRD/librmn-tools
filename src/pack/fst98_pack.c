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
// legacy packers used for standard files
// encoder derived from fstecr
// decoder derived from fstluk
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <App.h>

#include <rmn/fst98_pack.h>

#include <rmn/tile_encoders.h>

#define Max(x,y) ((x > y) ? x : y)
#define Min(x,y) ((x < y) ? x : y)

// borrowed from armn_compress.h
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

int armn_compress(unsigned char *fld, int ni, int nj, int nk, int nbits, int op_code, const int swap_stream);

// borrowed from packers.h
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

void c_float_packer_params(int32_t *header_size, int32_t *stream_size, int32_t *p1, int32_t *p2, int32_t npts);
int32_t c_float_packer(float *source, int32_t nbits, int32_t *header, int32_t *stream, int32_t npts);
int32_t c_float_unpacker(float *dest, int32_t *header, int32_t *stream, int32_t npts, int32_t *nbits );

// borrowed from primitives/primitives.h
void f77name(ieeepak)(int32_t *IFLD, int32_t *IPK, const int32_t *NI, const int32_t *NJ, const int32_t *NPAK, const int32_t *serpas, const int32_t *mode);

// borrowed from armn_compress_32.h
int c_armn_compress32(unsigned char *, float *, int, int, int, int);
int  c_armn_uncompress32(float *fld, unsigned char *zstream, int ni, int nj, int nk, int nchiffres_sign);

// borrowed from fst98_internal.h
void resize_int(
    void* restrict dest,        //!< Destination array
    const int dest_size,        //!< Element size of destination array (in bits)
    const void* restrict src,   //!< Source array
    const int src_size,         //!< Element size of source array (in bits)
    const int64_t num_elem      //!< Number of elements to convert
);

typedef uint8_t byte ;

// byte to halfword unsigned copy
static void memcpy_8_16(uint16_t *p16, const uint8_t *p8, int nb) {
  for (int i = 0; i < nb; i++) { p16[i] = p8[i]; }
}

// halfword to byte unsigned copy
static void memcpy_16_8(uint8_t *p8, const uint16_t *p16, int nb) {
  for (int i = 0; i < nb; i++) { p8[i] = p16[i]; }
}

// halfword to word unsigned copy
static void memcpy_16_32(uint32_t *p32, const uint16_t *p16, int nbits, int nb) {
  uint16_t mask = ~ (0xffff << nbits);        // keep lower nbits bits only
  for (int i = 0; i < nb; i++) { p32[i] = p16[i] & mask; }
}

// word to halfword unsigned copy
static void memcpy_32_16(uint16_t *p16, const uint32_t * p32, int nbits, int nb) {
  uint32_t mask = ~ (0xffffffff << nbits);    // keep lower nbits bits only
  for (int i = 0; i < nb; i++) { p16[i] = p32[i] & mask; }
}

// double to float copy
static void memcpy_d_f(float * restrict f, const double *restrict d, int nb) {
  for (int i = 0; i < nb; i++) { f[i] = d[i] ; }
}

// be consistent with legacy fstd98 code
#define use_old_signed_pack_unpack_code
// force Little endian for now
#if ! defined(Little_Endian)
#define Little_Endian YES
#endif

// flags to avoid unnecessary warning messages
static uint8_t dejavu[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;
// if nbits > ieee_turbo_threshold, use 32 bits + turbo for encoding reals
static int ieee_turbo_threshold = 16 ;

//! \return range describing encoded data, NULL RANGE in case of error
RANGE(int32_t) fst98_encode(
  //! [in] Field to encode
  const void * const field_in,
  //! [out] encoded field
  RANGE(int32_t) field_out,
  //! [in] Number of bits kept for the elements of the field (npak < 0), packing ratio (npak >= 0)
  int npak,
  //! [in] First dimension of the data field
  int ni,
  //! [in] Second dimension of the data field
  int nj,
  //! [in] Third dimension of the data field
  int nk,
  //! [in] Data type of elements (including flags used to control xdf_double/xdf_short/xdf_byte)
  int datyp_in,
  //! [out] effective data type and nbits
  int *data_kind
) {
  if(base_fst_type(datyp_in) == FST_TYPE_BINARY){    // cancel all features if FST_TYPE_BINARY
    datyp_in = FST_TYPE_BINARY ;
    xdf_double = xdf_short = xdf_byte = 0 ;
  }
  int data_control = datyp_in & 0xFF000000 ;  // keep upper 8 bits
  datyp_in &= 0xFFFFFF ;                      // lower 24 bits ;
  // account for legacy xdf_double / xdf_short / xdf_byte
  int XdfDouble = xdf_double || (data_control & SRC_DOUBLE) ;
  int XdfShort  = xdf_short  || (data_control & SRC_SHORT) ;
  int XdfByte   = xdf_byte   || (data_control & SRC_BYTE);

  nk = Max(1, nk);                          // take care of nk == 0
  const uint32_t *field_u32 = field_in;
  float* field_f = NULL;                    // float version of the data
  uint32_t* field_missing = NULL;           // data with missing values transformed
  int32_t *buffer = NULL ;                  // used for encoding if field_out not large enough
  int nw;                                   // worst case number of 32 bit words needed for encoded stream
  double dmin = 0.0, dmax = 0.0;            // by_product of some encoders
  int nbits = (npak < 0) ? (-npak) : ( Max(1, 32 / Max(1, npak)) );    // npak == 0 or 1 will set nbits to 32

// TODO : use turbo a priori (backtrack later if impractical) ?
  // datyp_in |= FST_TYPE_TURBOPACK ;

  // FST_TYPE_MAGIC: 512+256+32+1 no interference with turbo pack (128) and missing value (64) flags
  int is_magic   = ((datyp_in & FST_TYPE_MAGIC) == FST_TYPE_MAGIC) ;
//   if(is_magic) goto fail ;         // TODO : disallow FST_TYPE_MAGIC ?

  int is_missing = datyp_in & FSTD_MISSING_FLAG;      // flag : missing value feature is requested
  int is_turbo   = datyp_in & FST_TYPE_TURBOPACK;     // flag : turbo packing activated
  int in_datyp   = base_fst_type(datyp_in);           // suppress flags, only retain base type

// TODO: data type 6 with nbits <= 16 automatically activates turbo
  if((in_datyp == FST_TYPE_REAL) && (nbits <= 16) && (is_turbo == 0)){
fprintf(stderr, "DEBUG :  FST_TYPE_REAL && nbits <= 16  turbo activated, datyp_in = %d\n", datyp_in) ;
    is_turbo = FST_TYPE_TURBOPACK ;                     // activate turbo
    datyp_in = FST_TYPE_REAL | is_turbo | is_missing ;  // keep flags
  }

// TODO: data type 1 with nbits <= 16 becomes data type 6 with same nbits
  if(in_datyp == FST_TYPE_REAL_OLD_QUANT && nbits <= 16){
fprintf(stderr, "DEBUG :  FST_TYPE_REAL_OLD_QUANT && nbits <= 16  > FST_TYPE_REAL, datyp_in = %d\n", datyp_in) ;
    in_datyp = FST_TYPE_REAL ;
    datyp_in = FST_TYPE_REAL | is_turbo | is_missing ;  // keep flags
  }

// TODO : real type with nbits > ieee_turbo_threshold ====> type 5 + turbo
  if( (is_type_real(in_datyp)) && (nbits > ieee_turbo_threshold) && is_turbo) {
    nbits += 9 ;                                             // add 9 to bit count
    nbits = (nbits > 32) ? 32 : nbits ;                      // at most 32 bits
    in_datyp = FST_TYPE_REAL_IEEE ;                          // set data type to IEEE (6)
    is_turbo = FST_TYPE_TURBOPACK ;                          // force turbo
fprintf(stderr, "DEBUG :  type 1, 5 or 6 with nbits > %d ====> type 5 + turbo (%d bits), datyp_in = %d\n", ieee_turbo_threshold, nbits, datyp_in) ;
    datyp_in = FST_TYPE_REAL_IEEE | is_turbo | is_missing ;  // keep is_missing if it was present
  }

  int datyp = is_magic ? 1 : in_datyp;                // base data type, is_magic means source array is double (type 1 + XdfDouble)
//
  int local_buffer = 0 ;
  int header_size, stream_size, p1out, p2out;
  int IEEE_64 = 0;                            // 64 bit IEEE (type 5 or 8) flag

  if (datyp == FST_TYPE_BINARY) { is_missing = is_turbo = 0 ; }  // missing and turbo are meaningless for binary type

  if (datyp == FST_TYPE_COMPLEX) {
    if (is_missing || is_turbo) {
      if (! dejavu[5]) {
        Lib_Log(APP_LIBFST, APP_WARNING, "%s: compression and/or missing values not supported for complex data, type %d reset to %d (complex)\n",
            __func__, datyp_in, FST_TYPE_COMPLEX);
        dejavu[5] = 1;
      }
      is_missing = is_turbo = 0;              // missing values and turbo compression not supported for complex type
    }
  }

// is_magic means source array is double
  PackFunctionPointer packfunc = ((XdfDouble) || (is_magic)) ? compact_p_double : compact_p_float;

  if (base_fst_type(datyp) == FST_TYPE_REAL_IEEE && nbits < 16) {
    Lib_Log(APP_LIBFST, APP_ERROR, "%s: a truncated IEEE float with less than 16 bits is not allowed\n",
            __func__);
    goto fail ;
  }

  if ( (datyp_in == (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK)) && (nbits > 32) ) {
    if (! dejavu[4]) {
      Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported for IEEE when nbits > 32, "
              "data type FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK (%d) reset to FST_TYPE_REAL_IEEE (%d) (IEEE)\n", __func__,
              FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK, FST_TYPE_REAL_IEEE);
      dejavu[4] = 1;
    }
    datyp = FST_TYPE_REAL_IEEE;
    is_turbo = 0 ;      // extra compression not supported
  }

  if (is_turbo && (nk > 1)) {
    if (! dejavu[3]) {
      Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported for 3D data. It will be disabled.\n", __func__);
      dejavu[3] = 1;
    }
    is_turbo = 0 ;                       // cancel turbo compression
  }

  if ( (datyp == FST_TYPE_REAL_OLD_QUANT) && ((nbits == 31) || (nbits == 32)) ) {
    // R31/R32 to E32 automatic conversion, turbo packing and missing remain applicable to FST_TYPE_REAL_IEEE
    datyp = FST_TYPE_REAL_IEEE;
    nbits = 32;                          // bump nbits to 32
  }

  // flag 64 bit IEEE (type 5 or 8)
  if ( (is_type_real(datyp))    && (nbits > 32) ) IEEE_64 = 1;        // 64 bits real IEEE
  if ( (is_type_complex(datyp)) && (nbits > 32) ) IEEE_64 = 1;        // 64 bits complex IEEE
  if(IEEE_64){
    nbits = 64 ;
//     XdfDouble = 0 ;
  }
  if ((npak == 0) || (npak == 1)) { datyp = FST_TYPE_BINARY; }        // no compaction, nbits is already 32

  // fudge field if missing value feature is used, 
  int sizefactor = 4;
  if (XdfByte)  sizefactor = 1;                  // source is a byte array (8 bit integer values)
  if (XdfShort) sizefactor = 2;                  // source is a halfword array (16 bit integer values)
  if (XdfDouble || IEEE_64) sizefactor = 8;      // source is a double array (64 bit values)
  // put appropriate values into field_missing after allocating it
  if (is_missing) {
    field_missing = malloc(ni*nj*nk * sizefactor);       // allocate temporary field for missing values flagging
    if(field_missing == NULL) goto fail ;
    if (EncodeMissingValue(field_missing, field_in, ni*nj*nk, datyp, sizefactor*8, nbits) > 0) {
      field_u32 = field_missing;
    }else{
      field_u32 = field_in;
      Lib_Log(APP_LIBFST, APP_INFO, "%s: NO missing value, data type reset to %d\n", __func__, datyp);
      is_missing = 0;      // no missing value detected, cancel missing data flag
    }
  }

  // handle double real / complex type
  if ( (is_type_real(datyp) || is_type_complex(datyp)) && (is_missing == 0) ) {
    if (XdfDouble || IEEE_64) {
      int _nk = is_type_complex(datyp) ? (2 * nk) : nk ;
      if (nbits <= 32) {                // convert from double to float if nbits not larger than 32
        field_f = malloc(ni * nj * _nk * sizeof(float));
//           const double * const field_d = field_in;
        memcpy_d_f(field_f, (const double *)field_in, ni*nj*_nk) ;
        packfunc = &compact_p_float;                    // will pack from floats
        field_u32 = (uint32_t*)field_f;
      }else if (nbits != 64) {
        if (! dejavu[2]) {
          Lib_Log(APP_LIBFST, APP_WARNING, "%s: Requested %d packed bits for 64-bit reals, but we can only do"
                  " 64 or less than 32. Will use 64 bits.\n", __func__, nbits);
          dejavu[2] = 1;
        }
        nbits = 64;
      }
    }
  }

  if (is_type_real(datyp) && nbits > 32) {                          // floating point and more than 32 bits
    datyp = FST_TYPE_REAL_IEEE;                                     // force 64 bits IEEE
    nbits = 64;
  }

  if (datyp == FST_TYPE_REAL) {                                     // type F, new float quantification
    if (nbits > 24) {
      if (! dejavu[0]) {
        Lib_Log(APP_LIBFST, APP_INFO, "%s: floats with nbits > 24, using E32 instead of F%2d\n", __func__, nbits);
        dejavu[0] = 1;
      }
      datyp = FST_TYPE_REAL_IEEE ;                                  // keep turbo and/or missing flag
      nbits = 32;
    }
    else if (nbits > 16) {
      if (! dejavu[1]) {
        Lib_Log(APP_LIBFST, APP_INFO, "%s: nbits > 16, using R%2d instead of F%2d\n", __func__, nbits, nbits);
        dejavu[1] = 1;
      }
      datyp = FST_TYPE_REAL_OLD_QUANT;     // type F cannot use more than 16 bits, revert to type R
      is_turbo = 0 ;                       // cancel turbo compression
    }
  }

  // cancel turbo compression if nbits > 16 (except for IEEE reals)
  if ( (nbits > 16) && (datyp != FST_TYPE_REAL_IEEE) && (datyp < 16) ) is_turbo = 0 ;
  // if nbits <= 16 and turbo compression is requested, use FST_TYPE_REAL instead
  if( (nbits <= 16) && (datyp == FST_TYPE_REAL_OLD_QUANT) && is_turbo ){
    datyp = FST_TYPE_REAL ;                    // replace base type FST_TYPE_REAL_OLD_QUANT with FST_TYPE_REAL
  }
// ======= preliminary evaluation of space needed for encoded data (worst case) =======
  switch (datyp) {
    case FST_TYPE_REAL:                                           // float, new style
      c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni*nj*nk);
      nw = ((header_size + stream_size) * 8 + 31) / 32;
      break;

    case FST_TYPE_COMPLEX:                                        // IEEE 32/64 bits
      nw = 2 * ((ni*nj*nk * 64) / 32);
      break;

    default:
      nw = (ni*nj*nk * nbits + 31) / 32 + 32;
      break;
  }
  nw += 32 ;  // nw = estimate of worst case encoded length
  if(IEEE_64){
    nw = 2 * ni*nj*nk ;
    if(datyp == FST_TYPE_COMPLEX) nw *= 2 ;
// fprintf(stderr, "DEBUG : nw = %d, datyp = %d, nbits = %d\n", nw, datyp, nbits) ;
  }

  buffer = NULL ;
  if(VALID_RANGE(field_out)){      // is field_out valid and large enough for encoded stream ?
    if( nw <= RANGE_ITEMS(field_out) ) buffer = (int32_t *) RANGE_BOT(field_out) ;    // large enough, use field_out
  }
  local_buffer = (buffer == NULL) ;
  if(local_buffer){ buffer = (int32_t *) malloc(nw * sizeof(int32_t)); }      // need to allocate buffer
  if(buffer == NULL) goto fail ;
  if(local_buffer) fprintf(stderr,"DEBUG : need %d words, have %ld, allocated buffer with size %d words\n", nw, RANGE_ITEMS(field_out), nw);

// TODO : handle 64 bit straight IEEE (IEEE_64). add endian swap ?
  if(IEEE_64){
// double *buf64 = (double *)field_u32 ;
// fprintf(stderr, "DEBUG : datyp = %d, buf64 = %f %f %f\n", datyp, buf64[0], buf64[nw/4-1], buf64[nw/2-1]);
#if defined(Little_Endian)
    uint64_t *bui64 = (uint64_t *)field_u32 ;
    uint64_t *buo64 = (uint64_t *)buffer ;
    for(int i = 0 ; i<nw/2 ; i++) { buo64[i] = (bui64[i] >> 32) | (bui64[i] << 32) ; } ;  // swap 32/32
#else
    for(int i = 0 ; i<nw ; i++) { buffer[i] = field_u32[i] ; } ;                          // copy 64
#endif
    goto end ;
  }

redo_switch_datyp:
  switch (datyp) {

    // transparent bit stream data, nbits per item
    case FST_TYPE_BINARY:
      nw = (ni*nj*nk * nbits + 31) / 32 ;
      for (int i = 0; i < nw; i++) { buffer[i] = field_u32[i]; }
      is_turbo = 0;
      break;                      // nw = actual length of "encoded" stream

    // floating point, old style packers
    case FST_TYPE_REAL_OLD_QUANT: {
      double tempfloat = 99999.0;
      // straight quantifier, no turbo, pack with offset 24 (120 bit header)
      packfunc(field_u32, buffer, buffer+3, ni*nj*nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
      nw = (ni*nj*nk * nbits + 96 + 24 + 31) / 32;
      is_turbo = 0;
      break;                      // nw = actual length of "encoded" stream
    }

    // floating point, last gen style packers and encoders
    case FST_TYPE_REAL+16:
fprintf(stderr,"FST_TYPE_REAL+16 : is_turbo = %d\n", is_turbo) ;
      break;

    // floating point, new packers
    case FST_TYPE_REAL:
      nw = ((header_size + stream_size) * 8 + 31) / 32;      // length if turbo packing not used
      header_size /= sizeof(int32_t);
      if (is_turbo && (nbits <= 16)) {    // use turbo compression scheme
        c_float_packer((float *)field_u32, nbits, buffer + 1, buffer + 1 + header_size, ni*nj*nk);
        int compressed_lng = armn_compress((byte *)(buffer+1+header_size), ni, nj, nk, nbits, 1, 1);
        if (compressed_lng < 0) {
          is_turbo = 0 ;
          c_float_packer((float *)field_u32, nbits, (int32_t *)buffer, (int32_t *)(buffer+header_size), ni*nj*nk);
        }else{
          int nbytes = 16 + (header_size*4) + compressed_lng;
          buffer[0] = nw = (nbytes * 8 + 31) / 32;
          nw ++ ;    // turbo extra header, bump nw ;
        }
      }else{                // no turbo compression
        c_float_packer((float *)field_u32, nbits, (int32_t *)buffer, (int32_t *)(buffer+header_size), ni*nj*nk);
      }
      break;

    // integers, short integers or bytes (unsigned), last gen encoders
    case FST_TYPE_UNSIGNED+16:{
        uint32_t *d32 = (uint32_t *)buffer ;
        const void *source = (XdfShort || XdfByte) ? buffer : (const void *)field_u32 ;   // 32 bit source for packing
        if (XdfShort) {               // 16 bits to 32 bits expansion
          nbits = Min(16, nbits);     // at most 16 bits
          uint16_t *s16 = (uint16_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { d32[i] = s16[i] ; } ;
        } else if (XdfByte) {         // 8 bits to 32 bits expansion
          nbits = Min(8, nbits);      // at most 8 bits
          uint8_t *s8 = (uint8_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { d32[i] = s8[i] ; } ;
        }else{                        // 32 bits to 32 bits
          memcpy(buffer, field_u32, ni*nj*nk*sizeof(uint32_t)) ;
        }
        bitstream stream ;
        InitStream(&stream, buffer, nw*sizeof(uint32_t), BIT_FULL_INIT|BIT_INSERT|SET_BIG_ENDIAN) ;
        int nwords = encode_block(&stream, (int32_t *)source, ni, ni, nj, 8, ENCODE_DRY_RUN);
        nwords = (nwords+31)/32 ;
//      int encode_block(bitstream *s_in, int32_t *block, int lnis, int ni, int nj, int tsize, int options);
        compact_p_integer(source, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 0);
        nw = ((ni*nj*nk * nbits) + 31) / 32;                 // recompute nw using possibly revised nbits
fprintf(stderr,"FST_TYPE_UNSIGNED+16 : is_turbo = %d, datyp = %d, nw = %d, nwords = %d\n", is_turbo, datyp, nw, nwords) ;
      }
      break;

    // integers, short integers or bytes (unsigned)
    case FST_TYPE_UNSIGNED:
//       if(nbits > 16) is_turbo = 0 ;
      if (is_turbo) {
        const int offset = 1;
        if (XdfShort) {               // 16 bits to 16 bits copy
          nbits = Min(16, nbits);     // at most 16 bits
          memcpy((int16_t *)(buffer + offset), field_u32, ni*nj*nk * 2);
        } else if (XdfByte) {         // 8 bits to 16 bits expansion
          nbits = Min(8, nbits);      // at most 8 bits
          memcpy_8_16((uint16_t *)(buffer + offset), (uint8_t *)field_u32, ni*nj*nk);
        }else{                        // 32 bits to 16 bits truncation
          memcpy_32_16((uint16_t *)(buffer + offset), (const uint32_t *)field_u32, nbits, ni*nj*nk);
        }
        int compressed_lng = armn_compress((byte *)(buffer + offset), ni, nj, nk, nbits, 1, 0);
        if (compressed_lng < 0) {     // no gain from turbo, repack
          datyp = FST_TYPE_UNSIGNED;
          is_turbo = 0 ;
          goto redo_switch_datyp ;
//           compact_p_integer(field_u32, (void *) NULL, buffer /*+ offset*/, ni*nj*nk, nbits, 0, xdf_stride, 0);
//           nw = (ni*nj*nk * nbits + 31) / 32 ;              // recompute nw using possibly revised nbits
        }else{
          int nbytes = 4 + compressed_lng;
          buffer[0] = nw = (nbytes * 8 + 31) / 32;
          nw ++ ;    // turbo header, bump nw ;
        }
      }else{    // straight packing, no turbo
        if (XdfShort) {
          nbits = Min(16, nbits);    // at most 16 bits
          compact_p_short(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride);
        } else if (XdfByte) {
          nbits = Min(8, nbits);     // at most 8 bits
          compact_p_char(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride);
        }else{
          compact_p_integer(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 0);
        }
        nw = ((ni*nj*nk * nbits) + 31) / 32;                 // recompute nw using possibly revised nbits
      }
      xdf_short = xdf_byte = 0 ;
      break;                      // nw = actual length of "encoded" stream

    // integers, short integers or bytes (signed), last gen encoders
    case FST_TYPE_SIGNED+16:{
fprintf(stderr,"FST_TYPE_SIGNED+16 : is_turbo = %d, nbits = %d\n", is_turbo, nbits) ; nw = (ni*nj*nk*nbits + 31) / 32 ;
        int32_t *d32 = (int32_t *)buffer ;
        const void *source = (XdfShort || XdfByte) ? buffer : (const void *)field_u32 ;   // 32 bit source for packing
        if (XdfShort) {               // 16 bits to 32 bits expansion
          nbits = Min(16, nbits);     // at most 16 bits
          int16_t *s16 = (int16_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { d32[i] = s16[i] ; } ;
        } else if (XdfByte) {         // 8 bits to 32 bits expansion
          nbits = Min(8, nbits);      // at most 8 bits
          int8_t *s8 = (int8_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { d32[i] = s8[i] ; } ;
        }else{                        // 32 bits to 32 bits
          memcpy(buffer, field_u32, ni*nj*nk*sizeof(int32_t)) ;
        }
        compact_p_integer(source, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 1);
        nw = ((ni*nj*nk * nbits) + 31) / 32;                 // recompute nw using possibly revised nbits
      }
      break;

    // integers, short integers or bytes (signed)
    case FST_TYPE_SIGNED:
      if (is_turbo) {
        if (! dejavu[6]) {
          Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported for signed integers, data type reset to FST_TYPE_SIGNED (%d)\n",
                  __func__, is_missing | FST_TYPE_SIGNED);
          dejavu[6] = 1;
        }
        is_turbo = 0;
      }
#ifdef use_old_signed_pack_unpack_code
      if (XdfShort || XdfByte) {
//                 int32_t* field3 = (int *)malloc(ni*nj*nk*sizeof(int));
        int32_t field3[ni*nj*nk] ;
        if (XdfShort){
          int16_t *s_field = (int16_t *)field_u32;
          for (int i = 0; i < ni*nj*nk;i++) { field3[i] = s_field[i]; };    // expand to int32_t
          nbits = Min(16, nbits);    // at most 16 bits
        }else if (XdfByte){
          int8_t  *b_field = (int8_t  *)field_u32;
          for (int i = 0; i < ni*nj*nk;i++) { field3[i] = b_field[i]; };    // expand to int32_t
          nbits = Min(8, nbits);     // at most 8 bits
        }
        compact_p_integer(field3, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 1);
//                 free(field3); 
      }else{
        compact_p_integer(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 1);
      }
      nw = ((ni*nj*nk * nbits) + 31) / 32;
      xdf_short = xdf_byte = 0 ;
#else
#error "use_old_signed_pack_unpack_code not defined"
#endif
      break;

    // floats with max relative error, last gen encoders
    case FST_TYPE_REAL_IEEE+16:
fprintf(stderr,"FST_TYPE_REAL_IEEE+16 : is_turbo = %d\n", is_turbo) ;
      break;

    // IEEE and IEEE complex representation
    case FST_TYPE_REAL_IEEE:
    case FST_TYPE_COMPLEX: {
      int32_t f_ni = (int32_t) ni;
      int32_t f_njnk = nj * nk;
      int32_t f_zero = 0;
      int32_t f_one = 1;
      int32_t f_minus_nbits = (- nbits);
      if ( (datyp == FST_TYPE_REAL_IEEE) && is_turbo) {    // use turbo compression scheme for FST_TYPE_REAL_IEEE
        int compressed_lng = c_armn_compress32((byte *)&(buffer[1]), (float *)field_u32, ni, nj, nk, nbits);
        if (compressed_lng < 0) {     // no gain with turbo compression
          is_turbo = 0 ;
          f77name(ieeepak)((int32_t *)field_u32, (int32_t *)buffer, &f_ni, &f_njnk, &f_minus_nbits, &f_zero, &f_one);
          nw = (f_ni*f_njnk * nbits + 31) / 32 ;
      }else{
          int nbytes = 16 + compressed_lng;
          buffer[0] = nw = (nbytes * 8 + 31) / 32;
          nw ++ ;    // turbo used, bump nw ;
        }
      }else{
        if (datyp == FST_TYPE_COMPLEX) f_ni = f_ni * 2;
        f77name(ieeepak)((int32_t*)field_u32, (int32_t *)buffer, &f_ni, &f_njnk, &f_minus_nbits, &f_zero, &f_one);
        nw = (f_ni*f_njnk * nbits + 31) / 32 ;
      }
      break;
    }

    // character data, R4A items (4 chars in an unsigned integer)
    case FST_TYPE_CHAR:
      if (is_turbo) {
        if (! dejavu[7]) {
          Lib_Log(APP_LIBFST,APP_WARNING, "%s: extra compression not available for characters, data type reset to FST_TYPE_CHAR (%d)\n",
                  __func__, FST_TYPE_CHAR);
          dejavu[7] = 1;
        }
        is_turbo = 0;
      }
      int nc4 = (ni*nj*nk + 3) / 4;
      compact_p_integer(field_u32, (void *) NULL, buffer, nc4, 32, 0, xdf_stride, 0);
      nbits = 8;
      nw = ((ni*nj*nk * nbits) + 32 - 1) / 32;                 // nw = actual length of "encoded" stream (use possibly revised nbits)
      break;

    // character string
    case FST_TYPE_STRING:
      if (is_turbo) {
        if (! dejavu[8]) {
          Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not available for strings, data type reset to FST_TYPE_STRING (%d)\n",
                  __func__, FST_TYPE_STRING);
          dejavu[8] = 1;
        }
        is_turbo = 0;
    }
      compact_p_char(field_u32, (void *) NULL, buffer, ni*nj*nk, 8, 0, xdf_stride);
      nw = (ni*nj*nk * 8 + 31) / 32 ;
      break;

    default:
        Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
        goto fail ;
  } // end switch

end:
  // free temporary arrays if they were used
  if (field_f       != NULL) free(field_f);
  if (field_missing != NULL) free(field_missing);

  xdf_byte = xdf_short = xdf_double = 0 ;               // reset other than 32 bits flags
  datyp = datyp | is_missing | is_turbo ;               // restore missing and turbo flags
  *data_kind = datyp | (nbits << 16) ;                  // compound information for decoder
  return (RANGE(int32_t)) { buffer, (buffer + nw) } ;   // RANGE_NULL(int32_t) if buffer is NULL and nw is 0

fail :
  // cleanup before failing
  if(buffer && local_buffer) free(buffer) ;   // free buffer if it was allocated locally
  buffer = NULL ;
  nw = nbits = 0 ;
  datyp = is_missing = is_turbo = 0 ;
  goto end ;
}

//! \return token size
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
) {
  int data_control = data_kind & 0xFF000000 ;  // upper 8 bits
  data_kind = data_kind & 0xFFFFFF ;           // keep lower 24 bits
  // account for legacy xdf_double / xdf_short / xdf_byte / downgrade_32
  int XdfDouble   = xdf_double   || (data_control & DST_DOUBLE) ;     // output will be 64 bit doubles
  int XdfShort    = xdf_short    || (data_control & DST_SHORT) ;      // output will be 16 bit
  int XdfByte     = xdf_byte     || (data_control & DST_BYTE);        // output will be 8 bit
  int Downgrade32 = downgrade_32 || (data_control & DST_WORD);        // output will be 32 bit floats
  uint32_t *field = data_out;
  int ier = 0 ;
  int datyp = data_kind & 0xFFFF ;
  int is_turbo = (datyp & FST_TYPE_TURBOPACK) ;
  int nbits_in = (data_kind >> 16) & 0xFF ;

  if(base_fst_type(datyp) == FST_TYPE_BINARY){    // cancel all features if FST_TYPE_BINARY, only keep nbits
    datyp = FST_TYPE_BINARY ;
    xdf_double = xdf_short = xdf_byte = 0 ;
    XdfDouble = XdfShort = XdfByte = Downgrade32 = 0 ;
  }
  // Get missing data flag
  int is_missing = datyp & FSTD_MISSING_FLAG;
  // Suppress missing data flag
  datyp = datyp & ~FSTD_MISSING_FLAG;
//     int xdf_datatyp = datyp;

  UnpackFunctionPointer packfunc = XdfDouble ? &compact_u_double : &compact_u_float;
  double dmin=0.0, dmax=0.0;

  int nelm = ni*nj*nk ;
  uint32_t *buf = (uint32_t *) data_in ;
  if (datyp == 8) nelm *= 2;    // complex data, double number of values
#if 0
    // TODO : take care of IEEE case, nbits > 16 O.K. with turbo
    // nbits > 16, remove FST_TYPE_TURBOPACK, replace FST_TYPE_REAL with FST_TYPE_REAL_OLD_QUANT
    if(nbits_in > 16){
      if(base_fst_type(datyp) == FST_TYPE_REAL || base_fst_type(datyp) == FST_TYPE_REAL_OLD_QUANT){
        datyp &+ (~FST_TYPE_TURBOPACK) ;
        datyp = ((datyp >> 6) << 6) | FST_TYPE_REAL_OLD_QUANT ;
      }
    }
//   turbo mode not supported for FST_TYPE_REAL_OLD_QUANT
    if( (nbits_in <= 16) && (base_fst_type(datyp) == FST_TYPE_REAL_OLD_QUANT) && (datyp & FST_TYPE_TURBOPACK) ){
      datyp = ((datyp >> 6) << 6) | FST_TYPE_REAL ; // replace base type FST_TYPE_REAL_OLD_QUANT with FST_TYPE_REAL
    }
    if( (base_fst_type(datyp) == FST_TYPE_REAL_OLD_QUANT ) && (datyp & FST_TYPE_TURBOPACK) ){
      datyp &= (~FST_TYPE_TURBOPACK) ;
    }
#endif
// TODO : handle 64 bit straight IEEE correctly
  ier = nbits_in ;
  switch (datyp) {
    case FST_TYPE_BINARY: {            // Raw binary
      int lngw = ((nelm * nbits_in) + 32 - 1) / 32;
      for (int i = 0; i < lngw; i++) {
          field[i] = buf[i];
      }
      break;
    }

    case FST_TYPE_REAL_OLD_QUANT: {          // Floating Point, old style packers
      double tempfloat = 99999.0;
      packfunc(field, buf, buf + 3, nelm, nbits_in, 24, xdf_stride, 0, &tempfloat, &dmin , &dmax);
      break;
    }

    // integers, short integers or bytes (unsigned), last gen encoders
    case FST_TYPE_UNSIGNED+16:
    case (FST_TYPE_UNSIGNED+16) | FST_TYPE_TURBOPACK:
fprintf(stderr,"FST_TYPE_UNSIGNED+16 : is_turbo = %d\n", is_turbo) ;
      bitstream stream ;
//       int decoded ;
      InitStream(&stream, buf, nelm*sizeof(uint32_t), BIT_FULL_INIT|BIT_XTRACT|SET_BIG_ENDIAN) ;
//    int decode_block(bitstream *s_in, int32_t *block, int lnid, int ni, int nj, int tsize);
      if(XdfShort || XdfByte){
        uint32_t t[nelm] ;
//         decoded = decode_block(&stream, (int32_t *)field, ni, ni, nj, 8) ;
        ier = compact_u_integer(t, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 0);
        if (XdfShort) {
          uint16_t *d16 = (uint16_t *)field ;
          for(int i=0 ; i<nelm ; i++){ d16[i] = t[i] ; } ;
        }else if(XdfByte) {
          uint8_t *d8 = (uint8_t *)field ;
          for(int i=0 ; i<nelm ; i++){ d8[i] = t[i] ; } ;
        }
      }else{
//         decoded = decode_block(&stream, (int32_t *)field, ni, ni, nj, 8) ;
        ier = compact_u_integer(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 0);
      }
      break;

    case FST_TYPE_UNSIGNED:                // Integers, short integers or bytes (unsigned)
    case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK: {
      int offset = is_type_turbopack(datyp) ? 1 : 0;
      if (XdfShort) {
        if (is_type_turbopack(datyp)) {
          int nbytes = armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
          memcpy(field, buf + offset, nbytes);
        }else{
          ier = compact_u_short(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride);
        }
      } else if(XdfByte) {
        if (is_type_turbopack(datyp)) {
          armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
          memcpy_16_8((uint8_t *)field, (uint16_t *)(buf + offset), nelm);
        }else{
          ier = compact_u_char(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride);
        }
      }else{
        if (is_type_turbopack(datyp)) {
          armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
          memcpy_16_32((uint32_t *)field, (uint16_t *)(buf + offset), nbits_in, nelm);
        }else{
          ier = compact_u_integer(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 0);
        }
      }
      break;
    }

    // integers, short integers or bytes (signed), last gen encoders
    case FST_TYPE_SIGNED+16:
    case (FST_TYPE_SIGNED+16) | FST_TYPE_TURBOPACK:
fprintf(stderr,"FST_TYPE_SIGNED+16 : is_turbo = %d\n", is_turbo) ;
      if(XdfShort || XdfByte){
        int32_t t[nelm] ;
        ier = compact_u_integer(t, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
        if (XdfShort) {
          int16_t *d16 = (int16_t *)field ;
          for(int i=0 ; i<nelm ; i++){ d16[i] = t[i] ; } ;
        }else if(XdfByte) {
          int8_t *d8 = (int8_t *)field ;
          for(int i=0 ; i<nelm ; i++){ d8[i] = t[i] ; } ;
        }
      }else{
        ier = compact_u_integer(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
      }
      break;

    case FST_TYPE_SIGNED: {                // Integers, short integers or bytes (signed)
#ifdef use_old_signed_pack_unpack_code
      int32_t *field_out;
      if (XdfShort || XdfByte || XdfDouble) {                // need temporary array to unpack
        field_out = malloc(nelm * sizeof(int));
      }else{
        field_out = (int32_t *)field;
      }
      // unpack into field_out
      ier = compact_u_integer(field_out, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
      if (XdfShort) {                           // copy into "short" destination
        short *s_field_out = (short *)field;
        for (int i = 0; i < nelm; i++) {
          s_field_out[i] = field_out[i];
        }
      }
      else if (XdfByte) {                       // copy into "byte" destination
        signed char *b_field_out = (signed char *)field;
        for (int i = 0; i < nelm; i++) {
          b_field_out[i] = field_out[i];
          }
      }
      if (field_out != (int32_t*)field) free(field_out); // needed temporary array
#else
#error "use_old_signed_pack_unpack_code not defined"
#endif
      break;
    }

    // floats with max relative error, last gen encoders
    case FST_TYPE_REAL_IEEE+16:
    case (FST_TYPE_REAL_IEEE+16) | FST_TYPE_TURBOPACK:
fprintf(stderr,"FST_TYPE_REAL_IEEE+16 : is_turbo = %d\n", is_turbo) ;
      break;

    case FST_TYPE_REAL_IEEE:                // IEEE representation
    case FST_TYPE_COMPLEX: {
// fprintf(stderr, "FST_TYPE_IEEE : nelm = %d, nbits_in = %d\n", nelm, nbits_in) ;
      register int32_t temp32, *src, *dest;
      if ((downgrade_32 || Downgrade32) && (nbits_in == 64)) {
        // Downgrade 64 bit to 32 bit
        float * ptr_real = (float *) field;
        double * ptr_double = (double *) buf;
#if defined(Little_Endian)
        src = (int32_t *) buf;
        dest = (int32_t *) buf;
        for (int i = 0; i < nelm; i++) {    // 32/32 endian swap
          temp32 = *src++;
          *dest++ = *src++;
          *dest++ = temp32;
        }
#endif
        for (int i = 0; i < nelm; i++) {
          *ptr_real++ = *ptr_double++;
        }
fprintf(stderr, "FST_TYPE_IEEE : downgrading double -> float\n") ;
      }else{
        int32_t f_one = 1;
        int32_t f_zero = 0;
        int32_t f_mode = 2;
        int f_minus_nbits = (-nbits_in);
        if(nbits_in == 64){
          double *ptr_in = (double *)buf, *ptr_out = (double *)field;
#if defined(Little_Endian)
          uint64_t *t64 = (uint64_t *)buf ;
          for (int i = 0; i < nelm; i++) { t64[i] = (t64[i] >> 32) | (t64[i] << 32) ; }   // 32/32 endian swap
#endif
fprintf(stderr, "FST_TYPE_IEEE :copying double -> double\n") ;
          for (int i = 0; i < nelm; i++) { ptr_out[i] = ptr_in[i] ; }
        }else{
fprintf(stderr, "FST_TYPE_IEEE : calling ieeepak, f_minus_nbits = %d\n", f_minus_nbits) ;
          f77name(ieeepak)((int32_t *)field, (int32_t *)buf, &nelm, &f_one, &f_minus_nbits, &f_zero, &f_mode);
        }
      }

      break;
    }

    case FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK: {
      // IEEE Floating point direct packers
      c_armn_uncompress32((float *)field, (byte *)(buf + 1), ni, nj, nk, nbits_in);
      break;
    }

    // floating point, last gen style packers and encoders
    case FST_TYPE_REAL+16:
    case (FST_TYPE_REAL+16) | FST_TYPE_TURBOPACK:
fprintf(stderr,"FST_TYPE_REAL+16 : is_turbo = %d\n", is_turbo) ;
      break;

    case FST_TYPE_REAL:
    case FST_TYPE_REAL | FST_TYPE_TURBOPACK: {
      // Floating point, new packers
      // printf("Debug+ fstluk - Floating point, new packers (6, 134)\n");
      int nbits, header_size, stream_size, p1out, p2out;
      c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni*nj*nk);
      header_size /= sizeof(int32_t);
      if (is_type_turbopack(datyp)) {
        armn_compress((byte *)(buf + 1 + header_size), ni, nj, nk, nbits_in, 2, 1);
        c_float_unpacker((float *)field, (int32_t *)(buf + 1), (int32_t *)(buf + 1 + header_size), nelm, &nbits);
      }else{
        c_float_unpacker((float *)field, (int32_t *)buf, (int32_t *)(buf + header_size), nelm, &nbits);
      }
      break;
    }

    case FST_TYPE_CHAR: {
      // Character data, R4A style (4 chars in a 32 bit integer)
      int nc = (nelm + 3) / 4;
      ier = compact_u_integer(field, (void *) NULL, buf, nc, 32, 0, xdf_stride, 0);
      break;
    }

    case FST_TYPE_STRING:
      // Character string
      ier = compact_u_char(field, (void *) NULL, buf, nelm, 8, 0, xdf_stride);
      break;

    default:
      Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
      ier = -1;
      goto end ;
  } // switch

  if (is_missing) {
    // Replace "missing" data points with the appropriate values given the type of data (int/float)
    // if nbits = 64 and IEEE , set XdfDouble
    if ((datyp & 0xF) == 5 && nbits_in == 64 ) XdfDouble = 1;
    int sz=(XdfDouble?64:(XdfShort?16:(XdfByte?8:32)));
    DecodeMissingValue(field , (ni) * (nj) * (nk) , datyp & 0x3F, sz);
}

  // Upgrade size, if necessary
  if (XdfDouble && (nbits_in != 64)) {
    const int base_type = base_fst_type(datyp);
    if (base_type == FST_TYPE_REAL_IEEE || base_type == FST_TYPE_REAL) {         // float -> double copy
      float f[nelm], *ff = (float *)field;
      memcpy(f, field, nelm * sizeof(float));
fprintf(stderr, "decoder : XdfDouble upgrade_size, nelm = %d, f[0] = %f, ff[0] = %f\n", nelm, f[0], ff[0]);
      upgrade_size(field, 64, f, 32, nelm, 0);
    }
//     else if (base_type == FST_TYPE_SIGNED || base_type == FST_TYPE_UNSIGNED) {   // int -> long copy
//       int32_t x[nelm];
//       memcpy(x, field, nelm * sizeof(int32_t));
//       resize_int(field, 64, x, 32, nelm);
//     }
  }
end:
  return ier ;   // token size (normally > 0)
}

int32_t fst98_codec(zmap *map, zmap_block block, zmap_stream stream, int encode){
  struct{
    uint32_t nbits ;         // item size
    uint32_t datyp ;         // item type
    uint32_t data_control ;  // SRC_DOUBLE/SRC_SHORT/SRC_BYTE/DST_DOUBLE/DST_SHORT/DST_BYTE
    uint32_t dummy ;         // not used for now
  } fst98_codec_args;
  CT_ASSERT(sizeof(fst98_codec_args) == CODEC_ARGS_SIZE, "sizeof(fst98_codec_args) != CODEC_ARGS_SIZE") ;
  int32_t status = 0 ;

  memcpy(&fst98_codec_args, ZMAP_CODEC_ARGS(map), CODEC_ARGS_SIZE) ;  // get codec arguments
  if(encode){
    int ni = map->fhead.gni ;
    int nj = map->fhead.gnj ;
    int nk = map->fhead.gnk ;
    void *f_in = block.mem ;
//     RANGE(zmap_t) field_out = RANGE_KIND(zmap_t, stream, stream+ni*nj*nk) ;
    RANGE(zmap_t) field_out = stream ;
//     status = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, nk, datyp, data_control, &data_kind) ;
  }else{
//     status = fst98_decode((void *)f_out,  encoded.bot, ni, nj, 1, data_kind, data_control) ;
  }
  return status ;
}
// typedef int32_t codec_fn(zmap *map, zmap_block block, zmap_stream stream, int encode) ;
// codec_args args_codec ;     // for use by codec_fn
// SET_CODEC_ARGS(map, c_args) ;                            // set encode/restore codec arguments
// typedef struct{
//   uint32_t nbits ;      // item size
//   uint32_t datyp ;      // item type
//   uint64_t dummy ;      // not used for now
// } fst98_codec_args;
// CT_ASSERT(sizeof(fst98_codec_args) == CODEC_ARGS_SIZE, "sizeof(fst98_codec_args) != CODEC_ARGS_SIZE") ;
