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
#include <rmn/lorenzo.h>
#include <rmn/fp_qlin.h>
#include <rmn/fp_qflog.h>
#include <rmn/ieee_extras.h>

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
static void memcpy_d_f(void * restrict f_, const void *restrict d_, int nb) {
  float *f = (float *)f_ ;
  double *d = (double *)d_ ;
  for (int i = 0; i < nb; i++) { f[i] = d[i] ; }
// fprintf(stderr,"memcpy_d_f : double -> float copy\n");
}

// static void print_d(const void *restrict d_, int nb){
//   double *d = (double *)d_ ;
//   for (int i = 0; i < nb; i++) { fprintf(stderr," %g", d[i]) ; }
//   fprintf(stderr,"\n") ;
// }

// static void swap_64(void *out_, void *in_, int32_t n){
//   uint64_t *in = (uint64_t *)in_, *out = (uint64_t *)out_ ;
//   for(int i=0 ; i<n ; i++) { out[i] = (in[i] >> 32) | (in[i] << 32) ; }
// }

static inline int32_t type_is_real(const int32_t type_flag) {
    return ((base_fst_type(type_flag) == FST_TYPE_REAL_IEEE)      ||
            (base_fst_type(type_flag) == FST_TYPE_REAL_OLD_QUANT) ||
            (base_fst_type(type_flag) == FST_TYPE_REAL_ABS_ERR)   ||
            (base_fst_type(type_flag) == FST_TYPE_REAL_REL_ERR)   ||
            (base_fst_type(type_flag) == FST_TYPE_REAL));
}

static inline int32_t type_is_old_real(const int32_t type_flag) {
    return ((base_fst_type(type_flag) == FST_TYPE_REAL_OLD_QUANT) || (base_fst_type(type_flag) == FST_TYPE_REAL));
}

// remain consistent with legacy fstd98 code
#define use_old_signed_pack_unpack_code
// force Little endian mode
#if ! defined(Little_Endian)
#define Little_Endian YES
#endif
static int endian_int = 1 ;
static uint8_t *little_endian = (uint8_t *) &endian_int ;

