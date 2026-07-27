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
#include <stdint.h>
#include <string.h>

#include <App.h>

// #include <rmn/rpnmacros.h>
#include <rmn/fst98.h>
#include <rmn/fst_missing.h>

// #include <armn_compress.h>
// extern int xdf_double;
// extern int xdf_short;
// extern int xdf_byte;
// extern int xdf_stride;

extern int FTN_Bitmot;

#define Max(x,y) ((x > y) ? x : y)
#define Min(x,y) ((x < y) ? x : y)

// byte to halfword copy
static void memcpy_8_16(int16_t *p16, const int8_t *p8, int nb) {
    for (int i = 0; i < nb; i++) {
        *p16++ = *p8++;
    }
}

// halfword to byte copy
// static void memcpy_16_8(int8_t *p8, const int16_t *p16, int nb) {
//     for (int i = 0; i < nb; i++) {
//         *p8++ = *p16++;
//     }
// }

// static void memcpy_16_32(int32_t *p32, const int16_t *p16, int nbits, int nb) {
//     int16_t mask = ~ (0xffff << nbits);    // keep lower nbits bits
//     for (int i = 0; i < nb; i++) {
//         *p32++ = *p16++ & mask;
//     }
// }

static void memcpy_32_16(int16_t *p16, const int32_t * p32, int nbits, int nb) {
    int32_t mask = ~ (0xffffffff << nbits);    // keep lower nbits bits
    for (int i = 0; i < nb; i++) {
        *p16++ = *p32++ & mask;
    }
}

// remain consistent with fstd98 code
#define use_old_signed_pack_unpack_code

void c_float_packer_params(int32_t *header_size, int32_t *stream_size, int32_t *p1, int32_t *p2, int32_t npts);
int32_t c_float_packer(float *source, int32_t nbits, int32_t *header, int32_t *stream, int32_t npts);

void f77name(ieeepak)(int32_t *IFLD, int32_t *IPK, const int32_t *NI, const int32_t *NJ, const int32_t *NPAK, const int32_t *serpas, const int32_t *mode);

int c_armn_compress32(unsigned char *, float *, int, int, int, int);
int armn_compress(unsigned char *fld, int ni, int nj, int nk, int nbits, int op_code, const int swap_stream);

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

