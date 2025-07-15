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
#undef NPTS
#define NPTS 4096

int CONCAT(PREFIX,test)(){
  int i, nbits, npts, errors, status ;
  ssize_t totbits = 0, copybits ;
  uint32_t token, w32, sbuf[NPTS], copy_buffer[NPTS] ;
  bitstream s0 ;

  fprintf(stderr, "============================== base test ==============================\n\n") ;

  s0 = NULL_BITSTREAM ;
  //  initialize bit stream, set endianness
  STREAM_INIT(&s0, sbuf, sizeof(sbuf), 0) ;
  if(s0.endian != PACK_ENDIAN) return 1 ;                    // test failed

  StreamPrintParams(s0, "after InitStream", NULL) ;
  StreamPrintData(s0, "s0", 2) ;
  totbits = 0 ;
  if(StreamAvailableBits(&s0) != 0){                         // test failed
    fprintf(stderr, "StreamAvailableBits = %ld, expecting 0\n", StreamAvailableBits(&s0)) ;
    return 2 ;
  }
  if(StreamAvailableSpace(&s0) != 8*sizeof(sbuf)){           // test failed
    fprintf(stderr, "StreamAvailableSpace = %ld, expecting %ld\n", StreamAvailableSpace(&s0), 8*sizeof(sbuf)) ;
    return 3 ;
  }

  nbits = 12 ;
  npts  = NPTS -1 ;
  for(i=0 ; i<npts ; i++){
    STREAM_PUT_NBITS(s0, i, nbits) ;
    totbits += nbits ;
  }
  StreamPrintData(s0, "s0", 2) ;
  StreamPrintParams(s0, "before finalize", NULL) ;
  if(STREAM_BITS_STORED(s0) != totbits){
    fprintf(stderr, "STREAM_BITS_STORED= %ld, expecting %ld\n", STREAM_BITS_STORED(s0), totbits) ;
    return 4 ;
  }
  if(StreamAvailableBits(&s0) != totbits){
    fprintf(stderr, "StreamAvailableBits = %ld, expecting %ld\n", StreamAvailableBits(&s0), totbits) ;
    return 5 ;
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
    return 6 ;
  }
  InitStream(&s0, sbuf, sizeof(sbuf), BIT_FULL_INIT);
  s0.endian = PACK_ENDIAN ;
  StreamPrintParams(s0, "after reinitialization", NULL) ;

  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "================= nbits = 1->32 insert/extract/verify test =================\n\n") ;
  npts = NPTS ;
  uint32_t mask, token_or, token_and ;
  InitStream(&s0, sbuf, sizeof(sbuf), BIT_FULL_INIT) ;
  s0.endian = PACK_ENDIAN ;
  for(nbits = 1 ; nbits < 33 ; nbits++){
    // write stream
    STREAM_REWRITE(s0, 1) ; StreamRewrite(&s0, 1) ;    // redundant, testing macro and function syntax
    mask = 0xFFFFFFFFu ;
    mask >>= (32-nbits) ;
    if(nbits == 1){
      for(i=0 ; i<npts ; i++){
        token = (i & mask) ;
        STREAM_INSERT_CHECK(s0) ;
        if(token){
          STREAM_FAST_PUT_1(s0) ;
        }else{
          STREAM_FAST_PUT_0(s0) ;
        }
      }
    }else{
      token_or = 0 ; token_and = 0xFFFFFFFFu ;
      for(i=0 ; i<npts ; i++){
        token = (i-npts/2) ;    // token = ((i-npts/2) & mask) ; // masking not needed
        STREAM_PUT_NBITS(s0, token, nbits) ;
        token &= mask ;
        token_or |= token ; token_and &= token ;
      }
      if(token_or  != mask) return 100 ;  // check that there is a 1 in all useful bit positions in one of the tokens
      if(token_and !=    0) return 101 ;  // check that there is a 0 in all bit positions in one of the tokens
    }
    STREAM_FLUSH(s0) ; STREAM_INSERT_FINALIZE(s0) ; StreamFlush(&s0) ;    // redundant, testing macro and function syntax
    // read back what was written and perform error check
    STREAM_REWIND(s0, 1) ; StreamRewind(&s0, 1) ;    // redundant, testing macro and function syntax
    errors = 0 ;
    if(nbits == 1){
      for(i=0 ; i<npts ; i++){
        STREAM_XTRACT_CHECK(s0) ;
        STREAM_PEEK_1(s0, w32) ;
        STREAM_SKIP_1(s0) ;
        if(w32 != (uint32_t)(i&mask)){
          errors++ ;
        }
      }
    }else{
      for(i=0 ; i<npts ; i++){
        STREAM_GET_NBITS(s0, w32, nbits) ;
        token = ((i-npts/2) & mask) ;
        if(w32 != token){
          errors++ ;
        }
      }
    }
    if(errors > 0) {
      fprintf(stderr, "%d errors detected when extracting from stream\n", errors) ;
      return 7 ;
    }
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "============================== destruct test ===============================\n\n") ;

  bitstream s1, s2, *ps0, *ps1, *ps2 ;
  npts = NPTS + 1 ;

  int debug_mode = StreamDebugSet(1) ;
  fprintf(stderr, "debug mode was %d, is now 1\n", debug_mode) ;
  if(debug_mode != 0) return 30 ;

  SET_NULL_BITSTREAM(s1) ;
  fprintf(stderr, "attempt to free an invalid stream\n") ;
  ps1 = FreeStream(&s1, &status) ;
  if(ps1 != NULL || status == 0) return 31 ;

  SET_NULL_BITSTREAM(s1) ;
  STREAM_INIT(&s1, NULL, sizeof(sbuf), BIT_FULL_INIT) ;  // mode = BIT_FULL_INIT redundant, s1 is already set to NULL_BITSTREAM
  STREAM_INIT(&s1, NULL, sizeof(sbuf), 0) ;
  fprintf(stderr, "attempt to free a stream with caller supplied buffer\n") ;
  ps1 = FreeStream(&s1, &status) ;
  if(ps1 != &s1 || status != 0) return 8 ;

  ps0 = CreateStream(NULL, sizeof(sbuf), BIT_FULL_INIT) ; ps0->endian = PACK_ENDIAN ;
  if(ps0->endian != PACK_ENDIAN){
    fprintf(stderr, "ps1->endian = %cE, expected %cE\n", ps0->endian, PACK_ENDIAN) ;
    return 9 ;
  }
  fprintf(stderr, "attempt to free a single malloc stream\n") ;
  ps0 = FreeStream(ps0, &status) ;
  if(ps0 != NULL || status != 0) return 10 ;

  STREAM_CREATE(ps1, NULL, sizeof(sbuf), BIT_FULL_INIT) ;
  if(ps1->endian != PACK_ENDIAN){
    fprintf(stderr, "ps1->endian = %cE, expected %cE\n", ps1->endian, PACK_ENDIAN) ;
    return 11 ;
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "============================== resize test ================================\n\n") ;

  ps2 = StreamResize(ps1, NULL, sizeof(sbuf));
  if(ps2 != ps1) return 12 ;

  ps2 = StreamResize(ps1, NULL, 2*sizeof(sbuf));
  if(ps2 == ps1) return 13 ;
  if(STREAM_BUFFER_BYTES(*ps2) != 2*sizeof(sbuf)) return 14 ;

  nbits = 11 ;

  s1 = NULL_BITSTREAM ;
  STREAM_INIT(&s1, NULL, sizeof(sbuf), 0) ; // mode = BIT_FULL_INIT not needed, s1 is already set to NULL_BITSTREAM
  if(s1.endian != PACK_ENDIAN) return 15 ;
  // fill stream with data
  for(i=0 ; i<npts ; i++){ STREAM_PUT_NBITS(s1, (i & 0xFF), nbits) ; } ;
  copybits = StreamDataCopy(&s1, sbuf, sizeof(sbuf)) ;
  fprintf(stderr, "copybits = %ld, inserted bits = %d, bits in source stream = %ld\n", copybits, nbits*npts, StreamAvailableBits(&s1)) ;
  STREAM_FLUSH(s1) ;

  ps1 = StreamResize(&s1, NULL, 2*sizeof(sbuf)) ;
  if(ps1 != &s1) return 16 ;
  if(STREAM_BUFFER_BYTES(s1) != 2*sizeof(sbuf)) return 17 ;
