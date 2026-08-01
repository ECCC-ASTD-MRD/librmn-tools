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

// byte to halfword copy
static void memcpy_8_16(int16_t *p16, const int8_t *p8, int nb) {
    for (int i = 0; i < nb; i++) {
        p16[i] = p8[i];
    }
}

// halfword to byte copy
static void memcpy_16_8(int8_t *p8, const int16_t *p16, int nb) {
    for (int i = 0; i < nb; i++) {
        p8[i] = p16[i];
    }
}

static void memcpy_16_32(int32_t *p32, const int16_t *p16, int nbits, int nb) {
    int16_t mask = ~ (0xffff << nbits);        // keep lower nbits bits only
    for (int i = 0; i < nb; i++) {
        p32[i] = p16[i] & mask;
    }
}

static void memcpy_32_16(int16_t *p16, const int32_t * p32, int nbits, int nb) {
    int32_t mask = ~ (0xffffffff << nbits);    // keep lower nbits bits only
    for (int i = 0; i < nb; i++) {
        p16[i] = p32[i] & mask;
    }
}

// remain consistent with fstd98 code
#define use_old_signed_pack_unpack_code
// force Little endian for now
#if ! defined(Little_Endian)
#define Little_Endian YES
#endif

static int dejafait_xdf_1 = 0;
static int dejafait_xdf_2 = 0;

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
  int *new_datyp,
  const int xdf_double,
  const int xdf_short,
  const int xdf_byte,
  const int xdf_stride
) {

    float* field_f = NULL;          // float version of the data
    uint32_t* field_missing = NULL; // data with missing values transformed
    int32_t *buffer = NULL ;
    int nw;  // worst case number of 32 bit words needed for encoded stream
    double dmin = 0.0;
    double dmax = 0.0;
    int nbits = (npak < 0) ? (-npak) : ( Max(1, 32 / Max(1, npak)) );    // npak == 0 or 1 will set nbits to 32

    int is_missing = in_datyp_ori & FSTD_MISSING_FLAG;      //  flag : missing value feature is requested
    int in_datyp = in_datyp_ori & (~FSTD_MISSING_FLAG) ;    // suppress missing value flag (64)
    if (is_type_complex(in_datyp)) {
        if (in_datyp_ori != FST_TYPE_COMPLEX) {
           Lib_Log(APP_LIBFST, APP_WARNING, "%s: compression and/or missing values not supported, data type %d reset to %d (complex)\n",
                __func__, in_datyp_ori, 8);
        }
        is_missing = 0;                     // missing values not supported for complex type
        in_datyp = FST_TYPE_COMPLEX;        // turbo compression not supported for complex type
    }

    // 512+256+32+1 no interference with turbo pack (128) and missing value (64) flags
    int datyp = (in_datyp == FST_TYPE_MAGIC) ? 1 : in_datyp;

    PackFunctionPointer packfunc = ((xdf_double) || (in_datyp == FST_TYPE_MAGIC)) ? compact_p_double : compact_p_float;

    nk = Max(1, nk);

    if (base_fst_type(datyp) == FST_TYPE_REAL_IEEE && nbits < 16) {
        Lib_Log(APP_LIBFST, APP_ERROR, "%s: a truncated IEEE float with less than 16 bits is not allowed\n",
                __func__);
        goto fail ;
    }

    if ( (in_datyp_ori == (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK)) && (nbits > 32) ) {
        Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported for IEEE when nbits > 32, "
                "data type FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK (%d) reset to FST_TYPE_REAL_IEEE (%d) (IEEE)\n", __func__,
                FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK, FST_TYPE_REAL_IEEE);
        // extra compression not supported
        in_datyp = FST_TYPE_REAL_IEEE;
        datyp = in_datyp;
    }

    if (is_type_turbopack(datyp) && (nk > 1)) {
        Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported for 3D data. We will disable it.\n", __func__);
        datyp &= ( ~ FST_TYPE_TURBOPACK ) ;  // cancel turbo compression
    }

    if ( (base_fst_type(in_datyp) == FST_TYPE_REAL_OLD_QUANT) && ((nbits == 31) || (nbits == 32)) ) {
        // R31/R32 to E32 automatic conversion
        datyp = FST_TYPE_REAL_IEEE;
        if (is_type_turbopack(in_datyp)) datyp |= FST_TYPE_TURBOPACK;    // turbo packing is applicable to FST_TYPE_REAL_IEEE
        nbits = 32;
    }

    // flag 64 bit IEEE (type 5 or 8)
    int IEEE_64 = 0;
    if ( (is_type_real(in_datyp))    && (nbits > 32) ) IEEE_64 = 1;    // 64 bits real IEEE
    if ( (is_type_complex(in_datyp)) && (nbits > 32) ) IEEE_64 = 1;    // 64 bits complex IEEE
    if(IEEE_64) nbits = 64 ;
    if ((npak == 0) || (npak == 1)) { datyp = FST_TYPE_BINARY; }        // no compaction, nbits is already 32

    if (is_type_real(datyp) && nbits > 32) {
        datyp = FST_TYPE_REAL_IEEE;                                     // force 64 bits IEEE
        nbits = 64;
    }

    if ((base_fst_type(datyp) == FST_TYPE_REAL)) {                      // type F, new quantification
        if (nbits > 24) {
            if (! dejafait_xdf_1) {
                Lib_Log(APP_LIBFST, APP_INFO, "%s: nbits > 24, using E32 instead of F%2d\n", __func__, nbits);
                dejafait_xdf_1 = 1;
            }
            datyp = FST_TYPE_REAL_IEEE | ( datyp & FST_TYPE_TURBOPACK) ;    // transfer turbo flag
            nbits = 32;
        }
        else if (nbits > 16) {
            if (! dejafait_xdf_2) {
                Lib_Log(APP_LIBFST, APP_INFO, "%s: nbits > 16, using R%2d instead of F%2d\n", __func__, nbits, nbits);
                dejafait_xdf_2 = 1;
            }
            datyp = FST_TYPE_REAL_OLD_QUANT; // type F cannot use more than 16 bits, revert to type R
        }
    }

    // cancel turbo compression if nbits > 16 (except for IEEE reals)
    if ((nbits > 16) && (datyp != (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK))) datyp &= (~FST_TYPE_TURBOPACK);
    // if nbits <= 16 and turbo compression is requested, use FST_TYPE_REAL instead
    if( (nbits <= 16) && (base_fst_type(datyp) == FST_TYPE_REAL_OLD_QUANT) && (datyp & FST_TYPE_TURBOPACK) ){
      datyp = ((datyp >> 6) << 6) | FST_TYPE_REAL ; // replace base type FST_TYPE_REAL_OLD_QUANT with FST_TYPE_REAL
    }

    // handle double real / complex type
    if (is_type_real(datyp) || is_type_complex(datyp)) {
      if (xdf_double || IEEE_64) {
        int _nk = is_type_complex(datyp) ? (2 * nk) : nk ;
        if (nbits <= 32) {                // convert from double to float if nbits not larger than 32
            field_f = malloc(ni * nj * _nk * sizeof(float));
            const double * const field_d = field_in;
            for (int i = 0; i < ni * nj * _nk; i++) {
                field_f[i] = (float)field_d[i];
            }
        }else if (nbits != 64) {
            Lib_Log(APP_LIBFST, APP_WARNING, "%s: Requested %d packed bits for 64-bit reals, but we can only do"
                    " 64 or less than 32. Will use 64 bits.\n", __func__, nbits);
            nbits = 64;
        }
      }
    }

    int header_size, stream_size, p1out, p2out;

    switch (datyp) {
        case FST_TYPE_BINARY:                                         // transparent 32 bit data
        case FST_TYPE_BINARY | FST_TYPE_TURBOPACK:
            nbits = 32 ;
            datyp = FST_TYPE_BINARY ;
            nw = ni*nj*nk ;                                           // (correct length)
            break ;

        case FST_TYPE_REAL:                                           // float
            c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni*nj*nk);
            nw = ((header_size + stream_size) * 8 + 31) / 32;         // (correct length)
            header_size /= sizeof(int32_t);
            stream_size /= sizeof(int32_t);
            break;

        case FST_TYPE_REAL | FST_TYPE_TURBOPACK:                      // float
            c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni*nj*nk);
            nw = ((header_size + stream_size) * 8 + 32 + 31) / 32;    // (worst case length)
            header_size /= sizeof(int32_t);
            stream_size /= sizeof(int32_t);
            break;

        case FST_TYPE_COMPLEX:                                        // IEEE 32/64 bits
            nw = 2 * ((ni*nj*nk * 64) / 32);                          // (worst case length)
            break;

        case FST_TYPE_REAL_OLD_QUANT:                                 // float (old style)
            nw = (ni*nj*nk * nbits + 120 + 31) / 32;                  // (correct length)
            break;

        case FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK:
            // 128 bits (floatpack header + 8), 32 bits (turbo header)
            nw = (128 + 32 + (ni*nj*nk * Max(nbits, 16)) + 31) / 32;  // (worst case length)
            break;

        case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK:
            // 32 bits (extra header)   (worst case)
            nw = ( 32 + (ni*nj*nk * Max(nbits, 16)) +31) / 32;
            break;

        default:                                                      //    (worst case length)
            nw = (ni*nj*nk * nbits + 120 + 32 + 31) / 32;
            break;
    }

    if(VALID_RANGE(field_out)){      // is field_out valid and large enough for encoded stream
      if( (nw + 256) <= RANGE_ELEMENTS(field_out) ) buffer = (int32_t *) RANGE_BOT(field_out) ;    // large enough, use field_out
    }
    int local_buffer = (buffer == NULL) ;
    if(local_buffer){ buffer = (int32_t *) malloc((nw + 256) * sizeof(int32_t)); }

    const uint32_t *field_u32 = field_in;
    if (field_f != NULL) {
        field_u32 = (uint32_t*)field_f;
        packfunc = &compact_p_float; // Use float packing function by default
    }

    // fudge field if missing value feature is used
    int sizefactor = 4;
    if (xdf_byte)  sizefactor = 1;                  // source is a byte array
    if (xdf_short) sizefactor = 2;                  // source is a halfword array
    if (xdf_double || IEEE_64) sizefactor = 8;      // source is a doubleword array
    // put appropriate values into field after allocating it
    if (is_missing) {
        field_missing = malloc(ni*nj*nk * sizefactor);       // allocate temporary field for missing values flaging
        if (EncodeMissingValue(field_missing, field_in, ni*nj*nk, in_datyp, sizefactor*8, nbits) > 0) {
            field_u32 = field_missing;
            if (field_f != NULL) packfunc = &compact_p_double;     // field_f not NULL => previous double to float conversion
        } else {
            field_u32 = (field_f == NULL) ? field_in : field_f;
            Lib_Log(APP_LIBFST, APP_INFO, "%s: NO missing value, data type to %d\n", __func__, datyp);
            is_missing = 0;      // cancel missing data flag
        }
    }

    // nw = best estimate of worst case encoded length (correct length in some cases)
    switch (datyp) {

        case FST_TYPE_BINARY:                   // transparent 32 bit data
        case FST_TYPE_BINARY | FST_TYPE_TURBOPACK: {
            if (is_type_turbopack(datyp)) {
                Lib_Log(APP_LIBFST,APP_WARNING, "%s: extra compression not available, data type reset to FST_TYPE_BINARY (%d)\n", __func__, FST_TYPE_BINARY);
                datyp = FST_TYPE_BINARY;
            }
            nw = ni*nj*nk;
            for (int i = 0; i < nw; i++) { buffer[i] = field_u32[i]; }
            break;                      // nw = actual length of "encoded" stream
        }

        case FST_TYPE_REAL_OLD_QUANT:            // floating point, old style packers
        case FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK: {
            double tempfloat = 99999.0;
//             if (is_type_turbopack(datyp) && (nbits <= 16)) {      // can use turbo encoding scheme
//                 // nbits > 64 flags a different encoding scheme, nbits >> 6 is the effective number of bits to use (minimum 16)
//                 packfunc(field_u32, buffer, buffer+5, ni*nj*nk, nbits + 64 * Max(16, nbits), 0, xdf_stride, 0, &tempfloat, &dmin, &dmax);
//                 int compressed_lng = armn_compress((byte *)(buffer+5), ni, nj, nk, nbits, 1, 1);
//                 if (compressed_lng < 0) {                         // no gain from turbo, repack with offset 24 (120 bit header)
//                     datyp = FST_TYPE_REAL_OLD_QUANT;
//                     packfunc(field_u32,buffer, buffer+3, ni*nj*nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
//                     nw = (ni*nj*nk * nbits + 96 + 24 + 31) / 32;  // data + old style header
//                 } else {                                          // turbo is usable
//                     int nbytes = 16 + compressed_lng;
//                     nw = (nbytes * 8 + 31) / 32;
//                     buffer[0] = nw;
//                     nw ++ ;    // turbo used, bump nw ;
//                 }
//             } else {    // straight quantifier, no turbo, pack with offset 24 (120 bit header)
                packfunc(field_u32, buffer, buffer+3, ni*nj*nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
                nw = (ni*nj*nk * nbits + 96 + 24 + 31) / 32;
//             }
            break;                      // nw = actual length of "encoded" stream
        }

        case FST_TYPE_UNSIGNED:            // integers, short integers or bytes (unsigned)
        case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK: {
            if (is_type_turbopack(datyp)) {
                const int offset = 1;
                if (xdf_short) {               // 16 bits to 16 bits
                    nbits = Min(16, nbits);    // at most 16 bits
                    memcpy((int16_t *)buffer + offset, field_u32, ni*nj*nk * 2);
                } else if (xdf_byte) {         // 8 bits to 16 bits
                    nbits = Min(8, nbits);     // at most 8 bits
                    memcpy_8_16((int16_t *)buffer + offset, (int8_t *)field_u32, ni*nj*nk);
                } else {                       // 32 bits to 16 bits
                    memcpy_32_16((int16_t *)buffer + offset, (const int32_t *)field_u32, nbits, ni*nj*nk);
                }
                int compressed_lng = armn_compress((byte *)buffer + offset, ni, nj, nk, nbits, 1, 0);
                if (compressed_lng < 0) {     // no gain from turbo, repack
                    datyp = FST_TYPE_UNSIGNED;
                    compact_p_integer(field_u32, (void *) NULL, buffer + offset, ni*nj*nk, nbits, 0, xdf_stride, 0);
                } else {
                    int nbytes = 4 + compressed_lng;
                    nw = (nbytes * 8 + 31) / 32;
                    buffer[0] = nw ;
                    nw ++ ;    // turbo used, bump nw ;
                }
            } else {    // straight packing, no turbo
                if (xdf_short) {
                    nbits = Min(16, nbits);    // at most 16 bits
                    compact_p_short(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride);
                } else if (xdf_byte) {
                    nbits = Min(8, nbits);     // at most 8 bits
                    compact_p_char(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride);
                } else {
                    compact_p_integer(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 0);
                }
                nw = ((ni*nj*nk * nbits) + 32 - 1) / 32;                 // recompute nw using possibly revised nbits
            }
            break;                      // nw = actual length of "encoded" stream
        }


        case FST_TYPE_CHAR:            // character data
        case FST_TYPE_CHAR | FST_TYPE_TURBOPACK: {
                int nc = (ni*nj*nk + 3) / 4;
                if (is_type_turbopack(datyp)) {
                    Lib_Log(APP_LIBFST,APP_WARNING, "%s: extra compression not available, data type reset to FST_TYPE_CHAR (%d)\n",
                            __func__, FST_TYPE_CHAR);
                    datyp = FST_TYPE_CHAR;
                }
                compact_p_integer(field_u32, (void *) NULL, buffer, nc, 32, 0, xdf_stride, 0);
                nbits = 8;
                nw = ((ni*nj*nk * nbits) + 32 - 1) / 32;                 // nw = actual length of "encoded" stream (use possibly revised nbits)
            }
            break;

        case FST_TYPE_SIGNED:            // integers, short integers or bytes (signed)
        case FST_TYPE_SIGNED | FST_TYPE_TURBOPACK: {
            if (is_type_turbopack(datyp)) {
                Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported, data type reset to FST_TYPE_SIGNED (%d)\n", __func__, is_missing | FST_TYPE_SIGNED);
                datyp = FST_TYPE_SIGNED;
            }
            datyp = is_missing | FST_TYPE_SIGNED;
#ifdef use_old_signed_pack_unpack_code
            if (xdf_short || xdf_byte) {
//                 int32_t* field3 = (int *)malloc(ni*nj*nk*sizeof(int));
                int32_t field3[ni*nj*nk] ;
                int16_t *s_field = (int16_t *)field_u32;
                int8_t  *b_field = (int8_t  *)field_u32;
                if (xdf_short) for (int i = 0; i < ni*nj*nk;i++) { field3[i] = s_field[i]; };    // expand to int32_t
                if (xdf_byte)  for (int i = 0; i < ni*nj*nk;i++) { field3[i] = b_field[i]; };    // expand to int32_t
                compact_p_integer(field3, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 1);
//                 free(field3); 
            }else{
              compact_p_integer(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 1);
            }
#else
            if (xdf_short) {
                compact_p_short(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 7);
            } else if (xdf_byte) {
                compact_p_char(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 11);
            } else {
                compact_p_integer(field_u32, (void *) NULL, buffer, ni*nj*nk, nbits, 0, xdf_stride, 1);
            }
#endif
            break;
        }
        case FST_TYPE_REAL_IEEE:            // IEEE and IEEE complex representation
        case FST_TYPE_COMPLEX:
        case FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK:
        case FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK: {
                int32_t f_ni = (int32_t) ni;
                int32_t f_njnk = nj * nk;
                int32_t f_zero = 0;
                int32_t f_one = 1;
                int32_t f_minus_nbits = (- nbits);
                if (datyp == (FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK)) {
                    Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not available for complex data, data typereset to FST_TYPE_COMPLEX (%d)\n",
                            __func__, FST_TYPE_COMPLEX);
                    datyp = FST_TYPE_COMPLEX;
                }
                if (datyp == (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK)) {    // use turbo compression scheme
                    int compressed_lng = c_armn_compress32((byte *)&(buffer[1]), (float *)field_u32, ni, nj, nk, nbits);
                    if (compressed_lng < 0) {
                        datyp = FST_TYPE_REAL_IEEE;
                        f77name(ieeepak)((int32_t *)field_u32, (int32_t *)buffer, &f_ni, &f_njnk, &f_minus_nbits, &f_zero, &f_one);
                    } else {
                        int nbytes = 16 + compressed_lng;
                        nw = (nbytes * 8 + 31) / 32;
                        buffer[0] = nw;
                        nw ++ ;    // turbo used, bump nw ;
                    }
                } else {
                    if (datyp == FST_TYPE_COMPLEX) f_ni = f_ni * 2;
                    f77name(ieeepak)((int32_t*)field_u32, (int32_t *)buffer, &f_ni, &f_njnk, &f_minus_nbits, &f_zero, &f_one);
                }
            }
            break;

        case FST_TYPE_REAL:
        case FST_TYPE_REAL | FST_TYPE_TURBOPACK:            // floating point, new packers
            if (is_type_turbopack(datyp) && (nbits <= 16)) {    // use turbo compression scheme
                c_float_packer((float *)field_u32, nbits, buffer + 1, buffer + 1 + header_size, ni*nj*nk);
                int compressed_lng = armn_compress((byte *)(buffer+1+header_size), ni, nj, nk, nbits, 1, 1);
                if (compressed_lng < 0) {
                    datyp = FST_TYPE_REAL;
                    c_float_packer((float *)field_u32, nbits, (int32_t *)buffer, (int32_t *)(buffer+header_size), ni*nj*nk);
                } else {
                    int nbytes = 16 + (header_size*4) + compressed_lng;
                    nw = (nbytes * 8 + 31) / 32;
                    buffer[0] = nw;
                    nw ++ ;    // turbo used, bump nw ;
                }
            } else {
                c_float_packer((float *)field_u32, nbits, (int32_t *)buffer, (int32_t *)(buffer+header_size), ni*nj*nk);
            }
            break;


        case FST_TYPE_STRING:
        case FST_TYPE_STRING | FST_TYPE_TURBOPACK:            // character string
            if (is_type_turbopack(datyp)) {
                Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not available, data typereset to FST_TYPE_STRING (%d)\n",
                        __func__, FST_TYPE_STRING);
                datyp = FST_TYPE_STRING;
            }
            compact_p_char(field_u32, (void *) NULL, buffer, ni*nj*nk, 8, 0, xdf_stride);
            break;

        default:
            Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
            goto fail ;
    } // end switch

    if (field_f       != NULL) free(field_f);
    if (field_missing != NULL) free(field_missing);

    *new_datyp = datyp ;
    return (RANGE(int32_t)) { buffer, (buffer + nw) } ;
