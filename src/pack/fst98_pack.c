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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <App.h>

#include <rmn/fst98_pack.h>

#define Max(x,y) ((x > y) ? x : y)
#define Min(x,y) ((x < y) ? x : y)

typedef uint8_t byte ;

// byte to halfword signed copy
static void memcpy_8_16(int16_t *p16, const int8_t *p8, int nb) {
    for (int i = 0; i < nb; i++) {
        p16[i] = p8[i];
    }
}

// halfword to byte signed copy
static void memcpy_16_8(int8_t *p8, const int16_t *p16, int nb) {
    for (int i = 0; i < nb; i++) {
        p8[i] = p16[i];
    }
}

// halfword to word signed copy
static void memcpy_16_32(int32_t *p32, const int16_t *p16, int nbits, int nb) {
    int16_t mask = ~ (0xffff << nbits);        // keep lower nbits bits only
    for (int i = 0; i < nb; i++) {
        p32[i] = p16[i] & mask;
    }
}

// word to halfword signed copy
static void memcpy_32_16(int16_t *p16, const int32_t * p32, int nbits, int nb) {
    int32_t mask = ~ (0xffffffff << nbits);    // keep lower nbits bits only
    for (int i = 0; i < nb; i++) {
        p16[i] = p32[i] & mask;
    }
}

// float to double copy
// static void memcpy_f_d(double * restrict d, const float *restrict f, int nb) {
//   for (int i = 0; i < nb; i++) {
//     d[i] = f[i] ;
//   }
// }

// double to float copy
static void memcpy_d_f(float * restrict f, const double *restrict d, int nb) {
  for (int i = 0; i < nb; i++) {
    f[i] = d[i] ;
  }
}

// remain consistent with legacy fstd98 code
#define use_old_signed_pack_unpack_code
// force Little endian for now
#if ! defined(Little_Endian)
#define Little_Endian YES
#endif