int compact_p_char(
    const void * const unpackedArrayOfBytes,
    void * const packedHeader,
    void * const packedArrayOfInt,
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

static int dejafait_xdf_1 = 0;
static int dejafait_xdf_2 = 0;
const int bitmot = 32;

uint32_t *fst98_encode(
    //! [in] Field to encode
    const void * const field_in,
    //! [in] Number of bits kept for the elements of the field
    const int npak,
    //! [in] First dimension of the data field
    const int ni,
    //! [in] Second dimension of the data field
    const int nj,
    //! [in] Third dimension of the data field
    const int nk,
    //! [in] Data type of elements
    const int in_datyp_ori,
  const int xdf_double,
  const int xdf_short,
  const int xdf_byte,
  const int xdf_stride
) {
//     (void)work; // unused

    float* field_f = NULL; // float version of the data
    uint32_t* field_missing = NULL; // data with missing values transformed
    uint32_t *buffer = NULL ;

    // will be cancelled later if not supported or no missing values detected
    //  missing value feature used flag
    int is_missing = in_datyp_ori & FSTD_MISSING_FLAG;
    // suppress missing value flag (64)
    int in_datyp = in_datyp_ori & ~FSTD_MISSING_FLAG;
    if (is_type_complex(in_datyp)) {
        if (in_datyp_ori != FST_TYPE_COMPLEX) {
           Lib_Log(APP_LIBFST, APP_WARNING, "%s: compression and/or missing values not supported, data type %d reset to %d (complex)\n",
                __func__, in_datyp_ori, 8);
        }
        // missing values not supported for complex type
        is_missing = 0;
        // extra compression not supported for complex type
        in_datyp = FST_TYPE_COMPLEX;
    }

    // 512+256+32+1 no interference with turbo pack (128) and missing value (64) flags
    int datyp = in_datyp == FST_TYPE_MAGIC ? 1 : in_datyp;

    PackFunctionPointer packfunc = (xdf_double) || (in_datyp == FST_TYPE_MAGIC) ? &compact_p_double : &compact_p_float;
    double dmin = 0.0;
    double dmax = 0.0;

    int nbits;
    if (npak == 0 || npak == 1) {
        nbits = FTN_Bitmot;
    } else {
        nbits = (npak < 0) ? -npak : Max(1, FTN_Bitmot / Max(1, npak));
    }
    int _nk = Max(1, nk);

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

    if (is_type_turbopack(datyp) && _nk > 1) {
        Lib_Log(APP_LIBFST, APP_WARNING, "%s: Turbo compression not supported for 3D data. We will disable it.\n", __func__);
        datyp &= FST_TYPE_TURBOPACK;
    }

    if ((base_fst_type(in_datyp) == FST_TYPE_REAL_OLD_QUANT) && ((nbits == 31) || (nbits == 32)) /*&& !image_mode_copy*/) {
        // R32 to E32 automatic conversion
        datyp = FST_TYPE_REAL_IEEE;
        if (is_type_turbopack(in_datyp)) datyp |= FST_TYPE_TURBOPACK;
        nbits = 32;
    }

    // flag 64 bit IEEE (type 5 or 8)
    int IEEE_64 = 0;
    // 64 bits real IEEE
    if ( (is_type_real(in_datyp)) && (nbits == 64) ) IEEE_64 = 1;
    // 64 bits complex IEEE
    if ( is_type_complex(in_datyp) && (nbits == 64) ) IEEE_64 = 1;

    if ((npak == 0) || (npak == 1)) {
        // no compaction
        datyp = FST_TYPE_BINARY;
    }

    if (is_type_real(datyp) && nbits > 32) {
        datyp = FST_TYPE_REAL_IEEE;
        nbits = 64;
    }

    if ((base_fst_type(datyp) == FST_TYPE_REAL)) { 
        if (nbits > 24) {
            if (! dejafait_xdf_1) {
                Lib_Log(APP_LIBFST, APP_INFO, "%s: nbits > 24, writing E32 instead of F%2d\n", __func__, nbits);
                dejafait_xdf_1 = 1;
            }
            datyp = FST_TYPE_REAL_IEEE | (is_type_turbopack(datyp) ? FST_TYPE_TURBOPACK : 0);
            nbits = 32;
        }
        else if (nbits > 16) {
            if (! dejafait_xdf_2) {
                Lib_Log(APP_LIBFST, APP_INFO, "%s: nbits > 16, writing R%2d instead of F%2d\n", __func__, nbits, nbits);
                dejafait_xdf_2 = 1;
            }
            datyp = FST_TYPE_REAL_OLD_QUANT; // No turbopack for R with >16 bits
        }
    }

    // no extra compression if nbits > 16 (except for IEEE reals)
    if ((nbits > 16) && (datyp != (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK))) datyp = base_fst_type(datyp);

    // Determine data_nbits (uncompressed datatype size)
    int8_t data_nbits = 0;
    if (is_type_real(datyp) || is_type_complex(datyp)) {
        data_nbits = (xdf_double || IEEE_64) ? 64 : 32;
        if (data_nbits == 64) {
            if (nbits <= 32) {
                // We convert now from double to float
                data_nbits = 32;
                field_f = malloc(ni * nj * _nk * sizeof(float));
                const double * const field_d = field_in;
                for (int i = 0; i < ni * nj * _nk; i++) {
                    field_f[i] = (float)field_d[i];
                }
            }
            else if (nbits != 64) {
                Lib_Log(APP_LIBFST, APP_WARNING, "%s: Requested %d packed bits for 64-bit reals, but we can only do"
                        " 64 or less than 32. Will store 64 bits.\n", __func__, nbits);
                nbits = 64;
            }
        }
    } else if (is_type_integer(datyp)) {
        data_nbits = xdf_byte   ?  8 :
                     xdf_short  ? 16 :
                     xdf_double ? 64 :
                                  32;
    }

    int minus_nbits = -nbits;
    int header_size;
    int stream_size;
    int nw;  // number of 32 bit words
    switch (datyp) {
        case FST_TYPE_REAL: {
            int p1out;
            int p2out;
            c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni * nj * _nk);
            nw = ((header_size + stream_size) * 8 + 31) / 32;
            header_size /= sizeof(int32_t);
            stream_size /= sizeof(int32_t);
            break;
        }

        case FST_TYPE_COMPLEX:
            nw = 2 * ((ni * nj * _nk * nbits + 31) / 32);
            break;

        case FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK:
            // 128 bits (floatpack header + 8), 32 bits (extra header)
            nw = (128 + 32 + (ni * nj * _nk * Max(nbits, 16)) + 31) / 32;
            break;

        case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK:
            // 32 bits (extra header)
            nw = ( 32 + (ni * nj * _nk * Max(nbits, 16)) +31) / 32;
            break;

        case FST_TYPE_REAL | FST_TYPE_TURBOPACK:
            int p1out;
            int p2out;
            c_float_packer_params(&header_size, &stream_size, &p1out, &p2out, ni * nj * _nk);
            nw = ((header_size + stream_size) * 8 + 32 + 31) / 32;
            stream_size /= sizeof(int32_t);
            header_size /= sizeof(int32_t);
            break;

        default:
            nw = (ni * nj * _nk * nbits + 120 + 31) / 32;
            break;
    }

    buffer = malloc((nw + 256) * sizeof(uint32_t));
//     uint32_t buffer[nw + 256] ;

    const uint32_t * field_u32 = field_in;
    if (field_f != NULL) {
        field_u32 = (uint32_t*)field_f;
        packfunc = &compact_p_float; // Use corresponding packing function
    }
        // time to fudge field if missing value feature is used

        // number of bytes per data item
        int sizefactor = 4;
        if (xdf_byte)  sizefactor = 1;
        if (xdf_short) sizefactor = 2;
        if (xdf_double | IEEE_64) sizefactor = 8;
        // put appropriate values into field after allocating it
        if (is_missing) {
            field_missing = malloc(ni * nj * _nk * sizefactor);
            if (EncodeMissingValue(field_missing, field_in, ni * nj * _nk, in_datyp, sizefactor*8, nbits) > 0) {
                field_u32 = field_missing;
                if (field_f != NULL) packfunc = &compact_p_double;
            } else {
                field_u32 = field_f == NULL ? field_in : field_f;
                Lib_Log(APP_LIBFST, APP_INFO, "%s: NO missing value, data type to %d\n", __func__, datyp);
                // cancel missing data flag in data type
//                 stdf_entry->datyp = datyp;
                is_missing = 0;
            }
        }

        switch (datyp) {

            case FST_TYPE_BINARY:
            case FST_TYPE_BINARY | FST_TYPE_TURBOPACK: {
                // transparent mode
                if (is_type_turbopack(datyp)) {
                    Lib_Log(APP_LIBFST,APP_WARNING, "%s: extra compression not available, data type reset to FST_TYPE_BINARY (%d)\n", __func__, FST_TYPE_BINARY);
                    datyp = FST_TYPE_BINARY;
//                     stdf_entry->datyp = datyp;
                }
                int lngw = ((ni * nj * _nk * nbits) + bitmot - 1) / bitmot;
                for (int i = 0; i < lngw; i++) {
//                     buffer->data[keys_len+i] = field_u32[i];
                    buffer[i] = field_u32[i];
                }
                break;
            }

            case FST_TYPE_REAL_OLD_QUANT:
            case FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK: {
                // floating point
                double tempfloat = 99999.0;
                if (is_type_turbopack(datyp) && (nbits <= 16)) {
                    // use an additional compression scheme
                    // nbits>64 flags a different packing
//                     packfunc(field_u32, &(buffer->data[keys_len+1]), &(buffer->data[keys_len+5]),
//                         ni * nj * _nk, nbits + 64 * Max(16, nbits), 0, xdf_stride, 0, &tempfloat, &dmin, &dmax);
                    packfunc(field_u32, &(buffer[1]), &(buffer[5]),
                        ni * nj * _nk, nbits + 64 * Max(16, nbits), 0, xdf_stride, 0, &tempfloat, &dmin, &dmax);
//                     int compressed_lng = armn_compress((unsigned char *)&(buffer->data[keys_len+5]), ni, nj, _nk, nbits, 1, 1);
                    int compressed_lng = armn_compress((unsigned char *)&(buffer[5]), ni, nj, _nk, nbits, 1, 1);
                    if (compressed_lng < 0) {
//                         stdf_entry->datyp = FST_TYPE_REAL_OLD_QUANT;
                        datyp = FST_TYPE_REAL_OLD_QUANT;
//                         packfunc(field_u32, &(buffer->data[keys_len]), &(buffer->data[keys_len+3]),
//                             ni * nj * _nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
                        packfunc(field_u32, &(buffer[0]), &(buffer[3]),
                            ni * nj * _nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
                    } else {
                        int nbytes = 16 + compressed_lng;
                        // fprintf(stderr, "Debug+ apres armn_compress nbytes=%d\n", nbytes);
                        nw = (nbytes * 8 + 31) / 32;
//                         buffer->data[keys_len] = nw;
                        buffer[0] = nw;
                        // fprintf(stderr, "Debug+ pack buffer->data[keys_len]=%d\n", buffer->data[keys_len]);
//                         buffer->nbits = (keys_len + nw) * bitmot;
                    }
                } else {
//                     packfunc(field_u32, &(buffer->data[keys_len]), &(buffer->data[keys_len+3]),
//                         ni * nj * _nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
                    packfunc(field_u32, &(buffer[0]), &(buffer[3]),
                        ni * nj * _nk, nbits, 24, xdf_stride, 0, &tempfloat, &dmin, &dmax);
                }
                break;
            }

            case FST_TYPE_UNSIGNED:
            case FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK: {
                // integer, short integer or byte stream
                if (is_type_turbopack(datyp)) {
                    const int offset = 1;
                    if (xdf_short) {
//                         stdf_entry->nbits = Min(16, nbits);
//                         nbits = stdf_entry->nbits;
                        nbits = Min(16, nbits);
//                         memcpy(&(buffer->data[keys_len+offset]), field_u32, ni * nj * _nk * 2);
                        memcpy(&(buffer[offset]), field_u32, ni * nj * _nk * 2);
                    } else if (xdf_byte) {
//                         stdf_entry->nbits = Min(8, nbits);
//                         nbits = stdf_entry->nbits;
                        nbits = Min(8, nbits);
//                         memcpy_8_16((int16_t *)&(buffer->data[keys_len+offset]), (int8_t *)field_u32, ni * nj * _nk);
                        memcpy_8_16((int16_t *)&(buffer[offset]), (int8_t *)field_u32, ni * nj * _nk);
                    } else {
//                         memcpy_32_16((short *)&(buffer->data[keys_len+offset]), (const int32_t *)field_u32, nbits, ni * nj * _nk);
                        memcpy_32_16((short *)&(buffer[offset]), (const int32_t *)field_u32, nbits, ni * nj * _nk);
                    }
//                     int compressed_lng = armn_compress((unsigned char *)&(buffer->data[keys_len+offset]), ni, nj, _nk, nbits, 1, 0);
                    int compressed_lng = armn_compress((unsigned char *)&(buffer[offset]), ni, nj, _nk, nbits, 1, 0);
                    if (compressed_lng < 0) {
//                         stdf_entry->datyp = FST_TYPE_UNSIGNED;
                        datyp = FST_TYPE_UNSIGNED;
//                         compact_p_integer(field_u32, (void *) NULL, &(buffer->data[keys_len + offset]),
//                             ni * nj * _nk, nbits, 0, xdf_stride, 0);
                        compact_p_integer(field_u32, (void *) NULL, &(buffer[offset]),
                            ni * nj * _nk, nbits, 0, xdf_stride, 0);
                    } else {
                        int nbytes = 4 + compressed_lng;
                        // fprintf(stderr, "Debug+ fstecr armn_compress compressed_lng=%d\n", compressed_lng);
                        nw = (nbytes * 8 + 31) / 32;
                        buffer[0] = nw ;
//                         buffer->data[keys_len] = nw;
//                         buffer->nbits = (keys_len + nw) * bitmot;
                    }
                } else {
                    if (xdf_short) {
                        nbits = Min(16, nbits);
//                         stdf_entry->nbits = nbits;
//                         compact_p_short(field_u32, (void *) NULL, &(buffer->data[keys_len]),
                        compact_p_short(field_u32, (void *) NULL, &(buffer[0]),
                            ni * nj * _nk, nbits, 0, xdf_stride);
                    } else if (xdf_byte) {
                        nbits = Min(8, nbits);
//                         stdf_entry->nbits = nbits;
//                         compact_p_char(field_u32, (void *) NULL, &(buffer->data[keys_len]),
                        compact_p_char(field_u32, (void *) NULL, &(buffer[0]),
                            ni * nj * _nk, nbits, 0, xdf_stride);
                    } else {
//                         compact_p_integer(field_u32, (void *) NULL, &(buffer->data[keys_len]),
                        compact_p_integer(field_u32, (void *) NULL, &(buffer[0]),
                            ni * nj * _nk, nbits, 0, xdf_stride, 0);
                    }
                }
                break;
            }


            case FST_TYPE_CHAR:
            case FST_TYPE_CHAR | FST_TYPE_TURBOPACK:
                // character
                {
                    int nc = (ni * nj + 3) / 4;
                    if (is_type_turbopack(datyp)) {
                        Lib_Log(APP_LIBFST,APP_WARNING, "%s: extra compression not available, data type reset to FST_TYPE_CHAR (%d)\n",
                                __func__, FST_TYPE_CHAR);
                        datyp = FST_TYPE_CHAR;
//                         stdf_entry->datyp = datyp;
                    }
//                     compact_p_integer(field_u32, (void *) NULL, &(buffer->data[keys_len]), nc, 32, 0, xdf_stride, 0);
                    compact_p_integer(field_u32, (void *) NULL, &(buffer[0]), nc, 32, 0, xdf_stride, 0);
//                     stdf_entry->nbits = 8;
                    nbits = 8;
                }
                break;

            case FST_TYPE_SIGNED:
            case FST_TYPE_SIGNED | FST_TYPE_TURBOPACK: {
                // signed integer
                if (is_type_turbopack(datyp)) {
                    Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not supported, data typereset to FST_TYPE_SIGNED (%d)\n", __func__, is_missing | FST_TYPE_SIGNED);
                    datyp = FST_TYPE_SIGNED;
                }
                // turbo compression not supported for this type, revert to normal mode
//                 stdf_entry->datyp = is_missing | FST_TYPE_SIGNED;
                datyp = is_missing | FST_TYPE_SIGNED;
#ifdef use_old_signed_pack_unpack_code
                // fprintf(stderr, "OLD PACK CODE======================================\n");
                int32_t* field3 = (int32_t *)field_u32;
                if (xdf_short || xdf_byte) {
                    field3 = (int *)malloc(ni * nj * _nk*sizeof(int));
                    short * s_field = (short *)field_u32;
                    signed char * b_field = (signed char *)field_u32;
                    if (xdf_short) for (int i = 0; i < ni * nj * _nk;i++) { field3[i] = s_field[i]; };
                    if (xdf_byte)  for (int i = 0; i < ni * nj * _nk;i++) { field3[i] = b_field[i]; };
                }
//                 compact_p_integer(field3, (void *) NULL, &(buffer->data[keys_len]), ni * nj * _nk,
                compact_p_integer(field3, (void *) NULL, &(buffer[0]), ni * nj * _nk,
                    nbits, 0, xdf_stride, 1);
                if (field3 != (int32_t*)field_u32) free(field3);
#else
                // fprintf(stderr, "NEW PACK CODE======================================\n");
                if (xdf_short) {
//                     compact_p_short(field_u32, (void *) NULL, &(buffer->data[keys_len]), ni * nj * _nk,
                    compact_p_short(field_u32, (void *) NULL, &(buffer[0]), ni * nj * _nk,
                        nbits, 0, xdf_stride, 7);
                } else if (xdf_byte) {
//                     compact_p_char(field_u32, (void *) NULL, &(buffer->data[keys_len]), ni * nj * _nk,
                    compact_p_char(field_u32, (void *) NULL, &(buffer[0]), ni * nj * _nk,
                        nbits, 0, xdf_stride, 11);
                } else {
//                     compact_p_integer(field_u32, (void *) NULL, &(buffer->data[keys_len]), ni * nj * _nk,
                    compact_p_integer(field_u32, (void *) NULL, &(buffer[0]), ni * nj * _nk,
                        nbits, 0, xdf_stride, 1);
                }
#endif
                break;
            }
            case FST_TYPE_REAL_IEEE:
            case FST_TYPE_COMPLEX:
            case FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK:
            case FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK:
                // IEEE and IEEE complex representation
                {
                    int32_t f_ni = (int32_t) ni;
                    int32_t f_njnk = nj * _nk;
                    int32_t f_zero = 0;
                    int32_t f_one = 1;
                    int32_t f_minus_nbits = (int32_t) minus_nbits;
                    if (datyp == (FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK)) {
                        Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not available for complex data, data typereset to FST_TYPE_COMPLEX (%d)\n",
                                __func__, FST_TYPE_COMPLEX);
                        datyp = FST_TYPE_COMPLEX;
//                         stdf_entry->datyp = datyp;
                    }
                    if (datyp == (FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK)) {
                        // use an additionnal compression scheme
//                         int compressed_lng = c_armn_compress32((unsigned char *)&(buffer->data[keys_len+1]), (float *)field_u32, ni, nj, _nk, nbits);
                        int compressed_lng = c_armn_compress32((unsigned char *)&(buffer[1]), (float *)field_u32, ni, nj, _nk, nbits);
                        if (compressed_lng < 0) {
//                             stdf_entry->datyp = FST_TYPE_REAL_IEEE;
                            datyp = FST_TYPE_REAL_IEEE;
//                             f77name(ieeepak)((int32_t *)field_u32, (int32_t *)&(buffer->data[keys_len]), &f_ni, &f_njnk, &f_minus_nbits,
                            f77name(ieeepak)((int32_t *)field_u32, (int32_t *)&(buffer[0]), &f_ni, &f_njnk, &f_minus_nbits,
                                &f_zero, &f_one);
                        } else {
                            int nbytes = 16 + compressed_lng;
                            nw = (nbytes * 8 + 31) / 32;
//                             buffer->data[keys_len] = nw;
                            buffer[0] = nw;
//                             buffer->nbits = (keys_len + nw) * bitmot;
                        }
                    } else {
                        if (datyp == FST_TYPE_COMPLEX) f_ni = f_ni * 2;
//                         f77name(ieeepak)((int32_t*)field_u32, (int32_t *)&(buffer->data[keys_len]), &f_ni, &f_njnk, &f_minus_nbits,
                        f77name(ieeepak)((int32_t*)field_u32, (int32_t *)&(buffer[0]), &f_ni, &f_njnk, &f_minus_nbits,
                            &f_zero, &f_one);
                    }
                }
                break;

            case FST_TYPE_REAL:
            case FST_TYPE_REAL | FST_TYPE_TURBOPACK:
                // floating point, new packers

                if (is_type_turbopack(datyp) && (nbits <= 16)) {
                    // use an additional compression scheme
//                     c_float_packer((float *)field_u32, nbits, (int32_t *)&(buffer->data[keys_len+1]),
//                                    (int32_t *)&(buffer->data[keys_len+1+header_size]), ni * nj * _nk);
                    c_float_packer((float *)field_u32, nbits, (int32_t *)&(buffer[1]),
                                   (int32_t *)&(buffer[1+header_size]), ni * nj * _nk);
//                     int compressed_lng = armn_compress((unsigned char *)&(buffer->data[keys_len+1+header_size]), ni, nj, _nk, nbits, 1, 1);
                    int compressed_lng = armn_compress((unsigned char *)&(buffer[1+header_size]), ni, nj, _nk, nbits, 1, 1);
                    if (compressed_lng < 0) {
//                         stdf_entry->datyp = FST_TYPE_REAL;
                        datyp = FST_TYPE_REAL;
//                         c_float_packer((float *)field_u32, nbits, (int32_t *)&(buffer->data[keys_len]),
//                                         (int32_t *)&(buffer->data[keys_len+header_size]), ni * nj * _nk);
                        c_float_packer((float *)field_u32, nbits, (int32_t *)&(buffer[0]),
                                        (int32_t *)&(buffer[header_size]), ni * nj * _nk);
                    } else {
                        int nbytes = 16 + (header_size*4) + compressed_lng;
                        // fprintf(stderr, "Debug+ apres armn_compress nbytes=%d\n", nbytes);
                        nw = (nbytes * 8 + 31) / 32;
//                         buffer->data[keys_len] = nw;
                        buffer[0] = nw;
                        // fprintf(stderr, "Debug+ pack buffer->data[keys_len]=%d\n", buffer->data[keys_len]);
//                         buffer->nbits = (keys_len + nw) * bitmot;
                    }
                } else {
//                     c_float_packer((float *)field_u32, nbits, (int32_t *)&(buffer->data[keys_len]),
//                                    (int32_t *)&(buffer->data[keys_len+header_size]), ni * nj * _nk);
                    c_float_packer((float *)field_u32, nbits, (int32_t *)&(buffer[0]),
                                   (int32_t *)&(buffer[header_size]), ni * nj * _nk);
                    // fprintf(stderr, "Debug+ fstecr apres float_packer buffer->data=%8X\n", buffer->data[keys_len]);
                }
                break;


            case FST_TYPE_STRING:
            case FST_TYPE_STRING | FST_TYPE_TURBOPACK:
                // character string
                if (is_type_turbopack(datyp)) {
                    Lib_Log(APP_LIBFST, APP_WARNING, "%s: extra compression not available, data typereset to FST_TYPE_STRING (%d)\n",
                            __func__, FST_TYPE_STRING);
                    datyp = FST_TYPE_STRING;
//                     stdf_entry->datyp = datyp;
                }
//                 compact_p_char(field_u32, (void *) NULL, &(buffer->data[keys_len]), ni * nj * _nk, 8, 0, xdf_stride);
                compact_p_char(field_u32, (void *) NULL, &(buffer[0]), ni * nj * _nk, 8, 0, xdf_stride);
                break;

            default:
                Lib_Log(APP_LIBFST, APP_ERROR, "%s: invalid datyp=%d\n", __func__, datyp);
                goto fail ;
        } // end switch

    // write record to file and add entry to directory
//     int ier = c_xdfput(iun, handle, buffer);
//     if (Lib_LogLevel(APP_LIBFST, NULL) >= APP_INFO) {
//         char string[14];
//         snprintf(string, sizeof(string), "Write(%d)", iun);
//         print_std_parms(stdf_entry, string, prnt_options, -1);
//     }

    if (field_f != NULL) free(field_f);
    if (field_missing != NULL) free(field_missing);
//     free(buffer);

//     xdf_double = 0;
//     xdf_short = 0;
//     xdf_byte = 0;
    int ier = 0 ;
    return buffer;
fail :
   if(buffer) free(buffer) ;
   return NULL ;
}
