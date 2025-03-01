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
{
  int i, nbits = 12, npts = 255, errors ;
  size_t totbits = 0 ;
  uint32_t w32, sbuf[4096] ;
  bitstream s0 ;

  s0 = null_bitstream ;
  InitStream(&s0, sbuf, sizeof(sbuf), 0);
  s0.endian = PACK_ENDIAN ;

  print_stream_params(s0, "after InitStream", NULL) ;
  print_stream_data(s0, "s0", 2) ;
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
  print_stream_data(s0, "s0", 2) ;
  print_stream_params(s0, "before finalize", NULL) ;
  if(StreamAvailableBits(&s0) != totbits){
    fprintf(stderr, "StreamAvailableBits = %ld, expecting %ld\n", StreamAvailableBits(&s0), totbits) ;
    return 1 ;
  }
  STREAM_INSERT_FINALIZE(s0) ;
  print_stream_data(s0, "s0", 3) ;
  fprintf(stderr, "inserted %ld(%ld) bits\n", totbits, (totbits+31)/32*32) ;
  print_stream_params(s0, "after finalize", NULL) ;

  fprintf(stderr, "SUCCESS\n") ;

  STREAM_REWIND(s0, 1) ;
  print_stream_params(s0, "after rewind", NULL) ;
  errors = 0 ;
  for(i=0 ; i<npts ; i++){
    STREAM_GET_NBITS(s0, w32, nbits) ;
    if(w32 != i) errors++ ;
  }
  fprintf(stderr, "%d errors detected when extracting from stream\n", errors) ;
  if(errors > 0) {
    return 1 ;
  }

  InitStream(&s0, sbuf, sizeof(sbuf), BIT_FULL_INIT);
  s0.endian = PACK_ENDIAN ;
  print_stream_params(s0, "after reinitialization", NULL) ;
}