//   copybits = StreamDataCopy(ps1, sbuf, sizeof(sbuf)) ;
//   fprintf(stderr, "copybits = %ld, inserted bits = %d, bits in source stream = %ld\n", copybits, nbits*npts, StreamAvailableBits(ps1)) ;
  errors = 0 ;
  // read data from resized stream
  for(i=0 ; i<npts ; i++){ STREAM_GET_NBITS(s1, w32, nbits) ; if(w32 != (i & 0xFF)) errors++ ; }
  if(errors > 0) return 18 ;

  STREAM_INIT(&s2, sbuf, sizeof(sbuf), 0) ;  // sbuf contains stream data saved from ps1
  if(s2.endian != PACK_ENDIAN) return 19 ;
  status = StreamSetFilledBits(&s2, copybits) ;
  if(status != 0) return 20 ;
  errors = 0 ;
  for(i=0 ; i<npts ; i++){ STREAM_GET_NBITS(s2, w32, nbits) ; if(w32 != (i & 0xFF)) errors++ ; }
  if(errors > 0) return 21 ;


  int debug_mode2 = StreamDebugSet(debug_mode) ;
  fprintf(stderr, "debug mode was %d, is now %d\n", debug_mode2, debug_mode) ;
  if(debug_mode2 != 1) return 22 ;

  fprintf(stderr, "END OF TEST\n") ;

  return 0 ;
}

#undef PREFIX
