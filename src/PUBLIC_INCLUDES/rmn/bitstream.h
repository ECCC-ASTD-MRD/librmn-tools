//
// Copyright (C) 2025  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
// set of macros and functions to manage a bit stream
// N.B. this bitstream is a sequence of 32 bit unsigned integers

#if !defined(VALID_STREAM)

// validity marker
#define VALID_STREAM 0xCAFEFADEu
// stream insert/extract mode (0 or 3 would mean both insert and extract)
// extract mode
#define BIT_XTRACT        1
// insert mode
#define BIT_INSERT        2
// full initialization mode
#define BIT_FULL_INIT     8
// set endianness
#define SET_BIG_ENDIAN      16
#define SET_LITTLE_ENDIAN   32

// endianness
#define STREAM_BE 0xBE
#define STREAM_LE 0xEB
#define STREAM_ENDIANNESS(s) (s).endian
#define STREAM_IS_BIG_ENDIAN(s) ( (s).endian == STREAM_BE )
#define STREAM_IS_LITTLE_ENDIAN(s) ( (s).endian == STREAM_LE )

// true if stream is in read (extract) mode
// possibly false for a NEWLY INITIALIZED stream
#define STREAM_XTRACT_MODE(s) ((s).xtract >= 0)
// true if stream is in write (insert) mode
// possibly false for a NEWLY INITIALIZED (empty) stream
#define STREAM_INSERT_MODE(s) ((s).insert >= 0)

#include <stdint.h>
#include <stdlib.h>

// compile time assert macros
#include <rmn/ct_assert.h>

// bit stream descriptor. both insert / extract may be positive
// in insertion only mode, xtract MUST be -1
// in extraction only mode, insert MUST be -1
// for now, a bit stream is unidirectional (either insert or extract mode)
typedef struct{
  uint32_t valid:32 ; // signature marker
  uint32_t full:  1 , // the whole struct was allocated with malloc
           alloc: 1 , // buffer was allocated with malloc
           user:  1 , // buffer was user supplied
           spare:21 , // spare bits
           endian:8 ; // 0xBE : Big Endian stream, 0xEB : Little Endian stream, other value : invalid
  uint32_t *first ;   // pointer to start of stream data storage
  uint32_t *limit ;   // pointer to end of stream data storage (1 byte beyond stream buffer end)
  uint64_t  acc_i ;   // 64 bit unsigned bit accumulator for insertion
  uint32_t *in ;      // pointer into packed stream (insert mode)
  int32_t   insert ;  // number of bits used in accumulator (insert <= 64)
  int32_t   xtract ;  // number of bits extractable from accumulator (xtract <= 64)
  uint32_t *out ;     // pointer into packed stream (extract mode)
  uint64_t  acc_x ;   // 64 bit unsigned bit accumulator for extraction
} bitstream ;
CT_ASSERT_(sizeof(bitstream) == 64)    // 8 64 bit elements

// all fields set to 0, makes for a fast initialization xxx = null_bitstream
static const bitstream null_bitstream = { .acc_i = 0, .acc_x = 0 , .insert = 0 , .xtract = 0, 
                                    .first = NULL, .in = NULL, .out = NULL, .limit = NULL,
                                    .full = 0, .alloc = 0, .user = 0, .endian = 0, .spare = 0, .valid = 0 } ;

int StreamIsValid(bitstream *s);
int StreamIsInvalid(bitstream *s);

bitstream *CreateStream(void *mem, size_t size, int mode);
void  InitStream(bitstream *s, void *mem, size_t size, int mode);
bitstream *FreeStream(bitstream *s, int *error);

size_t StreamAvailableBits(bitstream *s);
size_t StreamStrictAvailableBits(bitstream *s);
ssize_t StreamAvailableSpace(bitstream *s);
char *StreamMode(bitstream s);
int StreamModeCode(bitstream s);

void StreamReset(bitstream *s);
size_t StreamDataCopy(bitstream *s, void *mem, size_t size);

#endif
