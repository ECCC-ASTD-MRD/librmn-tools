#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(WITH_TIMING)
#include <rmn/timers.h>
#define NITER 100
#endif

int w32_compare(void *wold, void *wnew, int n){
  uint32_t *old = (uint32_t *) wold ;
  uint32_t *new = (uint32_t *) wnew ;
  int errors = 0 ;
  while(n--){
    errors += (*old != *new) ? 1 : 0 ;
    if(errors == 1) fprintf(stderr, "expected %8.8x, got %8.8x\n", *old, *new) ;
  }
  return errors ;
}

#define FILL_FROM_TOP                                     // big and little endian packing macros
#include <rmn/bit_pack_macros.h>
int test_pack_top(void *src_u, void *src_s, int32_t nbits, int32_t n, int vfy){   // same as test_pack_bottom, but fill accumulator from MSB
  uint64_t accum_u ;
  int64_t accum_s ;
  uint32_t insert_u, insert_s, xtract_u, xtract_s,  *stream_u, *stream_s, *unpacked_u = (uint32_t *) src_u ;
  uint32_t buf_u[n], buf_s[n], restored_u[n] ;
  int32_t  restored_s[n], *unpacked_s = (int32_t *) src_s ;
  int i, errors_u = 0, errors_s = 0 ;

  // big endian style packing/unpacking
  BE64_INSERT_BEGIN(accum_u, insert_u) ; stream_u = buf_u ;                // unsigned stream
  BE64_INSERT_BEGIN(accum_s, insert_s) ; stream_s = buf_s ;                // signed stream
  for(i=0 ; i<n ; i++){
    BE64_PUT_NBITS(accum_u, insert_u, (unpacked_u[i]), nbits, stream_u) ;  // insert unsigned values
    BE64_PUT_NBITS(accum_s, insert_s, (unpacked_s[i]), nbits, stream_s) ;  // insert signed values
  }
  BE64_INSERT_FINAL(accum_u, insert_u, stream_u) ;                         // terminate insertion cleanly
  BE64_INSERT_FINAL(accum_s, insert_s, stream_s) ;

  stream_u = buf_u ; BE64_XTRACT_BEGIN(accum_u, xtract_u, stream_u) ;      // unsigned stream
  stream_s = buf_s ; BE64_XTRACT_BEGIN(accum_s, xtract_s, stream_s) ;      // signed stream
  for(i=0 ; i<n ; i++){
    BE64_GET_NBITS(accum_u, xtract_u, restored_u[i], nbits, stream_u) ;    // extract unsigned values
    BE64_GET_NBITS(accum_s, xtract_s, restored_s[i], nbits, stream_s) ;    // extract signed values
  }
  if(vfy){
    errors_u += w32_compare(unpacked_u, restored_u, n) ;                   // verify unsigned values
    errors_s += w32_compare(unpacked_s, restored_s, n) ;                   // verify signed values
  }

  // little endian style packing/unpacking
  LE64_INSERT_BEGIN(accum_u, insert_u) ; stream_u = buf_u ;                // unsigned stream
  LE64_INSERT_BEGIN(accum_s, insert_s) ; stream_s = buf_s ;                // signed stream
  for(i=0 ; i<n ; i++){
    LE64_PUT_NBITS(accum_u, insert_u, (unpacked_u[i]), nbits, stream_u) ;  // insert unsigned values
    LE64_PUT_NBITS(accum_s, insert_s, (unpacked_s[i]), nbits, stream_s) ;  // insert signed values
  }
  LE64_INSERT_FINAL(accum_u, insert_u, stream_u) ;                         // terminate insertion cleanly
  LE64_INSERT_FINAL(accum_s, insert_s, stream_s) ;

  stream_u = buf_u ; LE64_XTRACT_BEGIN(accum_u, xtract_u, stream_u) ;      // unsigned stream
  stream_s = buf_s ; LE64_XTRACT_BEGIN(accum_s, xtract_s, stream_s) ;      // signed stream
  for(i=0 ; i<n ; i++){
    LE64_GET_NBITS(accum_u, xtract_u, restored_u[i], nbits, stream_u) ;    // extract unsigned values
    LE64_GET_NBITS(accum_s, xtract_s, restored_s[i], nbits, stream_s) ;    // extract signed values
  }
  if(vfy){
    errors_u += w32_compare(unpacked_u, restored_u, n) ;                   // verify unsigned values
    errors_s += w32_compare(unpacked_s, restored_s, n) ;                   // verify signed values
  }else{
    errors_u = -1 ;
    errors_s = -1 ;
  }

  return errors_u + errors_s ;
}