// flags to avoid unnecessary warning messages
static uint8_t dejavu[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;

//! \return size of encoded data in 32 bit units
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
  int *data_kind
) {
  if(*little_endian == 0){
    Lib_Log(APP_LIBFST, APP_FATAL, "%s: CPU is not Little Endian.\n", __func__);
    exit(3) ;
  }
  int datyp_in = dtypef.type ;
  int npak = -dtypef.nbits ;                                             // stick to legacy behavior for npak
  int nbits = (npak < 0) ? (-npak) : ( Max(1, 32 / Max(1, npak)) );      // npak == 0 or 1 will set nbits to 32
  if ((npak == 0) || (npak == 1)) { datyp_in = FST_TYPE_BINARY; }        // no compaction, nbits is already 32

  bitstream stream_out_ = *stream_out ;                  // save output stream state
  StreamFlush(stream_out) ;                              // make sure we start on a word boundary

  int64_t navail = STREAM_BITS_EMPTY(*stream_out)/32 ;   // available space in stream for encoded data (32 bit units)
  datyp_in = dtypef.type ;
  // account for legacy xdf_double / xdf_short / xdf_byte
  int src_control = SRC_SIZE(dtypef.size) ;
  int XdfDouble = xdf_double || (src_control == FST_DOUBLE) ;
  int XdfShort  = xdf_short  || (src_control == FST_SHORT) ;
  int XdfByte   = xdf_byte   || (src_control == FST_BYTE);

  nk = Max(1, nk);                          // take care of nk == 0
  const uint32_t *field_u32 = field_in;
  float* field_f = NULL;                    // float version of the data
  uint32_t* field_missing = NULL;           // data with missing values transformed
  int nw;                                   // number of 32 bit words needed for encoded stream

  // FST_TYPE_MAGIC: 512+256+32+1 no interference with turbo pack (128) and missing value (64) flags
  int is_magic   = ((datyp_in & FST_TYPE_MAGIC) == FST_TYPE_MAGIC) ;
  //   if(is_magic) goto fail ;         // TODO : disallow FST_TYPE_MAGIC ?
  int is_missing = datyp_in & FSTD_MISSING_FLAG;      // flag : missing value feature is requested
  int is_turbo   = datyp_in & FST_TYPE_TURBOPACK;     // flag : turbo packing activated
  int no_turbo   = datyp_in & FST_NO_TURBOPACK;       // disable turbo if flag is present
  if(no_turbo) is_turbo = 0 ;
  int in_datyp   = base_fst_type(datyp_in);           // suppress flags, only retain base type

  if(base_fst_type(datyp_in) == FST_TYPE_BINARY){     // cancel all options if FST_TYPE_BINARY
    in_datyp = datyp_in = FST_TYPE_BINARY ;
    xdf_double = xdf_short = xdf_byte = 0 ;
    XdfDouble = XdfShort = XdfByte = 0 ;
    is_missing = is_turbo = 0 ;
  }

  if(type_is_real(in_datyp) && (nbits <= 16)){        // float data type with nbits <= 16 automatically activates turbo
    is_turbo = no_turbo ? 0 : FST_TYPE_TURBOPACK ;    // conditionally activate turbo
    datyp_in = datyp_in | is_turbo | is_missing ;     // keep flags
  }
  int datyp = is_magic ? 1 : in_datyp;                // base data type, is_magic means source array is double (type 1 + XdfDouble)

  int header_size, stream_size, p1out, p2out;

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

// is_magic means source array is double. set packing funtion for floating point numbers appropriately
  PackFunctionPointer packfunc = ((XdfDouble) || (is_magic)) ? compact_p_double : compact_p_float;

//   if (base_fst_type(datyp) == FST_TYPE_REAL_IEEE && nbits < 16) {
//     Lib_Log(APP_LIBFST, APP_WARNING, "%s: IEEE float with < 16 bits is not allowed, bumping to 16 bits\n", __func__);
//     nbits = 16 ;
//   }

//   if ( (datyp_in == (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK)) && (nbits > 32) ) {
  if ( is_turbo && (nbits > 32) ) {
    if (! dejavu[4]) {
      Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported if nbits > 32\n", __func__) ;
      dejavu[4] = 1;
    }
    datyp = datyp_in & (~FST_TYPE_TURBOPACK) ;   // remove turbo compression flag
    is_turbo = 0 ;      // extra compression not supported
  }

  if (is_turbo && (nk > 1)) {
    if (! dejavu[3]) {
      Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported for 3D data.\n", __func__);
      dejavu[3] = 1;
    }
    is_turbo = 0 ;                       // cancel turbo compression
  }

//   if ( (datyp == FST_TYPE_REAL_OLD_QUANT) && ((nbits >= 24) && (nbits <= 32)) ) {
  if ( type_is_old_real(datyp) && ((nbits >= 24) && (nbits <= 32)) ) {
    // E32 automatic conversion, turbo packing and missing remain applicable
    datyp = FST_TYPE_REAL_IEEE;          // will be switched to FST_TYPE_REAL_REL_ERR with maxerr/minabs/zval == 0
    nbits = 32;                          // bump nbits to 32
  }

  int IEEE_64 = 0;                                    // 64 bit IEEE, FST_TYPE_REAL_IEEE(5) or FST_TYPE_COMPLEX(8)
  if ( (type_is_real(datyp))    && (nbits > 32) ) { datyp = FST_TYPE_REAL_IEEE ; IEEE_64 = 1 ; }      // 64 bit floating point IEEE
  if ( (is_type_complex(datyp)) && (nbits > 32) ) IEEE_64 = 1;                                        // 64 bit complex IEEE
  if(IEEE_64){ nbits = 64 ; is_turbo = 0 ; }


  // fudge field if missing value feature is used, 
  int sizefactor = 4;
  if (XdfByte)  sizefactor = 1;                  // source is a byte array (8 bit integer values)
  if (XdfShort) sizefactor = 2;                  // source is a halfword array (16 bit integer values)
  if (XdfDouble || IEEE_64) sizefactor = 8;      // source is a double array (64 bit IEEE floating point values)
  // put appropriate values into field_missing after allocating it
  if (is_missing) {
    field_missing = malloc(ni*nj*nk * sizefactor);       // allocate temporary field for missing values flagging
    if(field_missing == NULL) goto fail ;
    // fudge datyp for call to DecodeMissingValue
    int mdatyp = base_fst_type(datyp) ;
    if(type_is_real(mdatyp))           mdatyp = FST_TYPE_REAL_IEEE ;   // float (32 or 64 bits)
    if(mdatyp == FST_TYPE_SIGNED_NG)   mdatyp = FST_TYPE_SIGNED ;      // signed integer (8 / 16 / 32) bits
    if(mdatyp == FST_TYPE_UNSIGNED_NG) mdatyp = FST_TYPE_UNSIGNED ;    // unsigned integer (8 / 16 / 32) bits
    if (EncodeMissingValue(field_missing, field_in, ni*nj*nk, mdatyp, sizefactor*8, nbits) > 0) {
      field_u32 = field_missing;
    }else{
      field_u32 = field_in;
      free(field_missing) ;
      field_missing = NULL ;
      Lib_Log(APP_LIBFST, APP_INFO, "%s: NO missing value, data type reset to %d\n", __func__, datyp);
      is_missing = 0;      // no missing value detected, cancel missing data flag
    }
  }

  // handle double real / complex type
  if ( (type_is_real(datyp) || is_type_complex(datyp)) && (is_missing == 0) ) {
    if (XdfDouble || IEEE_64) {
      int _nk = is_type_complex(datyp) ? (2 * nk) : nk ;
      if (nbits <= 32) {                // convert from double to float if nbits not larger than 32
        field_f = malloc(ni * nj * _nk * sizeof(float));
        memcpy_d_f(field_f, (const double *)field_in, ni*nj*_nk) ;     // copy doubles into floats
        packfunc = &compact_p_float;                                   // will pack from floats
        field_u32 = (uint32_t*)field_f;                                // source is now new float array
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

// handle 64 bit straight IEEE (IEEE_64). add endian swap
// datype would be FST_TYPE_REAL_IEEE or FST_TYPE_COMPLEX
  if(IEEE_64){
    nw = 2 * ni*nj*nk ;
    nbits = 64 ;
    if(is_type_complex(datyp)) nw *= 2 ;    // 64 bit complex
    if(navail < nw+1) goto fail ;           // insufficient space

    // TODO : use ni*nj*nk instead of nw in header ?
    uint32_t *buf = (uint32_t *)STREAM_IN(*stream_out) ;
    buf[0] = (datyp << 24) | ((nbits-1) << 18) | ((nw) & 0x3FFFF) ;
    buf++ ;
#if defined(Little_Endian)
    uint64_t *bui64 = (uint64_t *)field_u32 ;
    uint64_t *buo64 = (uint64_t *)buf ;
    for(int i = 0 ; i<nw/2 ; i++) { buo64[i] = (bui64[i] >> 32) | (bui64[i] << 32) ; } ;  // swap 32/32
#else
#error "Little_Endian NOT defined"
#endif
    STREAM_IN(*stream_out) += (nw+1) ;                        // inserted nw+1 32 bit words into stream
    goto end ;
  }
// fprintf(stderr, "is_turbo = %d, no_turbo = %d\n", is_turbo, no_turbo);
redo_switch_datyp:

  switch (datyp) {

    // transparent bit stream data, nbits per item
    case FST_TYPE_BINARY:{
      nw = (ni*nj*nk * nbits + 31) / 32 ;     // needed space in 32 bit units for data
      if(navail < nw+1) goto fail ;           // insufficient space ?

      uint32_t *buf = (uint32_t *)STREAM_IN(*stream_out) ;
      uint32_t header = (datyp << 24) | ((nbits-1) << 18) | (nw & 0x3FFFF) ;
      buf[0] = header ;                       // insert packing header into stream

      buf++ ;
      for (int i = 0; i < nw; i++) { buf[i] = field_u32[i]; }      // copy data into stream
      STREAM_IN(*stream_out) += (nw+1) ;                           // inserted nw+1 32 bit words into stream
      is_turbo = 0;
      break;                      // nw = actual length of "encoded" stream
    }

    // floating point, old style packers
    case FST_TYPE_REAL_OLD_QUANT: {
      dtypef.maxerr = 0.0f ;
      datyp = FST_TYPE_REAL_ABS_ERR ;
      goto redo_switch_datyp ;
      break;
    }

    // floating point with max absolute error, last gen style packers and encoders
    case FST_TYPE_REAL_ABS_ERR:{
      if(navail < ni*nj*nk+1) goto fail ;                   // insufficient space for worst case

      int32_t t[ni*nj*nk] ;
      float maxerr = dtypef.maxerr ;
      int32_t offset = is_turbo ? 0 : 0x7FFFFFFF, e_base = 0 ;
      e_base = fp_to_qlin((float *)field_u32, (int32_t *)t, ni*nj*nk, maxerr, ((maxerr != 0) ? 0 : nbits), &offset, NULL) ;    // quantize field_u32[] -> t[]
      if(e_base < 0 || e_base > 254) goto fail ;                                                       // linear quantizer error

      is_turbo = no_turbo ? 0 : FST_TYPE_TURBOPACK ;                                                   // turbo on except if prohibited
      if(is_turbo) LorenzoPredict((int32_t *)t, (int32_t *)t, ni, ni, ni, nj);                         // predict t[] in place

      uint32_t header = ((datyp | is_turbo) << 24) | ((nbits-1) << 18) | (e_base & 0xFF) ;
      STREAM_PUT_NBITS(*stream_out,   header, 32) ;
      STREAM_PUT_NBITS(*stream_out, offset, 32) ;
      int32_t encoded = encode_block(stream_out, t, ni, ni, nj, 8, 0 ) ;                               // encode t[]
      nw = (encoded+31)/32 ;
      break;
    }

    // floats with max relative error, last gen encoders.  keep nbits bits from mantissa
    // minabs and zval will be stored as biased IEEE exponents in header
    // minabs : smallest signicant absolute value (will be truncated to power of 2 <= minabs)
    // zval   : replace absolute value < minabs with zval (truncated to power of 2 <= zval)
    case FST_TYPE_REAL_REL_ERR:{
      if(navail < ni*nj*nk+1) goto fail ;                   // insufficient space for worst case

      int32_t t[ni*nj*nk] ;
      float minabs = dtypef.minabs , zabs = dtypef.zval ;
//       fp_to_flog((float *)field_u32, (int32_t *)t, ni*nj*nk, nbits) ;                                  // "quantize" field_u32[] -> t[]
      fp_to_qlog((float *)field_u32, (int32_t *)t, ni*nj*nk, nbits, minabs) ;                             // "quantize" field_u32[] -> t[]
      is_turbo = no_turbo ? 0 : FST_TYPE_TURBOPACK ;                                                   // turbo on except if prohibited
      if(is_turbo) LorenzoPredict((int32_t *)t, (int32_t *)t, ni, ni, ni, nj);                         // predict t[] in place

      uint32_t zexp = fp32_exp_raw(zabs), minexp = fp32_exp_raw(minabs) ;
      uint32_t header = ((datyp | is_turbo) << 24) | ((nbits-1) << 18) | (minexp << 8) | zexp ;
      STREAM_PUT_NBITS(*stream_out,   header, 32) ;
      int32_t encoded = encode_block(stream_out, t, ni, ni, nj, 8, 0 ) ;                               // encode t[]
      nw = (encoded+31)/32 ;
      break;
    }

    // floating point, new packers
    case FST_TYPE_REAL:{
      dtypef.maxerr = 0.0f ;
      datyp = FST_TYPE_REAL_ABS_ERR ;
      goto redo_switch_datyp ;
      break;
    }

    // IEEE and IEEE complex representation
    case FST_TYPE_REAL_IEEE:
      dtypef.maxerr = 0.0f ;
      dtypef.minabs = 0.0f ;
      dtypef.zval = 0.0f ;
      datyp = FST_TYPE_REAL_REL_ERR ;
      goto redo_switch_datyp ;
      break ;

    case FST_TYPE_COMPLEX: {
      int32_t f_ni   = 2 * ni;
      int32_t f_njnk = nj * nk;
      int32_t f_zero = 0;
      int32_t f_one  = 1;
      int32_t f_nbits = (- nbits);
      is_turbo = 0 ;
      nw = (f_ni*f_njnk * nbits + 31) / 32 ;                 // needed length
      if(navail < nw+1) goto fail ;                         // insufficient space ?
      uint32_t *buf = (void *)STREAM_IN(*stream_out), *header = buf ;
      buf++ ;
      f77name(ieeepak)((int32_t*)field_u32, (int32_t *)buf, &f_ni, &f_njnk, &f_nbits, &f_zero, &f_one);
      header[0] = ((datyp | is_turbo)  << 24) | ((nbits-1) << 18) | (nw & 0x3FFFF) ;
      STREAM_IN(*stream_out) += (nw+1) ;                        // inserted nw+1 32 bit words into stream
      break;
    }

    // integers, short integers or bytes (unsigned)
    case FST_TYPE_UNSIGNED:{
      datyp = FST_TYPE_UNSIGNED_NG ;
      goto redo_switch_datyp ;
      break;
    }

    // integers, short integers or bytes (unsigned), next gen encoders
    case FST_TYPE_UNSIGNED_NG:{
        uint32_t t_[ni*nj], *t = t_ ;
        if(navail < ni*nj*nk) goto fail ;
        if (XdfShort) {               // 16 bits to 32 bits expansion
          nbits = Min(16, nbits);     // at most 16 bits
          uint16_t *s16 = (uint16_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { t[i] = s16[i] ; } ;
        } else if (XdfByte) {         // 8 bits to 32 bits expansion
          nbits = Min(8, nbits);      // at most 8 bits
          uint8_t *s8 = (uint8_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { t[i] = s8[i] ; } ;
        }else{
          t = (uint32_t *)field_u32 ;
        }
        int32_t pred_[ni*nj], *pred = pred_ ;
        if(is_turbo){
          LorenzoPredict((int32_t *)t, pred, ni, ni, ni, nj) ;
        }else{
          pred = (int32_t *)t ;     // point to original data
        }
        int32_t encoded = encode_block(stream_out, (int32_t *)pred, ni, ni, nj, 8, 0 /*ENCODE_DRY_RUN*/);
        int nwords = (encoded+31)/32 ;
        nw = nwords ;
//         fprintf(stderr,"encode FST_TYPE_UNSIGNED_NG : is_turbo = %d, datyp = %d, nw = %d, nwords = %d, encoded = %d\n", is_turbo, datyp, nw, nwords, encoded) ;
      }
      break;

    // integers, short integers or bytes (signed), next gen encoders
    case FST_TYPE_SIGNED_NG:{
// fprintf(stderr,"encode FST_TYPE_SIGNED_NG : is_turbo = %d, nbits = %d, nw = %d\n", is_turbo, nbits, nw) ;
        int32_t t[ni*nj] ;
        if (XdfShort) {               // 16 bits to 32 bits expansion
          nbits = Min(16, nbits);     // at most 16 bits
          int16_t *s16 = (int16_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { t[i] = s16[i] ; t[i] = (t[i] << (32-nbits)) >> (32-nbits) ; } ;
        } else if (XdfByte) {         // 8 bits to 32 bits expansion
          nbits = Min(8, nbits);      // at most 8 bits
          int8_t *s8 = (int8_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { t[i] = s8[i] ; t[i] = (t[i] << (32-nbits)) >> (32-nbits) ; } ;
        }else{                        // 32 bits to 32 bits
          int32_t *s32 = (int32_t *)field_u32 ;
          for(int i=0 ; i<ni*nj*nk ; i++) { t[i] = (s32[i] << (32-nbits)) >> (32-nbits) ; }
        }
        int32_t pred_[ni*nj], *pred = pred_, encoded=-1 ;
        if(is_turbo){
          LorenzoPredict((int32_t *)t, pred, ni, ni, ni, nj) ;                  // predict t[] -> pred
        }else{
          pred = t ;                                                            // point pred -> t
        }
        encoded = encode_block(stream_out, pred, ni, ni, nj, 8, 0 ) ;            // encode pred[]
        nw = (encoded+31)/32 ;
//         fprintf(stderr,"encode FST_TYPE_SIGNED_NG : is_turbo = %d, datyp = %d, nw = %d, nwords = %d, encoded = %d, raw = %d\n", is_turbo, datyp, nwords, nwords, encoded, ni*nj*nk*nbits) ;
      }
      break;

    // integers, short integers or bytes (signed)
    case FST_TYPE_SIGNED:{
      datyp = FST_TYPE_SIGNED_NG ;
      goto redo_switch_datyp ;
      break;
    }

    // character data, R4A items (4 chars in an unsigned integer)
    case FST_TYPE_CHAR:{
      if (is_turbo) {
        if (! dejavu[7]) {
          Lib_Log(APP_LIBFST,APP_WARNING, "%s: extra compression not available for characters, data type reset to FST_TYPE_CHAR (%d)\n",
                  __func__, FST_TYPE_CHAR);
          dejavu[7] = 1;
        }
        is_turbo = 0;
      }
      uint32_t *buf = (void *)STREAM_IN(*stream_out) ;
      nbits = 8;
      nw = ((ni*nj*nk * 8) + 32 - 1) / 32;
      if(navail < nw+1) goto fail ;         // insufficient space
      buf[0] = (datyp  << 24) | ((8-1) << 18) | (nw & 0x3FFFF) ;

      buf++ ;
      compact_p_integer(field_u32, (void *) NULL, buf, nw, 32, 0, xdf_stride, 0);

      STREAM_IN(*stream_out) += (nw+1) ;                        // inserted nw+1 32 bit words into stream
      break;
    }

    // character string
    case FST_TYPE_STRING:{
      if (is_turbo) {
        if (! dejavu[8]) {
          Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not available for strings, data type reset to FST_TYPE_STRING (%d)\n",
                  __func__, FST_TYPE_STRING);
          dejavu[8] = 1;
        }
        is_turbo = 0;
    }

      uint32_t *buf = (void *)STREAM_IN(*stream_out) ;
      nw = (ni*nj*nk * 8 + 31) / 32 ;
      if(navail < nw+1) goto fail ;         // insufficient space
      buf[0] = (datyp  << 24) | ((8-1) << 18) | (nw & 0x3FFFF) ;

      buf++ ;
      compact_p_char(field_u32, (void *) NULL, buf, ni*nj*nk, 8, 0, xdf_stride);

      STREAM_IN(*stream_out) += (nw+1) ;                        // inserted nw+1 32 bit words into stream
      break;
    }

    default:
        Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
        goto fail ;
  } // end switch

  StreamFlush(stream_out) ;     // make sure we end on a proper stream boundary

end:
  // free temporary arrays if they were used
  if (field_f       != NULL) free(field_f);
//   if (field_missing != NULL) free(field_missing);

  xdf_byte = xdf_short = xdf_double = 0 ;               // reset other than 32 bits flags
  datyp = datyp | is_missing | is_turbo ;               // restore missing and turbo flags, use possibly revised datyp
  *data_kind = datyp | (nbits << 8) ;                  // compound information for decoder
  return nw ;

fail :
  // cleanup before failing
  nbits = 0 ;
  nw = -1 ;
  datyp = is_missing = is_turbo = 0 ;
  *stream_out = stream_out_ ;                 // restore output stream state
exit(1) ;                     // while debugging
  goto end ;
}

//! \return token size
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
) {
  if(*little_endian == 0){
    Lib_Log(APP_LIBFST, APP_FATAL, "%s: CPU is not Little Endian.\n", __func__);
    exit(3) ;
  }
  data_control = DST_SIZE(data_control) ;
  // account for legacy xdf_double / xdf_short / xdf_byte / downgrade_32
  int XdfDouble   = xdf_double   || (data_control == FST_DOUBLE) ;     // output will be 64 bit doubles
  int XdfShort    = xdf_short    || (data_control == FST_SHORT) ;      // output will be 16 bit
  int XdfByte     = xdf_byte     || (data_control == FST_BYTE);        // output will be 8 bit
  int Downgrade32 = downgrade_32 || (data_control == FST_WORD);        // output will be 32 bit floats

  uint32_t *field = data_out;
  int ier = 0 ;
  int datyp = data_kind & 0xFF ;
  int is_turbo = (datyp & FST_TYPE_TURBOPACK) ;
  int nbits_in = (data_kind >> 8) & 0xFF ;
  ssize_t navail ;
  bitstream stream_in_ = *stream_in ;              // save input stream state

  if(base_fst_type(datyp) == FST_TYPE_BINARY){    // cancel all features if FST_TYPE_BINARY, only keep nbits
    datyp = FST_TYPE_BINARY ;
    xdf_double = xdf_short = xdf_byte = 0 ;
    XdfDouble = XdfShort = XdfByte = Downgrade32 = 0 ;
  }else{
    if(nbits_in > 32 && nbits_in != 64) goto fail ;    // must be 64 bits if > 32 bits
  }
  // Get missing data flag
  int is_missing = datyp & FSTD_MISSING_FLAG ;
  // Suppress missing data flag
  datyp = datyp & (~FSTD_MISSING_FLAG) ;

  UnpackFunctionPointer packfunc = XdfDouble ? &compact_u_double : &compact_u_float;

  int nelm = ni*nj*nk ;
  uint32_t *buf = NULL ;
  STREAM_XTRACT_ALIGN32(*stream_in) ;
  buf = STREAM_OUT(*stream_in) ; ;

  ier = nbits_in ;
  STREAM_XTRACT_ALIGN32(*stream_in) ;                 // align bit stream to a 32 bit boundary
  navail = StreamAvailableBits(stream_in)/32 ;        // get number of available 32 bit words
  buf = STREAM_OUT(*stream_in) ;

  switch (datyp) {
    case FST_TYPE_BINARY: {            // Raw binary
      int32_t lngw = ((nelm * nbits_in) + 32 - 1) / 32 ;    // number of 32 bit words to extract
      if(lngw+1 > navail) goto fail ;                       // need more than what is available ?

      uint32_t header = buf[0] ;                            // get and check header
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , lngw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in || (lngw & 0x3FFFF) != lngw_) goto fail ;

      buf++ ;                                               // skip header
      for (int32_t i = 0; i < lngw; i++) { field[i] = buf[i]; }

      STREAM_OUT(*stream_in) += (lngw + 1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    case FST_TYPE_REAL_OLD_QUANT: {          // Floating Point, old style packers
exit(4) ;
      uint32_t lngw = ((nelm * nbits_in) + (96+24) + 31) / 32;

      uint32_t header = buf[0] ;                            // get and check header
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , lngw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in || (lngw & 0x3FFFF) != lngw_) goto fail ;

      buf++ ;                                               // skip header
      double dmin = 0.0, dmax = 0.0, tempfloat = 99999.0;   // by_product of decoder
      packfunc(field, buf, buf + 3, nelm, nbits_in, 24, xdf_stride, 0, &tempfloat, &dmin , &dmax);

      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    case FST_TYPE_REAL:
    case FST_TYPE_REAL | FST_TYPE_TURBOPACK: {
exit(4) ;
      // Floating point, new packers
      int nbits, header_size, stream_size, p1out, p2out, lngw;
      c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni*nj*nk);
      header_size /= sizeof(int32_t);
      stream_size /= sizeof(int32_t);

      uint32_t header = buf[0] ;                            // get and check header
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , lngw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in) goto fail ;

      buf++ ;                                               // skip header
      if (is_type_turbopack(datyp)) {
        int32_t tbuf[ni*nj*nk + 16] ;
        lngw = buf[0] + 1 ;
        if((lngw & 0x3FFFF) != lngw_) goto fail ;
        memcpy(tbuf, buf, (lngw+1)*sizeof(int32_t)) ;       // copy stream into temporary buffer to avoid overwriting input stream
        armn_compress((byte *)(tbuf + 1 + header_size), ni, nj, nk, nbits_in, 2, 1);
        c_float_unpacker((float *)field, (int32_t *)(tbuf + 1), (int32_t *)(tbuf + 1 + header_size), nelm, &nbits);

      }else{
        lngw = header_size + stream_size ;   // header + data
        if((lngw & 0x3FFFF) != lngw_) goto fail ;
        c_float_unpacker((float *)field, (int32_t *)buf, (int32_t *)(buf + header_size), nelm, &nbits);
      }

      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    case FST_TYPE_REAL_IEEE:                // IEEE (normally replaced with FST_TYPE_REAL_REL_ERR, except for IEEE_64)
if(nbits_in != 64)exit(4) ;
    case FST_TYPE_COMPLEX: {                // complex numbers (encoded as IEEE)
      if (datyp == FST_TYPE_COMPLEX) nelm *= 2;             // complex data, double number of values
      int lngw = (nelm * nbits_in + 31)/32 ;
      uint32_t header = buf[0] ;
      buf++ ;                                               // skip header

      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , nw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in || (lngw & 0x3FFFF) != nw_) goto fail ;

      if ((downgrade_32 || Downgrade32) && (nbits_in == 64)) {    // Downgrade 64 bit doubles to 32 bit floats
#if defined(Little_Endian)
        uint64_t *t64 = (uint64_t *)buf ;
        for (int i = 0; i < nelm; i++) { t64[i] = (t64[i] >> 32) | (t64[i] << 32) ; }   // 32/32 endian swap
#else
#error "Little_Endian is not defined"
#endif
        memcpy_d_f((void *)field, (void *)buf, nelm) ;            // downgrading double -> float
      }else{
        int32_t f_one = 1;
        int32_t f_zero = 0;
        int32_t f_mode = 2;
        int f_minus_nbits = (-nbits_in);
        if(nbits_in == 64){                                       // IEEE_64
#if defined(Little_Endian)
        uint64_t *t64 = (uint64_t *)buf, *o64 = (uint64_t *)field ;
        for (int i = 0; i < nelm; i++) { o64[i] = (t64[i] >> 32) | (t64[i] << 32) ; }   // 32/32 endian swap
#else
#error "Little_Endian is not defined"
#endif
        }else{                   // not 64 bits ( <= 32 bits)
          f77name(ieeepak)((int32_t *)field, (int32_t *)buf, &nelm, &f_one, &f_minus_nbits, &f_zero, &f_mode);
        }
      }
      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    case FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK: {
exit(4) ;
      uint32_t header = buf[0] ;
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , nw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in) goto fail ;

      buf++ ;                                               // skip header
      int32_t lngw = buf[0] + 1 ;
      if( (lngw & 0x3FFFF) != nw_ ) goto fail ;
      // IEEE Floating point direct packers
      c_armn_uncompress32((float *)field, (byte *)(buf + 1), ni, nj, nk, nbits_in);
      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    case FST_TYPE_UNSIGNED:                // Integers, short integers or bytes (unsigned)
    case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK: {
exit(4) ;
      uint32_t header = buf[0] ;
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , nw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in) goto fail ;

      int32_t lngw = nw_ ;   // TEMPORARY
      buf++ ;                                               // skip header
      if (is_type_turbopack(datyp)) lngw = buf[0] + 1 ;

      int offset = is_type_turbopack(datyp) ? 1 : 0;
      if (XdfShort) {
        if (is_type_turbopack(datyp)) {
          uint32_t t[ni*nj*nk] ;
          memcpy(t, buf, lngw*sizeof(uint32_t)) ;  // copy to temporary storage to avoid overwriting stream when unpacking
          int nbytes = armn_compress((byte *)(t + offset), ni, nj, nk, nbits_in, 2, 0);
          memcpy(field, t + offset, nbytes);
        }else{
          ier = compact_u_short(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride);
        }
      } else if(XdfByte) {
        if (is_type_turbopack(datyp)) {
          uint32_t t[ni*nj*nk] ;
          memcpy(t, buf, lngw*sizeof(uint32_t)) ;  // copy to temporary storage to avoid overwriting stream when unpacking
          armn_compress((byte *)(t + offset), ni, nj, nk, nbits_in, 2, 0);
          memcpy_16_8((uint8_t *)field, (uint16_t *)(t + offset), nelm);
        }else{
          ier = compact_u_char(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride);
        }
      }else{
        if (is_type_turbopack(datyp)) {
          uint32_t t[ni*nj*nk] ;
          memcpy(t, buf, lngw*sizeof(uint32_t)) ;  // copy to temporary storage to avoid overwriting stream when unpacking
          armn_compress((byte *)(t + offset), ni, nj, nk, nbits_in, 2, 0);
          memcpy_16_32((uint32_t *)field, (uint16_t *)(t + offset), nbits_in, nelm);
        }else{
          ier = compact_u_integer(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 0);
        }
      }
      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    // integers, short integers or bytes (unsigned), last gen encoders
    case FST_TYPE_UNSIGNED_NG:
    case (FST_TYPE_UNSIGNED_NG) | FST_TYPE_TURBOPACK: {
      int32_t decoded, t[nelm] ;
      if(XdfShort == 0 && XdfByte == 0 && is_turbo == 0){
        decoded = decode_block(stream_in, (int32_t *)field, ni, ni, nj, 8) ;  // 32 bit delivery, no turbo, decode into destination
      }else{
        decoded = decode_block(stream_in, (int32_t *)t, ni, ni, nj, 8) ;      // decode into temporary array t
      }
      if(decoded < 0) goto fail ;
      if(is_turbo){
        LorenzoUnpredict( (XdfShort || XdfByte) ? t : (int32_t *)field , t, ni, ni, ni, nj) ;     // unpredict into temporary or destination
      }
      if (XdfShort) {
        uint16_t *d16 = (uint16_t *)field ;
        for(int i=0 ; i<nelm ; i++){ d16[i] = t[i] ; } ;      // deliver into halfwords
      }else if(XdfByte) {
        uint8_t *d8 = (uint8_t *)field ;
        for(int i=0 ; i<nelm ; i++){ d8[i] = t[i] ; } ;       // deliver into bytes
      }
      break;
    }

    // integers, short integers or bytes (signed), last gen encoders
    case FST_TYPE_SIGNED_NG:
    case (FST_TYPE_SIGNED_NG) | FST_TYPE_TURBOPACK:{
      int32_t decoded = 0 ;

      if(XdfShort == 0 && XdfByte == 0){
        decoded = decode_block(stream_in, (int32_t *)field, ni, ni, nj, 8) ;
        if(is_turbo){ LorenzoUnpredict( (int32_t *)field , (int32_t *)field, ni, ni, ni, nj) ; }
      }else{
        int32_t t[nelm] ;
        decoded = decode_block(stream_in, t, ni, ni, nj, 8) ;
        if(is_turbo){ LorenzoUnpredict( t , t, ni, ni, ni, nj) ; }
        if (XdfShort) {
          int16_t *d16 = (int16_t *)field ;
          for(int i=0 ; i<nelm ; i++){ d16[i] = t[i] ; } ;
        }else if(XdfByte) {
          int8_t *d8 = (int8_t *)field ;
          for(int i=0 ; i<nelm ; i++){ d8[i] = t[i] ; } ;
        }
      }
      if(decoded < 0) goto fail ;
// fprintf(stderr,"decode FST_TYPE_SIGNED_NG : is_turbo = %d, decoded = %d\n", is_turbo, decoded) ;
      break;
    }

    // Integers, short integers or bytes (signed)
    case FST_TYPE_SIGNED: {
exit(4) ;
#ifdef use_old_signed_pack_unpack_code
      int lngw ;
      uint32_t header = buf[0] ;
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , nw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in) goto fail ;

      lngw = nw_ ;   // TEMPORARY
      buf++ ;                                               // skip header

      int32_t *field_out ;
      if (XdfShort || XdfByte || XdfDouble) {                // need temporary array to unpack
        field_out = malloc(nelm * sizeof(int));
      }else{
        field_out = (int32_t *)field;
      }
      // unpack into field_out
      ier = compact_u_integer(field_out, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
      if (XdfShort) {                           // copy into "short" destination
        int16_t *out = (int16_t *)field;
        for (int i = 0; i < nelm; i++) {
          out[i] = field_out[i];
        }
      }
      else if (XdfByte) {                       // copy into "byte" destination
        int8_t *out = (int8_t *)field;
        for (int i = 0; i < nelm; i++) {
          out[i] = field_out[i];
          }
      }
      if (field_out != (int32_t*)field) free(field_out); // needed temporary array
      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
#else
#error "use_old_signed_pack_unpack_code not defined"
#endif
      break;
    }

    // floats with max relative error, last gen encoders
    // minabs and zval are passed as biased IEEE exponents (in header)
    // minabs : smallest signicant absolute value (should match minabs/zval from fp_to_qlog_n)
    // zval   : an absolute value < |minabs| gets replaced with |zval| (sign of value is preserved)
    case FST_TYPE_REAL_REL_ERR:
    case (FST_TYPE_REAL_REL_ERR) | FST_TYPE_TURBOPACK:{
      int32_t t[ni*nj] ;
      uint32_t header ;
      STREAM_GET_NBITS(*stream_in, header, 32) ;    // get 32 bit header
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1, minexp = (header >> 8) & 0xFF, zexp = header & 0xFF ;
      if(datyp_ != datyp || nbits_ != nbits_in) goto fail ;

      int32_t decoded = decode_block(stream_in, (int32_t *)t, ni, ni, nj, 8) ;
      if(is_turbo)LorenzoUnpredict((int32_t *)t, (int32_t *)t, ni, ni, ni, nj);
//       flog_to_fp((float *)field, (int32_t *)t, nelm, nbits_in) ;
      qlog_to_fp((float *)field, (int32_t *)t, nelm, nbits_in, fp32_from_exp(minexp), fp32_from_exp(zexp)) ;

      break;
    }

    // floating point, last gen style packers and encoders
    case FST_TYPE_REAL_ABS_ERR:
    case (FST_TYPE_REAL_ABS_ERR) | FST_TYPE_TURBOPACK:{
      int32_t t[ni*nj], offset = 0 ;
      uint32_t header ;
      STREAM_GET_NBITS(*stream_in, header, 32) ;
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 ;
      if(datyp_ != datyp || nbits_ != nbits_in) goto fail ;
      uint32_t e_base = header & 0xFF ;
      STREAM_GET_NBITS(*stream_in, offset, 32) ;
      int32_t decoded = decode_block(stream_in, (int32_t *)t, ni, ni, nj, 8) ;
      if(is_turbo)LorenzoUnpredict((int32_t *)t, (int32_t *)t, ni, ni, ni, nj);
      qflin_to_fp((float *)field, (int32_t *)t, nelm, e_base, offset) ;

      break;
    }
    // Character data, R4A style (4 chars in a 32 bit integer)
    case FST_TYPE_CHAR: {
      int32_t lngw = (nelm + 3) / 4;
      uint32_t header = buf[0] ;
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , nw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in || (lngw & 0x3FFFF) != nw_) goto fail ;

      buf++ ;                                               // skip header
      ier = compact_u_integer(field, (void *) NULL, buf, lngw, 32, 0, xdf_stride, 0);
      if(nelm & 3){    // not a multiple of 4, null terminate
        int shift = (4 - (nelm & 3)) * 8 ;
        field[lngw-1] >>= shift ;
        field[lngw-1] <<= shift ;
      }

      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }
    // Character string
    case FST_TYPE_STRING: {
      int32_t lngw = (nelm*8 + 31)/32 ;
      uint32_t header = buf[0] ;
      int32_t datyp_ = header >> 24, nbits_ = ((header >> 18) & 0x3F)+1 , nw_ = header & 0x3FFFF ;
      if(datyp_ != datyp || nbits_ != nbits_in || (lngw & 0x3FFFF) != nw_) goto fail ;

      buf++ ;                                               // skip header
      ier = compact_u_char(field, (void *) NULL, buf, nelm, 8, 0, xdf_stride);

      STREAM_OUT(*stream_in) += (lngw+1) ;                // lngw + 1 32 bit words extracted from stream
      break;
    }

    default:
      Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
      ier = -1;
      goto end ;
  } // end of switch (datyp)

  STREAM_XTRACT_ALIGN32(*stream_in) ;   // align stream to 32 bit boundary

  if (is_missing) {
    // Replace "missing" data points with the appropriate values given the type of data (int/uint/float)
    // if nbits = 64 and IEEE , set XdfDouble (it may already be set)
    // handle new types properly
    int mdatyp = base_fst_type(datyp) ;
    if (mdatyp == FST_TYPE_REAL_IEEE && nbits_in == 64 ) XdfDouble = 1;
    int sz=(XdfDouble?64:(XdfShort?16:(XdfByte?8:32)));
    // fudge datyp for call to DecodeMissingValue
    if(type_is_real(mdatyp))           mdatyp = FST_TYPE_REAL_IEEE ;   // float (32 or 64 bits)
    if(mdatyp == FST_TYPE_SIGNED_NG)   mdatyp = FST_TYPE_SIGNED ;      // signed integer (8 / 16 / 32) bits
    if(mdatyp == FST_TYPE_UNSIGNED_NG) mdatyp = FST_TYPE_UNSIGNED ;    // unsigned integer (8 / 16 / 32) bits
    DecodeMissingValue(field , (ni) * (nj) * (nk) , mdatyp, sz);
  }

  // Upgrade size, if necessary (FST_TYPE_REAL_OLD_QUANT has already been taken care of)
  if (XdfDouble && (nbits_in != 64) && base_fst_type(datyp) != FST_TYPE_REAL_OLD_QUANT) {
    const int base_type = base_fst_type(datyp);
    if(type_is_real(base_type)) {          // float -> double copy
      float f[nelm], *ff = (float *)field;
      memcpy(f, field, nelm * sizeof(float));
// fprintf(stderr, "decoder : XdfDouble upgrade_size, nelm = %d, f[0] = %f, ff[0] = %f\n", nelm, f[0], ff[0]);
      upgrade_size(field, 64, f, 32, nelm, 0);
    }
//     else if (base_type == FST_TYPE_SIGNED || base_type == FST_TYPE_UNSIGNED) {   // int -> long copy
//       int32_t x[nelm];
//       memcpy(x, field, nelm * sizeof(int32_t));
//       resize_int(field, 64, x, 32, nelm);
//     }
  }

// TODO : instead of token size, return decoded length (same as encoder) ?

end:
  return ier ;   // token size (normally > 0)

fail:
  ier = -1 ;
  *stream_in = stream_in_ ;   // restore state of input stream to state at function entry
exit(1) ;                     // when debugging
  goto end ;
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
  (void) (block) ;
  (void) (stream) ;

  memcpy(&fst98_codec_args, ZMAP_CODEC_ARGS(map), CODEC_ARGS_SIZE) ;  // get codec arguments
  if(encode){
//     int ni = map->fhead.gni ;
//     int nj = map->fhead.gnj ;
//     int nk = map->fhead.gnk ;
//     void *f_in = block.mem ;
// //     RANGE(zmap_t) field_out = RANGE_KIND(zmap_t, stream, stream+ni*nj*nk) ;
//     RANGE(zmap_t) field_out = stream ;
// //     status = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, nk, datyp, data_control, &data_kind) ;
  }else{
// //     status = fst98_decode((void *)f_out,  encoded.bot, ni, nj, 1, data_kind, data_control) ;
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