fail :
   if(buffer && local_buffer) free(buffer) ;   // free buffer if it was allocated locally
   return RANGE_NULL(int32_t) ;
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
  int datyp,
  int nbits_in,
  int downgrade_32,
  int xdf_double,
  int xdf_short,
  int xdf_byte,
  int xdf_stride
) {
    uint32_t *field = data_out;
    int ier = 0 ;

    // Get missing data flag
    int has_missing = datyp & FSTD_MISSING_FLAG;
    // Suppress missing data flag
    datyp = datyp & ~FSTD_MISSING_FLAG;
//     int xdf_datatyp = datyp;

    UnpackFunctionPointer packfunc = xdf_double ? &compact_u_double : &compact_u_float;
    double dmin=0.0, dmax=0.0;

    // Ignore larger ni and nj bits for XDF (so only ni_a, nj_a)
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
    switch (datyp) {
        case FST_TYPE_BINARY: {            // Raw binary
            int lngw = ((nelm * nbits_in) + 32 - 1) / 32;
            for (int i = 0; i < lngw; i++) {
                field[i] = buf[i];
            }
            break;
        }

        case FST_TYPE_REAL_OLD_QUANT:            // Floating Point, old style packers
        case FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK: {
            double tempfloat = 99999.0;
//             if (is_type_turbopack(datyp)) {
//                 armn_compress((byte *)(buf + 5), ni, nj, nk, nbits_in, 2, 1);
//                 packfunc(field, buf + 1, buf + 5, nelm, nbits_in + 64 * Max(16, nbits_in), 0, xdf_stride, 0, &tempfloat, &dmin, &dmax);
//             } else {
                packfunc(field, buf, buf + 3, nelm, nbits_in, 24, xdf_stride, 0, &tempfloat, &dmin , &dmax);
//             }
            break;
        }

        case FST_TYPE_UNSIGNED:                // Integers, short integers or bytes (unsigned)
        case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK: {
                int offset = is_type_turbopack(datyp) ? 1 : 0;
                if (xdf_short) {
                    if (is_type_turbopack(datyp)) {
                        int nbytes = armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
                        memcpy(field, buf + offset, nbytes);
                    } else {
                        ier = compact_u_short(field, (void *) NULL, buf + offset, nelm, nbits_in, 0, xdf_stride);
                    }
                }  else if (xdf_byte) {
                    if (is_type_turbopack(datyp)) {
                        armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
                        memcpy_16_8((int8_t *)field, (int16_t *)(buf + offset), nelm);
                    } else {
                        ier = compact_u_char(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride);
                    }
                } else {
                    if (is_type_turbopack(datyp)) {
                        armn_compress((byte *)(buf + offset), ni, nj, nk, nbits_in, 2, 0);
                        memcpy_16_32((int32_t *)field, (int16_t *)(buf + offset), nbits_in, nelm);
                    } else {
                        ier = compact_u_integer(field, (void *) NULL, buf + offset, nelm, nbits_in, 0, xdf_stride, 0);
                    }
                }
                break;
            }

        case FST_TYPE_CHAR: {
            // Character
            int nc = (nelm + 3) / 4;
            ier = compact_u_integer(field, (void *) NULL, buf, nc, 32, 0, xdf_stride, 0);
            break;
        }


        case FST_TYPE_SIGNED: {
            // Signed integer
#ifdef use_old_signed_pack_unpack_code
            int32_t *field_out;
            short *s_field_out = (short *)field;
            signed char *b_field_out = (signed char *)field;
            if (xdf_short || xdf_byte) {
                field_out = malloc(nelm * sizeof(int));
            } else {
                field_out = (int32_t *)field;
            }
            ier = compact_u_integer(field_out, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
            if (xdf_short) {
                for (int i = 0; i < nelm; i++) {
                    s_field_out[i] = field_out[i];
                }
            }
            else if (xdf_byte) {
                for (int i = 0; i < nelm; i++) {
                    b_field_out[i] = field_out[i];
                }
            }
            if (field_out != (int32_t*)field) free(field_out);
#else
            // fprintf(stderr, "NEW UNPACK CODE ======================================\n");
            if (xdf_short) {
                ier = compact_u_short(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 8);
            } else if (xdf_byte) {
                ier = compact_u_char(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 12);
            } else {
                ier = compact_u_integer(field, (void *) NULL, buf, nelm, nbits_in, 0, xdf_stride, 1);
            }
#endif
            break;
        }
        case FST_TYPE_REAL_IEEE:
        case FST_TYPE_COMPLEX: {
            // IEEE representation
            // printf("Debug+ fstluk - IEEE representation\n");
            register int32_t temp32, *src, *dest;
            if ((downgrade_32) && (nbits_in == 64)) {
                // Downgrade 64 bit to 32 bit
                float * ptr_real = (float *) field;
                double * ptr_double = (double *) buf;
#if defined(Little_Endian)
                src = (int32_t *) buf;
                dest = (int32_t *) buf;
                for (int i = 0; i < nelm; i++) {
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
                int npak = nbits_in;
                f77name(ieeepak)((int32_t *)field, (int32_t *)buf, &nelm, &f_one, &npak, &f_zero, &f_mode);
            }

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

        case FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK: {
            // Floating point, new packers
            // printf("Debug+ fstluk - Floating point, new packers (133)\n");
            c_armn_uncompress32((float *)field, (byte *)(buf + 1), ni, nj, nk, nbits_in);
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
        // if nbits = 64 and IEEE , set xdf_double
        if ((datyp & 0xF) == 5 && nbits_in == 64 ) xdf_double = 1;
        int sz=(xdf_double?64:(xdf_short?16:(xdf_byte?8:32)));
        // printf("Debug+ fstluk - DecodeMissingValue\n");
//         DecodeMissingValue(field , (ni) * (nj) * (nk) , xdf_datatyp & 0x3F, sz);
        DecodeMissingValue(field , (ni) * (nj) * (nk) , datyp & 0x3F, sz);
    }

    // Upgrade size, if necessary
//     if (xdf_double && (stdf_entry.dasiz < 64 && stdf_entry.dasiz > 0)) {
    if (xdf_double) {
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
