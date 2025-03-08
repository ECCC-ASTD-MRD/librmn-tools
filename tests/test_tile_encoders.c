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
// Lesser General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/test_helpers.h>
#include <rmn/move_blocks.h>

// deliberate double inclusion
#include <rmn/timers.h>
#include <rmn/timers.h>
#include <rmn/tile_encoders.h>
#include <rmn/tile_encoders.h>
#include <rmn/cpp_extras.h>
#include <rmn/cpp_extras.h>

#include <rmn/be_stream.h>

#define NPTI 8
#define NPTJ 8
#define NPT 64

int main(int argc, char **argv){
  uint64_t freq ;
  double nano ;
  TIME_LOOP_DATA ;
  int i, status = 1, tilebits, totalbits, errors ;
  int32_t tile00[NPT], tile01[NPT], tile10[NPT], tile11[NPT] ;
  int32_t rest00[NPT], rest01[NPT], rest10[NPT], rest11[NPT] ;
  block_properties bp00, bp01, bp10, bp11 ;
  bitstream *ps ;

  // dummy code to avoid warnings
  if(argc > 1 && argv[1] == NULL) goto fail ;
  if(argc > 100) goto end ;

  // LOCAL scope compile time assertions
  CT_ASSERT_(8 == sizeof(nano))
  CT_ASSERT_(8 == sizeof(freq))
  CT_ASSERT_(2 == sizeof(uint16_t))

  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;

  start_of_test("C tile encoder test");

  fprintf(stderr, "\n============================== base test ==============================\n\n") ;
  for(i=0 ; i<NPT ; i++){
    tile00[i] = -12345 ;          // constant value
    tile01[i] = i * 64 ;          // all values >= 0, 0 -> 64*NPT
    tile10[i] = -tile01[i] ;      // all values <= 0
    tile11[i] = (i - 3) * 64  ;   // mixed signs
  }
  status = analyze_data32_block(tile00, NPTI, NPTI, NPTJ, &bp00);
  if(status != NPT){ status = 1 ; goto fail ; }
  adjust_block_properties(&bp00, int_data) ; print_int_props(bp00);

  status = analyze_data32_block(tile01, NPTI, NPTI, NPTJ, &bp01);
  if(status != NPT){ status = 2 ; goto fail ; }
  adjust_block_properties(&bp01, int_data) ; print_int_props(bp01);

  status = analyze_data32_block(tile10, NPTI, NPTI, NPTJ, &bp10);
  if(status != NPT){ status = 3 ; goto fail ; }
  adjust_block_properties(&bp10, int_data) ; print_int_props(bp10);

  status = analyze_data32_block(tile11, NPTI, NPTI, NPTJ, &bp11);
  if(status != NPT){ status = 4 ; goto fail ; }
  adjust_block_properties(&bp11, int_data) ; print_int_props(bp11);

  STREAM_CREATE(ps, NULL, sizeof(uint32_t)*NPT*8, BIT_FULL_INIT) ;
  if(ps->endian != PACK_ENDIAN){ status = 5 ; goto fail ; }
  if(StreamAvailableBits(ps) != 0){ status = 6 ; goto fail ; }
  if(StreamAvailableSpace(ps) != 8*sizeof(uint32_t)*NPT*8){ status = 7 ; goto fail ; }
  STREAM_INSERT_BEGIN(*ps) ;

  totalbits = 0 ;
  tilebits = encode_tile(ps, tile00, NPT, &bp00) ; totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile00 = %d\n", tilebits) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  tilebits = encode_tile(ps, tile01, NPT, &bp01) ; totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile01 = %d\n", tilebits) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  tilebits = encode_tile(ps, tile10, NPT, &bp10) ; totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile10 = %d\n", tilebits) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");
//   for(i=0 ; i<NPT ; i++) tile10[i] = -tile01[i] ;  // restore tile10

  tilebits = encode_tile(ps, tile11, NPT, &bp11) ; totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11 = %d\n", tilebits) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;
  status = analyze_data32_block(tile11, NPTI, NPTI, NPTJ, &bp11);

  tilebits = encode_tile(ps, tile11, NPT, &bp11) ; totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11 = %d\n", tilebits) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  if(totalbits != StreamAvailableBits(ps)){
    fprintf(stderr, "expecting %d bits in stream, found %ld\n", totalbits, StreamAvailableBits(ps)) ;
    status = 8 ;
    goto fail ;
  }

  STREAM_XTRACT_BEGIN(*ps) ;
  tilebits = decode_tile(ps, rest00, NPT) ;
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest00[i] != tile00[i]) errors++ ;
  fprintf(stderr, "tilebits for tile00 = %d, errors = %d\n\n", tilebits, errors) ;

  tilebits = decode_tile(ps, rest01, NPT) ;
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest01[i] != tile01[i]) errors++ ;
  fprintf(stderr, "tilebits for tile01 = %d, errors = %d\n\n", tilebits, errors) ;

  tilebits = decode_tile(ps, rest10, NPT) ;
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest10[i] != tile10[i]) errors++ ;
  fprintf(stderr, "tilebits for tile10 = %d, errors = %d\n\n", tilebits, errors) ;

end:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  fprintf(stderr, "FAILED(%d)\n", status) ;
  return 1 ;
}
