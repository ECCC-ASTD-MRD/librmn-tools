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
// set of functions to manage a bit stream
// N.B. this bitstream is a sequence of 32 bit unsigned integers

#include <string.h>
#include <stdio.h>

#include <rmn/bitstream.h>

// =======================  stream initialization functions =======================
//
// generic bit stream (re)initializer
// s    [OUT] : pointer to an existing bitstream structure (structure will be updated)
// mem   [IN] : pointer to user supplied memory (if NULL, use malloc to allocate memory for bit stream data)
// size  [IN] : size of the memory area (user supplied or auto allocated) (BYTES)
// mode  [IN] : combination of BIT_INSERT, BIT_XTRACT, BIT_FULL_INIT
// if mode == 0, both insertion and extraction operations are allowed
// size is in bytes
void  InitStream(bitstream *s, void *mem, size_t size, int mode){
  uint32_t *buf = (uint32_t *) mem ;
  if(mode & BIT_FULL_INIT){
    *s = null_bitstream ;    // perform a full (re)initialization, nullify all fields
  }
  if((mode & (BIT_INSERT | BIT_XTRACT)) == 0) mode = mode | BIT_INSERT | BIT_XTRACT ;  // neither insert nor extract set, set both

  if( (s->first != NULL) && (s->in != NULL) && (s->out != NULL) && (s->limit != NULL) && (s->valid == VALID_STREAM) ){
    buf = s->first ;        // existing and valid stream, already has a buffer, set buf to first, ignore mem
  }else{                    // not an existing stream, perform a full initialization
    if(buf == NULL){
      s->user   = 0 ;                          // not user supplied space
      s->alloc  = 1 ;                          // auto allocated space (can be freed if resizing)
      buf    = (uint32_t *) malloc(size) ;     // allocate space to accomodate up to size bytes
    }else{
      s->user   = 1 ;                          // user supplied space
      s->alloc  = 0 ;                          // not auto allocated space
    }
    s->full   = 0 ;                            // malloc not for both struct and buffer
    s->spare  = 0 ;
    s->valid   = VALID_STREAM ;                // mark bit stream as valid
    s->first  = buf ;                          // stream storage buffer
    s->limit  = buf + size/sizeof(uint32_t) ;  // potential truncation to 32 bit alignment
  }

  s->in     = buf ;                            // stream is empty and insertion starts at beginning of buffer
  s->out    = buf ;                            // stream is filled and extraction starts at beginning of buffer
  s->acc_i  = 0 ;                              // insertion accumulator is empty
  s->acc_x  = 0 ;                              // extraction accumulator is empty
  s->insert = 0 ;                              // insertion point at first free bit
  s->xtract = 0 ;                              // extraction point at first available bit
  if((mode & BIT_XTRACT) == 0) s->xtract = -1 ;  // deactivate extract mode (insert only mode)
  if((mode & BIT_INSERT) == 0) s->insert = -1 ;  // deactivate insert mode  (extract only mode)
  if(mode & SET_BIG_ENDIAN   ) s->endian = STREAM_BE ;
  if(mode & SET_LITTLE_ENDIAN) s->endian = STREAM_LE ;
//   if(s->endian == 0) s->endian = STREAM_BE ;   //  default to BIG endian if not already defined
}

// mem   [IN] : pointer to user supplied memory (if NULL, use malloc to allocate memory for bit stream data)
// size  [IN] : size of the memory area (user supplied or auto allocated) (BYTES)
// mode  [IN] : combination of BIT_INSERT, BIT_XTRACT, BIT_FULL_INIT
// if mode == 0, both insertion and extraction operations are allowed
// size is in bytes
// return pointer to created bitstream
bitstream *CreateStream(void *mem, size_t size, int mode){
  char *data = mem ;
  size_t size_alloc = sizeof(bitstream) ;
  bitstream *s ;
  if(data == NULL) size_alloc += size ;
  s = (bitstream *) malloc(size_alloc) ;
  if(s == NULL) return NULL ;

  if(data == NULL){
    data = (char *)s ;
    data += sizeof(bitstream) ;
  }
  InitStream(s, data, size, mode) ;
  s->full = 1 ;
  return s ;
}