#define FILL_FROM_BOTTOM                                     // big and little endian packing macros
#include <rmn/bit_pack_macros.h>
int test_pack_bottom(void *src_u, void *src_s, int32_t nbits, int32_t n, int vfy){   // same as test_pack_top, but fill accumulator from LSB
  uint64_t accum_u ;
  int64_t accum_s ;
  uint32_t insert_u, insert_s, xtract_u, xtract_s,  *stream_u, *stream_s, *unpacked_u = (uint32_t *) src_u ;
  uint32_t buf_u[n], buf_s[n], restored_u[n] ;
  int32_t  restored_s[n], *unpacked_s = (int32_t *) src_s ;
  int i, errors_u = 0, errors_s = 0 ;

  // big endian style packing/unpacking
  BE64_INSERT_BEGIN(accum_u, insert_u) ; stream_u = buf_u ;                // unsigned stream
  BE64_INSERT_BEGIN(accum_s, insert_s) ; stream_s = buf_s ;                // signed stream
  for(i=0 ; i<n ; i++){
    BE64_PUT_NBITS(accum_u, insert_u, (unpacked_u[i]), nbits, stream_u) ;  // insert unsigned values
    BE64_PUT_NBITS(accum_s, insert_s, (unpacked_s[i]), nbits, stream_s) ;  // insert signed values
  }
  BE64_INSERT_FINAL(accum_u, insert_u, stream_u) ;                         // terminate insertion cleanly
  BE64_INSERT_FINAL(accum_s, insert_s, stream_s) ;

  stream_u = buf_u ; BE64_XTRACT_BEGIN(accum_u, xtract_u, stream_u) ;      // unsigned stream
  stream_s = buf_s ; BE64_XTRACT_BEGIN(accum_s, xtract_s, stream_s) ;      // signed stream
  for(i=0 ; i<n ; i++){
    BE64_GET_NBITS(accum_u, xtract_u, restored_u[i], nbits, stream_u) ;    // extract unsigned values
    BE64_GET_NBITS(accum_s, xtract_s, restored_s[i], nbits, stream_s) ;    // extract signed values
  }
  if(vfy){
    errors_u += w32_compare(unpacked_u, restored_u, n) ;                   // verify unsigned values
    errors_s += w32_compare(unpacked_s, restored_s, n) ;                   // verify signed values
  }

  // little endian style packing/unpacking
  LE64_INSERT_BEGIN(accum_u, insert_u) ; stream_u = buf_u ;                // unsigned stream
  LE64_INSERT_BEGIN(accum_s, insert_s) ; stream_s = buf_s ;                // signed stream
  for(i=0 ; i<n ; i++){
    LE64_PUT_NBITS(accum_u, insert_u, (unpacked_u[i]), nbits, stream_u) ;  // insert unsigned values
    LE64_PUT_NBITS(accum_s, insert_s, (unpacked_s[i]), nbits, stream_s) ;  // insert signed values
  }
  LE64_INSERT_FINAL(accum_u, insert_u, stream_u) ;                         // terminate insertion cleanly
  LE64_INSERT_FINAL(accum_s, insert_s, stream_s) ;

  stream_u = buf_u ; LE64_XTRACT_BEGIN(accum_u, xtract_u, stream_u) ;      // unsigned stream
  stream_s = buf_s ; LE64_XTRACT_BEGIN(accum_s, xtract_s, stream_s) ;      // signed stream
  for(i=0 ; i<n ; i++){
    LE64_GET_NBITS(accum_u, xtract_u, restored_u[i], nbits, stream_u) ;    // extract unsigned values
    LE64_GET_NBITS(accum_s, xtract_s, restored_s[i], nbits, stream_s) ;    // extract signed values
  }
  if(vfy){
    errors_u += w32_compare(unpacked_u, restored_u, n) ;                   // verify unsigned values
    errors_s += w32_compare(unpacked_s, restored_s, n) ;                   // verify signed values
  }else{
    errors_u = -1 ;
    errors_s = -1 ;
  }

  return errors_u + errors_s ;
}

int main(int argc, char **argv){
  int npts = 4*1024 ;
  uint32_t *unpacked_u ;
  int32_t  *unpacked_s ;
  int i, nbits, errors ;
  int32_t maskn ;
#if defined(WITH_TIMING)
  TIME_LOOP_DATA ;
#endif

  unpacked_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  unpacked_s = (int32_t *)  malloc(npts * sizeof(uint32_t)) ;

  for(nbits=1 ; nbits <33 ; nbits+=1) {

    maskn = ~((0xFFFFFFFFu) << nbits) ;
    maskn >>= 1 ;
    for(i=0 ; i<npts ; i+=1) { unpacked_u[i] = maskn & i ; } ;
    for(i=0 ; i<npts ; i+=2) { unpacked_u[i] |= (1 << (nbits-1)) ; } ;  // set high bit of every other value
    for(i=0 ; i<npts ; i++) {                                           // propagate high bit up to sign bit
      unpacked_s[i] = unpacked_u[i] << (32 - nbits) ;
      unpacked_s[i] >>= (32 - nbits) ;
    } ;

    fprintf(stderr, "top: nbits = %2d, maskn = %8.8x, ", nbits, maskn) ;
    errors = test_pack_top(unpacked_u, unpacked_s, nbits, npts, 1) ;
#if defined(WITH_TIMING)
    TIME_LOOP_EZ(NITER, npts, test_pack_top(unpacked_u, unpacked_s, nbits, npts, 0))
    fprintf(stderr, "errors = %d %s\n", errors, timer_msg) ;
#else
    fprintf(stderr, "errors = %d\n", errors) ;
#endif
    if(errors > 0) exit(1) ;

    fprintf(stderr, "bot: nbits = %2d, maskn = %8.8x, ", nbits, maskn) ;
    errors = test_pack_bottom(unpacked_u, unpacked_s, nbits, npts, 1) ;
#if defined(WITH_TIMING)
    TIME_LOOP_EZ(NITER, npts, test_pack_top(unpacked_u, unpacked_s, nbits, npts, 0))
    fprintf(stderr, "errors = %d %s\n", errors, timer_msg) ;
#else
    fprintf(stderr, "errors = %d\n", errors) ;
#endif
    if(errors > 0) exit(1) ;
  }
  return 0;
}
