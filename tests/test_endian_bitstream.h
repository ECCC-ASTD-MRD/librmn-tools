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

int CONCAT(PREFIX,test)(){
  int i, nbits = 12, npts = 4095, errors ;
  ssize_t totbits = 0, copybits ;
  uint32_t w32, sbuf[4096], copy_buffer[4096] ;
  bitstream s0 ;

  fprintf(stderr, "============================== base test ==============================\n\n") ;
  s0 = NULL_BITSTREAM ;
//   InitStream(&s0, sbuf, sizeof(sbuf), 0); SET_STREAM_ENDIANNESS(s0) ;
  STREAM_INIT(&s0, sbuf, sizeof(sbuf), 0) ;
  if(s0.endian != PACK_ENDIAN) return 1 ;

  StreamPrintParams(s0, "after InitStream", NULL) ;
  StreamPrintData(s0, "s0", 2) ;
  totbits = 0 ;
  if(StreamAvailableBits(&s0) != 0){                         // test failed
    fprintf(stderr, "StreamAvailableBits = %ld, expecting 0\n", StreamAvailableBits(&s0)) ;
    return 1 ;
  }
  if(StreamAvailableSpace(&s0) != 8*sizeof(sbuf)){           // test failed
    fprintf(stderr, "StreamAvailableSpace = %ld, expecting %ld\n", StreamAvailableSpace(&s0), 8*sizeof(sbuf)) ;
    return 1 ;
  }

  for(i=0 ; i<npts ; i++){
    STREAM_PUT_NBITS(s0, i, nbits) ;
    totbits += nbits ;
  }
  StreamPrintData(s0, "s0", 2) ;
  StreamPrintParams(s0, "before finalize", NULL) ;
  if(STREAM_BITS_STORED(s0) != totbits){
    fprintf(stderr, "STREAM_BITS_STORED= %ld, expecting %ld\n", STREAM_BITS_STORED(s0), totbits) ;
    return 1 ;
  }
  if(StreamAvailableBits(&s0) != totbits){
    fprintf(stderr, "StreamAvailableBits = %ld, expecting %ld\n", StreamAvailableBits(&s0), totbits) ;
    return 1 ;
  }

  copybits = StreamDataCopy(&s0, (void *)copy_buffer, sizeof(copy_buffer));
  fprintf(stderr, "copied %ld bits before finalize from s0 stream\n", copybits) ;

  STREAM_FLUSH(s0) ; STREAM_INSERT_FINALIZE(s0) ; StreamFlush(&s0) ;

  StreamPrintData(s0, "s0", 3) ;
  fprintf(stderr, "inserted %ld(%ld) bits\n", totbits, (totbits+31)/32*32) ;
  StreamPrintParams(s0, "after finalize", NULL) ;

  copybits = StreamDataCopy(&s0, (void *)copy_buffer, sizeof(copy_buffer));
  fprintf(stderr, "copied %ld bits after finalize from s0 stream\n", copybits) ;

  STREAM_REWIND(s0, 1) ; StreamRewind(&s0, 1) ;

  StreamPrintParams(s0, "after rewind", NULL) ;
  errors = 0 ;
  for(i=0 ; i<npts ; i++){
    STREAM_GET_NBITS(s0, w32, nbits) ;
    if(w32 != (uint32_t)i) errors++ ;
  }
  if(errors > 0) {
    fprintf(stderr, "%d errors detected when extracting from stream\n", errors) ;
    return 1 ;
  }
  InitStream(&s0, sbuf, sizeof(sbuf), BIT_FULL_INIT);
  s0.endian = PACK_ENDIAN ;
  StreamPrintParams(s0, "after reinitialization", NULL) ;

  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "============================== nbits = 1->32 test ==============================\n\n") ;
  npts = 4096 ;
  uint32_t mask ;
  InitStream(&s0, sbuf, sizeof(sbuf), BIT_FULL_INIT) ;
  s0.endian = PACK_ENDIAN ;
  for(nbits = 1 ; nbits < 33 ; nbits++){
    STREAM_REWRITE(s0, 1) ; StreamRewrite(&s0, 1) ;
    mask = 0xFFFFFFFFu ;
    mask >>= (32-nbits) ;
    for(i=0 ; i<npts ; i++){
      STREAM_PUT_NBITS(s0, (i & mask), nbits) ;
    }
    STREAM_FLUSH(s0) ; STREAM_INSERT_FINALIZE(s0) ; StreamFlush(&s0) ;
    STREAM_REWIND(s0, 1) ; StreamRewind(&s0, 1) ;
    errors = 0 ;
    for(i=0 ; i<npts ; i++){
      STREAM_GET_NBITS(s0, w32, nbits) ;
      if(w32 != (uint32_t)(i&mask)) errors++ ;
    }
    if(errors > 0) {
      fprintf(stderr, "%d errors detected when extracting from stream\n", errors) ;
      return 1 ;
    }
  }
  fprintf(stderr, "SUCCESS\n") ;

  return 0 ;
}

#undef PREFIX