// generic bit stream destructor
// s    [IN] : pointer to an existing bitstream structure
// return 0 if operation successful, non zero if there was an error
bitstream *FreeStream(bitstream *s, int *error){
  if(StreamIsInvalid(s)){     // not a valid stream
    *error = 1 ;
    s = NULL ;
// fprintf(stderr, "invalid stream\n");
    goto end ;
  }

  if(s->full){                   // the whole bitstream struct was allocated with malloc()
    free(s) ;
    s = NULL ;
    *error = 0 ;
// fprintf(stderr, "fully allocated stream\n");
    goto end ;
  }
// fprintf(stderr, "stream with buffer at %p, alloc = %d\n", (void *)s->first, s->alloc);
  if(s->alloc) free(s->first) ;  // the stream buffer was not supplied by the user
  *s = null_bitstream ;          // blank stream

end:
  return s ;
}

// =======================  stream utility functions  =======================

// flush any insertion data left in accumulator into stream
// s [IN] : pointer to a bit stream struct
// return 0 if O.K., non zero in case of error
int StreamFlush(bitstream *s){

  if(! StreamIsValid(s))         return 1 ;              // invalid stream

  if(s->insert > 0) {                                    // flush contents of accumulator into buffer
    if(s->endian == STREAM_BE){                          // Big Endian flush
      *(s->in) = (s->acc_i >> 32) ; (s->in)++ ;
      if(s->insert > 32){ *(s->in) = s->acc_i ; (s->in)++ ; }
    }else{                                               // Little Endian flush
      s->acc_i >>= (64 - s->insert) ;                    // right align accumulator
      *(s->in) = s->acc_i ; (s->in)++ ;                  // lower 32 bits
      if(s->insert > 32){ *(s->in) = (s->acc_i >> 32) ; (s->in)++ ; }
    }
    s->insert = 0 ; s->acc_i = 0 ;
  }
  return 0 ;
}

// rewind a bit stream to read it from the beginning (potentially force valid read mode)
// s    [IN] : pointer to an existing bitstream structure
// return 0 if O.K., non zero if error
int StreamRewind(bitstream *s, int force_read){
  if(! StreamIsValid(s))         return 1 ;              // invalid stream
  if(s->insert > 0) StreamFlush(s) ;                     // data left in insert accumulator
  if(force_read) s->xtract = 0 ;
  if(s->xtract >= 0){
    s->acc_x  = 0 ;
    s->out = s->first ;
  }
  return 0 ;
}

// rewind a bit stream to rewrite it from the beginning (potentially force valid write mode)
// s    [IN] : pointer to an existing bitstream structure
// return 0 if O.K., non zero if error
int StreamRewrite(bitstream *s, int force_write){
  if(! StreamIsValid(s))         return 1 ;              // invalid stream
  if(force_write) s->insert = 0 ;
  if(s->insert > 0) StreamFlush(s) ;                     // data left in insert accumulator
  s->acc_i  = 0 ;
  s->in = s->first ;
  return 0 ;
}

// reset both read and write pointers to beginning of stream (according to insert/xtract only flags)
// s    [IN] : pointer to an existing bitstream structure
// return 0 if O.K., non zero if error
int StreamReset(bitstream *s){
  if(! StreamIsValid(s))         return 1 ;              // invalid stream
  if(s->insert >= 0){      // insertion allowed
    s->in     = s->first ;
    s->acc_i  = 0 ;
    s->insert = 0 ;
  }
  if(s->xtract >= 0){      // extraction allowed
    s->out    = s->first ;
    s->acc_x  = 0 ;
    s->xtract = 0 ;
  }
  return 0 ;
}
// =======================  stream information  =======================