// flags to avoid unnecessary warning messages
static uint8_t dejavu[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;

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
  //! [in] Data type of elements
  const int datyp_in,
  //! [in] used to control XdfDouble/XdfShort/XdfByte
  int data_control,
  //! [out] final compound data type and nbits
  int *data_kind
) {
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

  int is_missing = datyp_in & FSTD_MISSING_FLAG;      // flag : missing value feature is requested
  int is_turbo   = datyp_in & FST_TYPE_TURBOPACK;     // flag : turbo packing activated
  int in_datyp   = base_fst_type(datyp_in);           // suppress flags, only retain base type
  // FST_TYPE_MAGIC: 512+256+32+1 no interference with turbo pack (128) and missing value (64) flags
  int is_magic   = (datyp_in & FST_TYPE_MAGIC) == FST_TYPE_MAGIC ;

  int datyp = is_magic ? 1 : in_datyp;                // base data type
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
  if(IEEE_64) nbits = 64 ;
  if ((npak == 0) || (npak == 1)) { datyp = FST_TYPE_BINARY; }        // no compaction, nbits is already 32

  // fudge field if missing value feature is used, 
  int sizefactor = 4;
  if (XdfByte)  sizefactor = 1;                  // source is a byte array (8 bit integer values)
  if (XdfShort) sizefactor = 2;                  // source is a halfword array (16 bit integer values)
  if (XdfDouble || IEEE_64) sizefactor = 8;      // source is a double array (64 bit floating point values)
  // put appropriate values into field_missing after allocating it
  if (is_missing) {
      field_missing = malloc(ni*nj*nk * sizefactor);       // allocate temporary field for missing values flagging
      if(field_missing == NULL) goto fail ;
      if (EncodeMissingValue(field_missing, field_in, ni*nj*nk, datyp, sizefactor*8, nbits) > 0) {
          field_u32 = field_missing;
      } else {
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

  if (is_type_real(datyp) && nbits > 32) {
      datyp = FST_TYPE_REAL_IEEE;                                     // force 64 bits IEEE
      nbits = 64;
  }

  if (datyp == FST_TYPE_REAL) {                                       // type F, new float quantification
      if (nbits > 24) {
          if (! dejavu[0]) {
              Lib_Log(APP_LIBFST, APP_INFO, "%s: nbits > 24, using E32 instead of F%2d\n", __func__, nbits);
              dejavu[0] = 1;
          }
          datyp = FST_TYPE_REAL_IEEE ;                                // keep turbo and/or missing flag
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
  if ( (nbits > 16) && (datyp != FST_TYPE_REAL_IEEE) ) is_turbo = 0 ;
  // if nbits <= 16 and turbo compression is requested, use FST_TYPE_REAL instead
  if( (nbits <= 16) && (datyp == FST_TYPE_REAL_OLD_QUANT) && is_turbo ){
    datyp = FST_TYPE_REAL ;                    // replace base type FST_TYPE_REAL_OLD_QUANT with FST_TYPE_REAL
  }

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
  }

  buffer = NULL ;
  if(VALID_RANGE(field_out)){      // is field_out valid and large enough for encoded stream ?
    if( nw <= RANGE_ITEMS(field_out) ) buffer = (int32_t *) RANGE_BOT(field_out) ;    // large enough, use field_out
  }
  local_buffer = (buffer == NULL) ;
  if(local_buffer){ buffer = (int32_t *) malloc(nw * sizeof(int32_t)); }      // need to allocate buffer
  if(buffer == NULL) goto fail ;
//   if(local_buffer) fprintf(stderr,"DEBUG : need %d words, have %ld, allocated buffer with size %d words\n", nw, RANGE_ITEMS(field_out), nw);

// TODO : handle 64 bit straight IEEE (IEEE_64). add endian swap ?
  if(IEEE_64){
    for(int i = 0 ; i<nw ; i++) { buffer[i] = field_u32[i]; } ;
    goto end ;
  }
  switch (datyp) {

    case FST_TYPE_BINARY:                   // transparent bit stream data, nbits per item
        nw = (ni*nj*nk * nbits + 31) / 32 ;
        for (int i = 0; i < nw; i++) { buffer[i] = field_u32[i]; }
        break;                      // nw = actual length of "encoded" stream

    case FST_TYPE_REAL_OLD_QUANT: {          // floating point, old style packers
        double tempfloat = 99999.0;
        // straight quantifier, no turbo, pack with offset 24 (120 bit header)
        packfunc(field_u32, buffer, buffer+3, ni*nj*nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
        nw = (ni*nj*nk * nbits + 96 + 24 + 31) / 32;
        break;                      // nw = actual length of "encoded" stream
    }

    case FST_TYPE_REAL:                                 // floating point, new packers
        nw = ((header_size + stream_size) * 8 + 31) / 32;      // length if turbo packing not used
        header_size /= sizeof(int32_t);
        if (is_turbo && (nbits <= 16)) {    // use turbo compression scheme
            c_float_packer((float *)field_u32, nbits, buffer + 1, buffer + 1 + header_size, ni*nj*nk);
            int compressed_lng = armn_compress((byte *)(buffer+1+header_size), ni, nj, nk, nbits, 1, 1);
            if (compressed_lng < 0) {
                is_turbo = 0 ;
                c_float_packer((float *)field_u32, nbits, (int32_t *)buffer, (int32_t *)(buffer+header_size), ni*nj*nk);
            } else {
                int nbytes = 16 + (header_size*4) + compressed_lng;
                buffer[0] = nw = (nbytes * 8 + 31) / 32;
                nw ++ ;    // turbo extra header, bump nw ;
            }
        } else {                // no turbo compression
            c_float_packer((float *)field_u32, nbits, (int32_t *)buffer, (int32_t *)(buffer+header_size), ni*nj*nk);
        }
        break;

    case FST_TYPE_UNSIGNED:            // integers, short integers or bytes (unsigned)
        if (is_turbo) {
            const int offset = 1;
            if (XdfShort) {               // 16 bits to 16 bits copy
                nbits = Min(16, nbits);    // at most 16 bits
                memcpy((int16_t *)(buffer + offset), field_u32, ni*nj*nk * 2);
            } else if (XdfByte) {         // 8 bits to 16 bits expansion
                nbits = Min(8, nbits);     // at most 8 bits
                memcpy_8_16((int16_t *)(buffer + offset), (int8_t *)field_u32, ni*nj*nk);
            } else {                       // 32 bits to 16 bits truncation
                memcpy_32_16((int16_t *)(buffer + offset), (const int32_t *)field_u32, nbits, ni*nj*nk);
            }
            int compressed_lng = armn_compress((byte *)(buffer + offset), ni, nj, nk, nbits, 1, 0);
            if (compressed_lng < 0) {     // no gain from turbo, repack
                datyp = FST_TYPE_UNSIGNED;
                is_turbo = 0 ;
                compact_p_integer(field_u32, (void *) NULL, buffer + offset, ni*nj*nk, nbits, 0, xdf_stride, 0);
                nw = (ni*nj*nk * nbits + 31) / 32 ;              // recompute nw using possibly revised nbits
            } else {
                int nbytes = 4 + compressed_lng;
                buffer[0] = nw = (nbytes * 8 + 31) / 32;
                nw ++ ;    // turbo header, bump nw ;
            }
        } else {    // straight packing, no turbo
            if (XdfShort) {
                nbits = Min(16, nbits);    // at most 16 bits
                compact_p_short(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride);
            } else if (XdfByte) {
                nbits = Min(8, nbits);     // at most 8 bits
                compact_p_char(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride);
            } else {
                compact_p_integer(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 0);
            }
            nw = ((ni*nj*nk * nbits) + 31) / 32;                 // recompute nw using possibly revised nbits
        }
        xdf_short = xdf_byte = 0 ;
        break;                      // nw = actual length of "encoded" stream

    case FST_TYPE_SIGNED:            // integers, short integers or bytes (signed)
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

    case FST_TYPE_REAL_IEEE:            // IEEE and IEEE complex representation
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
              } else {
                  int nbytes = 16 + compressed_lng;
                  buffer[0] = nw = (nbytes * 8 + 31) / 32;
                  nw ++ ;    // turbo used, bump nw ;
              }
          } else {
              if (datyp == FST_TYPE_COMPLEX) f_ni = f_ni * 2;
              f77name(ieeepak)((int32_t*)field_u32, (int32_t *)buffer, &f_ni, &f_njnk, &f_minus_nbits, &f_zero, &f_one);
              nw = (f_ni*f_njnk * nbits + 31) / 32 ;
          }
        }
        break;

    case FST_TYPE_CHAR:            // character data, R4A items (4 chars in an unsigned integer)
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

    case FST_TYPE_STRING:                                 // character string
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
  //! [in] datyp + nbits
  int data_kind,
  //! [in] used to control XdfDouble/XdfShort/XdfByte
  int data_control
) {
  int XdfDouble = xdf_double || (data_control & DST_DOUBLE) ;
  int XdfShort  = xdf_short  || (data_control & DST_SHORT) ;
  int XdfByte   = xdf_byte   || (data_control & DST_BYTE);
// fprintf(stderr,"DEBUG (fst98_decode) : XdfDouble = %d, XdfShort = %d, XdfByte = %d\n", XdfDouble, XdfShort, XdfByte) ;
  uint32_t *field = data_out;
  int ier = 0 ;
  int datyp = data_kind & 0xFFFF ;
  int nbits_in = data_kind >> 16 ;

    // Get missing data flag
    int has_missing = datyp & FSTD_MISSING_FLAG;
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
    switch (datyp) {
        case FST_TYPE_BINARY: {            // Raw binary
            int lngw = ((nelm * nbits_in) + 32 - 1) / 32;
            for (int i = 0; i < lngw; i++) {
                field[i] = buf[i];
            }
            break;
        }

//         FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK NOT VALID ANYMORE
//         case FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK: {
//             double tempfloat = 99999.0;
//             armn_compress((byte *)(buf + 5), ni, nj, nk, nbits_in, 2, 1);
//             packfunc(field, buf + 1, buf + 5, nelm, nbits_in + 64 * Max(16, nbits_in), 0, xdf_stride, 0, &tempfloat, &dmin, &dmax);
//             break;
//     }
        case FST_TYPE_REAL_OLD_QUANT: {          // Floating Point, old style packers
            double tempfloat = 99999.0;
            packfunc(field, buf, buf + 3, nelm, nbits_in, 24, xdf_stride, 0, &tempfloat, &dmin , &dmax);
            break;
        }

        case FST_TYPE_UNSIGNED:                // Integers, short integers or bytes (unsigned)
        case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK: {
                int offset = is_type_turbopack(datyp) ? 1 : 0;
                if (XdfShort) {
                    if (is_type_turbopack(datyp)) {
                        int nbytes = armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
                        memcpy(field, buf + offset, nbytes);
                    } else {
                        ier = compact_u_short(field, (void *) NULL, buf + offset, nelm, nbits_in, 0, xdf_stride);
                    }
                }  else if (XdfByte) {
                    if (is_type_turbopack(datyp)) {
                        armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
                        memcpy_16_8((int8_t *)field, (int16_t *)(buf + offset), nelm);
                    } else {
                        ier = compact_u_char(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride);
                    }
                } else {
                    if (is_type_turbopack(datyp)) {
//                       armn_compress((unsigned char *)(buf->data + offset), *ni, *nj, *nk, stdf_entry.nbits, 2, 0);
                        armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
//                       memcpy_16_32((int32_t *)field, (int16_t *)(buf->data + offset), stdf_entry.nbits, nelm);
                        memcpy_16_32((int32_t *)field, (int16_t *)(buf + offset), nbits_in, nelm);
                    } else {
                        ier = compact_u_integer(field, (void *) NULL, buf + offset, nelm, nbits_in, 0, xdf_stride, 0);
                    }
                }
                break;
            }

        case FST_TYPE_SIGNED: {                // Integers, short integers or bytes (signed)
#ifdef use_old_signed_pack_unpack_code
            int32_t *field_out;
            short *s_field_out = (short *)field;
            signed char *b_field_out = (signed char *)field;
            if (XdfShort || XdfByte) {                // need temporary array to unpack
                field_out = malloc(nelm * sizeof(int));
            } else {
                field_out = (int32_t *)field;
            }
            // unpack into field_out
            ier = compact_u_integer(field_out, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
            if (XdfShort) {                           // copy into "short" destination
                for (int i = 0; i < nelm; i++) {
                    s_field_out[i] = field_out[i];
                }
            }
            else if (XdfByte) {                       // copy into "byte" destination
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

        case FST_TYPE_REAL_IEEE:                // IEEE representation
        case FST_TYPE_COMPLEX: {

            register int32_t temp32, *src, *dest;
            if ((downgrade_32) && (nbits_in == 64)) {
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
            } else {
                int32_t f_one = 1;
                int32_t f_zero = 0;
                int32_t f_mode = 2;
                int f_minus_nbits = (-nbits_in);
                f77name(ieeepak)((int32_t *)field, (int32_t *)buf, &nelm, &f_one, &f_minus_nbits, &f_zero, &f_mode);
            }

            break;
        }

        case FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK: {
            // Floating point, new packers
//             printf("Debug+ fstluk - Floating point, new packers (133)\n");
            c_armn_uncompress32((float *)field, (byte *)(buf + 1), ni, nj, nk, nbits_in);
            break;
        }

        case FST_TYPE_REAL:
        case FST_TYPE_REAL | FST_TYPE_TURBOPACK: {
            // Floating point, new packers
            // printf("Debug+ fstluk - Floating point, new packers (6, 134)\n");
            int nbits, header_size, stream_size, p1out, p2out;
            c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni*nj*nk);
            header_size /= sizeof(int32_t);
            if (is_type_turbopack(datyp)) {
                armn_compress((byte *)(buf + 1 + header_size), ni, nj, nk, nbits_in, 2, 1);
                // fprintf(stderr, "Debug+ buf+4+(nbytes/4)-1=%X buf+4+(nbytes/4)=%X \n",
                //    *(buf+4+(nbytes/4)-1), *(buf+4+(nbytes/4)));

                c_float_unpacker((float *)field, (int32_t *)(buf + 1), (int32_t *)(buf + 1 + header_size), nelm, &nbits);
            } else {
                c_float_unpacker((float *)field, (int32_t *)buf, (int32_t *)(buf + header_size), nelm, &nbits);
            }
            break;
        }

        case FST_TYPE_CHAR: {
            // Character
            int nc = (nelm + 3) / 4;
            ier = compact_u_integer(field, (void *) NULL, buf, nc, 32, 0, xdf_stride, 0);
            break;
        }

        case FST_TYPE_STRING:
            // Character string
            // printf("Debug+ fstluk - Character string\n");
            // printf("Debug fstluk compact_u_char xdf_stride=%d nelm =%d\n", xdf_stride, nelm);
            ier = compact_u_char(field, (void *) NULL, buf, nelm, 8, 0, xdf_stride);
            break;

        default:
            Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
            ier = -1;
            goto end ;
    } // switch

    if (has_missing) {
        // Replace "missing" data points with the appropriate values given the type of data (int/float)
        // if nbits = 64 and IEEE , set XdfDouble
        if ((datyp & 0xF) == 5 && nbits_in == 64 ) XdfDouble = 1;
        int sz=(XdfDouble?64:(XdfShort?16:(XdfByte?8:32)));
        // printf("Debug+ fstluk - DecodeMissingValue\n");
//         DecodeMissingValue(field , (ni) * (nj) * (nk) , xdf_datatyp & 0x3F, sz);
        DecodeMissingValue(field , (ni) * (nj) * (nk) , datyp & 0x3F, sz);
    }

    // Upgrade size, if necessary
    if (XdfDouble) {
        const int base_type = base_fst_type(datyp);
        if (base_type == FST_TYPE_REAL_IEEE || base_type == FST_TYPE_REAL) {
            float f[nelm];
            memcpy(f, field, nelm * sizeof(float));
            upgrade_size(field, 64, f, 32, nelm, 0);
        }
        else if (base_type == FST_TYPE_SIGNED || base_type == FST_TYPE_UNSIGNED) {
            int32_t x[nelm];
            memcpy(x, field, nelm * sizeof(int32_t));
            resize_int(field, 64, x, 32, nelm);
        }
    }
end:
    return ier ;
}

int32_t fst98_codec(zmap *map, zmap_block block, zmap_stream stream, int encode){
  struct{
    uint32_t nbits ;      // item size
    uint32_t datyp ;      // item type
    uint64_t dummy ;      // not used for now
  } fst98_codec_args;
  CT_ASSERT(sizeof(fst98_codec_args) == CODEC_ARGS_SIZE, "sizeof(fst98_codec_args) != CODEC_ARGS_SIZE") ;
  int32_t status = 0 ;

  memcpy(&fst98_codec_args, ZMAP_CODEC_ARGS(map), CODEC_ARGS_SIZE) ;
  if(encode){
//     status = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, 1, datyp, data_control, &data_kind) ;
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
