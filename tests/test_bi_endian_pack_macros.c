#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NITER 100
#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
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

void *pack_be_top(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  BE64_INSERT_BEGIN(accum, count) ;
  if(nbits < 9){
    while(n>3) {
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_CHECK(accum, count, pak_u) ; n -= 4 ;
    }
  }
  if(nbits < 17){
    while(n>1) {
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_CHECK(accum, count, pak_u) ; n -= 2 ;
    }
  }
  while(n--){ BE64_PUT_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; }
  BE64_INSERT_FINAL(accum, count, pak_u) ;
  return pak_u ;
}

void *pack_le_top(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  LE64_INSERT_BEGIN(accum, count) ;
  if(nbits < 9){
    while(n>3) {
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_CHECK(accum, count, pak_u) ; n -= 4 ;
    }
  }
  if(nbits < 17){
    while(n>1) {
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_CHECK(accum, count, pak_u) ; n -= 2 ;
    }
  }
  while(n--){ LE64_PUT_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; }
  LE64_INSERT_FINAL(accum, count, pak_u) ;
  return pak_u ;
}

void *unpack_be_top_u(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  BE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { BE64_GET_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

void *unpack_le_top_u(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  LE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { LE64_GET_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

void *unpack_be_top_s(void *unp, void *pak, int nbits, int n){
  int64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
  BE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { BE64_GET_NBITS(accum, count, *unp_s, nbits, pak_u) ; unp_s++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

void *unpack_le_top_s(void *unp, void *pak, int nbits, int n){
  int64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
  LE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { LE64_GET_NBITS(accum, count, *unp_s, nbits, pak_u) ; unp_s++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

int test_pack_top(void *src_u, void *src_s, int32_t nbits, int32_t n, int vfy){   // same as test_pack_bottom, but fill accumulator from MSB
  uint32_t *unpacked_u = (uint32_t *) src_u ;
  uint32_t buf_u[n], buf_s[n], restored_u[n], *pak_u, *unp_u, *pak_s, *unp_s ;
  int32_t  restored_s[n], *unpacked_s = (int32_t *) src_s ;
  int errors_u = 0, errors_s = 0 ;

  // big endian style packing/unpacking
  pak_u = pack_be_top(unpacked_u, buf_u, nbits, n) ;
  unp_u = unpack_be_top_u(restored_u, buf_u, nbits, n) ;
  if(pak_u != unp_u){
    fprintf(stderr, "buf_u = %16p, pak_u = %16p(%5ld), unp_u = %16p(%5ld)\n", (void *)buf_u, (void *)pak_u, pak_u-buf_u, (void *)unp_u, unp_u-buf_u) ;
    exit(1) ;
  }
  pak_s = pack_be_top(unpacked_s, buf_s, nbits, n) ;
  unp_s = unpack_be_top_s(restored_s, buf_s, nbits, n) ;
  if(pak_s != unp_s){
    fprintf(stderr, "buf_s = %16p, pak_s = %16p(%5ld), unp_s = %16p(%5ld)\n", (void *)buf_s, (void *)pak_s, pak_s-buf_s, (void *)unp_s, unp_s-buf_s) ;
    exit(1) ;
  }
  if(vfy){
    errors_u += w32_compare(unpacked_u, restored_u, n) ;                   // verify unsigned values
    errors_s += w32_compare(unpacked_s, restored_s, n) ;                   // verify signed values
  }

  // little endian style packing/unpacking
  pak_u = pack_le_top(unpacked_u, buf_u, nbits, n) ;
  unp_u = unpack_le_top_u(restored_u, buf_u, nbits, n) ;
  if(pak_u != unp_u){
    fprintf(stderr, "buf_u = %16p, pak_u = %16p(%5ld), unp_u = %16p(%5ld)\n", (void *)buf_u, (void *)pak_u, pak_u-buf_u, (void *)unp_u, unp_u-buf_u) ;
    exit(1) ;
  }
  pak_s = pack_le_top(unpacked_s, buf_s, nbits, n) ;
  unp_s = unpack_le_top_s(restored_s, buf_s, nbits, n) ;
  if(pak_s != unp_s){
    fprintf(stderr, "buf_s = %16p, pak_s = %16p(%5ld), unp_s = %16p(%5ld)\n", (void *)buf_s, (void *)pak_s, pak_s-buf_s, (void *)unp_s, unp_s-buf_s) ;
    exit(1) ;
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

void *pack_be_bot(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  BE64_INSERT_BEGIN(accum, count) ;
  if(nbits < 9){
    while(n>3) {
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_CHECK(accum, count, pak_u) ; n -= 4 ;
    }
  }
  if(nbits < 17){
    while(n>1) {
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      BE64_INSERT_CHECK(accum, count, pak_u) ; n -= 2 ;
    }
  }
  while(n--){ BE64_PUT_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; }
  BE64_INSERT_FINAL(accum, count, pak_u) ;
  return pak_u ;
}

void *pack_le_bot(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  LE64_INSERT_BEGIN(accum, count) ;
  if(nbits < 9){
    while(n>3) {
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_CHECK(accum, count, pak_u) ; n -= 4 ;
    }
  }
  if(nbits < 17){
    while(n>1) {
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_NBITS(accum, count, *unp_u, nbits) ; unp_u++ ;
      LE64_INSERT_CHECK(accum, count, pak_u) ; n -= 2 ;
    }
  }
  while(n--){ LE64_PUT_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; }
  LE64_INSERT_FINAL(accum, count, pak_u) ;
  return pak_u ;
}

void *unpack_be_bot_u(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  BE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { BE64_GET_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

void *unpack_le_bot_u(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  LE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { LE64_GET_NBITS(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

void *unpack_be_bot_s(void *unp, void *pak, int nbits, int n){
  int64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
  BE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { BE64_GET_NBITS(accum, count, *unp_s, nbits, pak_u) ; unp_s++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

void *unpack_le_bot_s(void *unp, void *pak, int nbits, int n){
  int64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
  LE64_XTRACT_BEGIN(accum, count, pak_u) ;
  while(n--) { LE64_GET_NBITS(accum, count, *unp_s, nbits, pak_u) ; unp_s++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u ;
}

int test_pack_bot(void *src_u, void *src_s, int32_t nbits, int32_t n, int vfy){   // same as test_pack_top, but fill accumulator from LSB
  uint32_t *unpacked_u = (uint32_t *) src_u ;
  uint32_t buf_u[n], buf_s[n], restored_u[n], *pak_u, *unp_u, *pak_s, *unp_s ;
  int32_t  restored_s[n], *unpacked_s = (int32_t *) src_s ;
  int errors_u = 0, errors_s = 0 ;

  // big endian style packing/unpacking
  pak_u = pack_be_bot(unpacked_u, buf_u, nbits, n) ;
  unp_u = unpack_be_bot_u(restored_u, buf_u, nbits, n) ;
  if(pak_u != unp_u){
    fprintf(stderr, "buf_u = %16p, pak_u = %16p(%5ld), unp_u = %16p(%5ld)\n", (void *)buf_u, (void *)pak_u, pak_u-buf_u, (void *)unp_u, unp_u-buf_u) ;
    exit(1) ;
  }
  pak_s = pack_be_bot(unpacked_s, buf_s, nbits, n) ;
  unp_s = unpack_be_bot_s(restored_s, buf_s, nbits, n) ;
  if(pak_s != unp_s){
    fprintf(stderr, "buf_s = %16p, pak_s = %16p(%5ld), unp_s = %16p(%5ld)\n", (void *)buf_s, (void *)pak_s, pak_s-buf_s, (void *)unp_s, unp_s-buf_s) ;
    exit(1) ;
  }
  if(vfy){
    errors_u += w32_compare(unpacked_u, restored_u, n) ;                   // verify unsigned values
    errors_s += w32_compare(unpacked_s, restored_s, n) ;                   // verify signed values
  }

  // little endian style packing/unpacking
  pak_u = pack_le_bot(unpacked_u, buf_u, nbits, n) ;
  unp_u = unpack_le_bot_u(restored_u, buf_u, nbits, n) ;
  if(pak_u != unp_u){
    fprintf(stderr, "buf_u = %16p, pak_u = %16p(%5ld), unp_u = %16p(%5ld)\n", (void *)buf_u, (void *)pak_u, pak_u-buf_u, (void *)unp_u, unp_u-buf_u) ;
    exit(1) ;
  }
  pak_s = pack_le_bot(unpacked_s, buf_s, nbits, n) ;
  unp_s = unpack_le_bot_s(restored_s, buf_s, nbits, n) ;
  if(pak_s != unp_s){
    fprintf(stderr, "buf_s = %16p, pak_s = %16p(%5ld), unp_s = %16p(%5ld)\n", (void *)buf_s, (void *)pak_s, pak_s-buf_s, (void *)unp_s, unp_s-buf_s) ;
    exit(1) ;
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
  int npts = 32*1024 + 17 ;
  uint32_t *unpacked_u, *packed ;
  int32_t  *unpacked_s ;
  int i, nbits, errors ;
  int32_t maskn ;
  TIME_LOOP_DATA ;
  float t0, t1, t2, t3, t4, t5 ;

  unpacked_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ; if(unpacked_u == NULL) exit(1) ;
  packed     = (uint32_t *) malloc(npts * sizeof(uint32_t)) ; if(packed == NULL) exit(1) ;
  unpacked_s = (int32_t *)  malloc(npts * sizeof(uint32_t)) ; if(unpacked_s == NULL) exit(1) ;

  fprintf(stderr, "timings :                                    pack(be/le) unp_u(be/le) unp_s(be/le) \n") ;
  for(nbits=1 ; nbits <33 ; nbits+=1) {

    maskn = ~((0xFFFFFFFFu) << nbits) ; maskn >>= 1 ;
    for(i=0 ; i<npts ; i+=1) { unpacked_u[i] = maskn & i ; } ;
    for(i=0 ; i<npts ; i+=2) { unpacked_u[i] |= (1 << (nbits-1)) ; } ;  // set high bit of every other value
    for(i=0 ; i<npts ; i++) {                                           // propagate high bit up to sign bit
      unpacked_s[i] = unpacked_u[i] << (32 - nbits) ; unpacked_s[i] >>= (32 - nbits) ;
    } ;

    fprintf(stderr, "bot: nbits = %2d, mask = %8.8x, ", nbits, maskn) ;
    errors = test_pack_bot(unpacked_u, unpacked_s, nbits, npts, 1) ;
    TIME_LOOP_EZ(NITER, npts, pack_be_bot(unpacked_u, packed, nbits, npts))
    t0 = timer_avg * NaNoSeC / (npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_be_bot_u(unpacked_u, packed, nbits, npts))
    t1 = timer_avg * NaNoSeC / (npts) ;
    pack_be_bot(unpacked_s, packed, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_be_bot_s(unpacked_s, packed, nbits, npts))
    t2 = timer_avg * NaNoSeC / (npts) ;
    TIME_LOOP_EZ(NITER, npts, pack_le_bot(unpacked_u, packed, nbits, npts))
    t3 = timer_avg * NaNoSeC / (npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_le_bot_u(unpacked_u, packed, nbits, npts))
    t4 = timer_avg * NaNoSeC / (npts) ;
    pack_le_bot(unpacked_s, packed, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_le_bot_s(unpacked_s, packed, nbits, npts))
    t5 = timer_avg * NaNoSeC / (npts) ;
    fprintf(stderr, "errors = %d   %4.2f/%4.2f    %4.2f/%4.2f    %4.2f/%4.2f ns/pt\n", errors, t0, t3, t1, t4, t2, t5) ;
    if(errors > 0) exit(1) ;

    fprintf(stderr, "top: nbits = %2d, mask = %8.8x, ", nbits, maskn) ;
    errors = test_pack_top(unpacked_u, unpacked_s, nbits, npts, 1) ;
    TIME_LOOP_EZ(NITER, npts, pack_be_top(unpacked_u, packed, nbits, npts))
    t0 = timer_avg * NaNoSeC / (npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_be_top_u(unpacked_u, packed, nbits, npts))
    t1 = timer_avg * NaNoSeC / (npts) ;
    pack_be_top(unpacked_s, packed, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_be_top_s(unpacked_s, packed, nbits, npts))
    t2 = timer_avg * NaNoSeC / (npts) ;
    TIME_LOOP_EZ(NITER, npts, pack_le_top(unpacked_u, packed, nbits, npts))
    t3 = timer_avg * NaNoSeC / (npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_le_top_u(unpacked_u, packed, nbits, npts))
    t4 = timer_avg * NaNoSeC / (npts) ;
    pack_le_top(unpacked_s, packed, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_le_top_s(unpacked_s, packed, nbits, npts))
    t5 = timer_avg * NaNoSeC / (npts) ;
    fprintf(stderr, "errors = %d   %4.2f/%4.2f    %4.2f/%4.2f    %4.2f/%4.2f ns/pt\n", errors, t0, t3, t1, t4, t2, t5) ;
    if(errors > 0) exit(1) ;

    fprintf(stderr, "\n") ;

  }   // for(nbits
  return 0;
}