// is stream valid ?
// s [IN] : pointer to a bit stream struct
int StreamIsValid(bitstream *s){
  if(s->valid != VALID_STREAM)                 return 0 ;    // incorrect marker
  if(s->first == NULL)                         return 0 ;    // no buffer
  if(s->limit == NULL)                         return 0 ;    // invalid limit
  if(s->limit <= s->first)                     return 0 ;    // invalid first/limit combination
  if(s->in < s->first  || s->in > s->limit)    return 0 ;    // in is out of bounds
  if(s->out < s->first || s->out > s->limit)   return 0 ;    // out is out of bounds
  if(s->endian == 0 || s->endian == 3 )        return 0 ;    // invalid endianness
  return 1 ;                                                 // probably valid stream
}

int StreamIsInvalid(bitstream *s){
  return (1 - StreamIsValid(s)) ;
}

// s    [IN] : pointer to an existing bitstream structure
// return number of bits available for extraction
size_t StreamAvailableBits(bitstream *s){
  if(s->xtract < 0) return -1 ;                               // extraction is not allowed
  int32_t in_xtract = (s->xtract < 0) ? 0 : s->xtract ;       // bits in extract accumulator
  int32_t in_insert = (s->insert < 0) ? 0 : s->insert ;       // bits in insert accumulator
  return (s->in - s->out)*32 + in_insert + in_xtract ;        // stream + bits in accumulators
}

// s    [IN] : pointer to an existing bitstream structure
// return number of bits available for extraction
// in strict mode, bits in insert accumulator are ignored
size_t StreamStrictAvailableBits(bitstream *s){
  if(s->xtract < 0) return -1 ;                               // extraction is not allowed
  int32_t in_xtract = (s->xtract < 0) ? 0 : s->xtract ;       // bits in accumulator
  return (s->in - s->out)*32 + in_xtract ;                    // stream + extract accumulator contents
}

// s    [IN] : pointer to an existing bitstream structure
// return number of bits available for insertion
ssize_t StreamAvailableSpace(bitstream *s){
  if(s->insert < 0) return -1 ;   // insertion is not allowd
  return (s->limit - s->in)*32 - s->insert ;   // available space in stream buffer minus accumulator contents
}

// s    [IN] : pointer to an existing bitstream structure
// get stream mode as a string
char *StreamMode(bitstream s){
  if( STREAM_INSERT_MODE(s) && STREAM_XTRACT_MODE(s)) return("RW") ;
  if( STREAM_XTRACT_MODE(s) ) return "R" ;
  if( STREAM_INSERT_MODE(s) ) return "W" ;
  return("Unknown") ;
}

// s    [IN] : pointer to an existing bitstream structure
// get stream mode as a code
int StreamModeCode(bitstream s){
  int32_t mode = 0 ;
  if( STREAM_XTRACT_MODE(s) ) mode |= BIT_XTRACT ;
  if( STREAM_INSERT_MODE(s) ) mode |= BIT_INSERT ;
  return mode ? mode : -1 ;                               // return -1 if neither extract nor insert is set
}

// =======================  stream data copy  =======================
//
// copy stream data into array mem (from beginning up to in pointer and data in accumulator if any)
// the original stream control info remains untouched (up to 2 32 bit items may get added to its buffer)
// stream [IN] : pointer to bit stream struct
// mem   [OUT] : where to copy
// size   [IN] : size of mem array in bytes
// return original size of valid info from stream in bits (-1 in case of error)
size_t StreamDataCopy(bitstream *s, void *mem, size_t size){
  size_t nbtot, nborig ;
  bitstream temp ;    // temporary struct used during the copy process

  if(! StreamIsValid(s))         return -1 ;               // invalid stream
  temp = *s ;                                              // copy stream struct to avoid altering original

  // precise number of used bits in stream buffer
  nborig = (temp.in - temp.first) * 8 * sizeof(uint32_t) + ((temp.insert > 0) ? temp.insert : 0) ;
  StreamFlush(&temp);                                          // flush contents of accumulator into buffer
//   if(temp.insert > 0) {                                    // flush contents of accumulator into buffer
//     if(temp.endian == STREAM_BE){                          // Big Endian flush
//       *(temp.in) = (temp.acc_i >> 32) ; (temp.in)++ ;
//       if(temp.insert > 32){ *(temp.in) = temp.acc_i ; (temp.in)++ ; }
//     }else{                                                 // Little Endian flush
//       temp.acc_i >>= (64 - temp.insert) ;                  // right align accumulator
//       *(temp.in) = temp.acc_i ; (temp.in)++ ;              // lower 32 bits
//       if(temp.insert > 32){ *(temp.in) = (temp.acc_i >> 32) ; (temp.in)++ ; }
//     }
//     temp.insert = 0 ; temp.acc_i = 0 ;
//   }
  nbtot = (temp.in - temp.first) * sizeof(uint32_t) ;           // size in bytes when nothing is left in acumulator
  if(nbtot == 0) return 0 ;                                     // there was no data in stream
  if(nbtot > size) return -1 ;                                  // insufficient space
  if(mem != memmove(mem, temp.first, nbtot)) return -1 ;        // error copying
// fprintf(stderr, "StreamDataCopy : nborig = %ld bits, nbtot = %ld bits\n", nborig, nbtot*8) ;
  return nborig ;                                               // return unrounded result
}

// =======================  print stream data and metadata  =======================
// print some elements at the beginning and at the end of the bit stream data buffer
// (see bi_endian_pack.h)
// s    [IN] : bitstream structure
// msg  [IN] : user message
// edge [IN] : do not print data elements more that edge positions from first or in
void StreamPrintData(bitstream s, char *msg, int edge){
  uint32_t *in = s.in ;
  uint32_t *first = s.first ;
  uint32_t *cur, *start ;
  int inc, count = 0 ;

  fprintf(stderr, "[%2s] %s : ", STREAM_IS_LITTLE_ENDIAN(s) ? "LE" : "BE", msg) ;
  fprintf(stderr, "accum = %16.16lx", s.acc_i << (64 - s.insert)) ;
  fprintf(stderr, ", guard = %8.8x, data =", *in) ;

  if(STREAM_IS_LITTLE_ENDIAN(s)){
    cur = in ; inc = -1 ;
  }else{                            // big endian or not specified
    cur = first ; inc = +1 ;
  }

  for(start=first ; start <= in ; start++, cur = cur + inc){
    if(in - cur == 0 && s.insert == 0) continue ;          // last element not used
    if(cur-first < edge || in-cur <= edge) {
      fprintf(stderr, " %8.8x ", *cur) ;
    }else{
      count++ ;
      if((count & 0xFF) == 1) fprintf(stderr, ".") ;
    }
  }
  fprintf(stderr, "\n") ;
}

// print bit stream control information
// s             [IN] : bitstream structure
// msg           [IN] : user message
// expected_mode [IN] : "R", "W", or "RW"  read/write/read-write, expected mode for bit stream
void StreamPrintParams(bitstream s, char *msg, char *expected_mode){
  int32_t available        = StreamAvailableBits(&s) ;
  int32_t strict_available = StreamStrictAvailableBits(&s) ;
  int32_t space_available   = StreamAvailableSpace(&s) ;

  available = (available < 0) ? 0 : available ;
  strict_available = (strict_available < 0) ? 0 : strict_available ;
  fprintf(stderr, "%s: filled = %d(%d), free= %d, first/in/out/limit [in - out] = %p/%ld/%ld/%ld [%ld], insert/xtract = %d/%d",
    msg, available, strict_available, space_available, 
    (void *)s.first, s.in-s.first, s.out-s.first, s.limit-s.first, s.in-s.out, s.insert, s.xtract ) ;
  fprintf(stderr, ", full/alloc/user = %d/%d/%d", s.full, s.alloc, s.user) ;
  fprintf(stderr, ", |%8.8x|%2.2x|", s.valid, s.endian) ;
  fprintf(stderr, ", Mode = %s(%d)", StreamMode(s), StreamModeCode(s)) ;
  if(expected_mode){
    fprintf(stderr, " (%s expected)", expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      fprintf(stderr, "\nBad mode, exiting\n") ;
      exit(1) ;
    }
  }
  fprintf(stderr, "\n") ;
}
