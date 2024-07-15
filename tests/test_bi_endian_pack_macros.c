#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct{
  uint64_t accum_in, accum_out ;
  uint32_t *stream ;
  uint32_t insert, extract;
}wstream_u ;

typedef struct{
  uint64_t accum_in ;
  int64_t accum_out ;
  uint32_t *stream ;
  uint32_t insert, extract;
}wstream_s ;

int w32_compare(void *wold, void *wnew, int n){
  uint32_t *old = (uint32_t *) wold ;
  uint32_t *new = (uint32_t *) wnew ;
  int errors = 0 ;
  while(n--){
    errors += (*old != *new) ? 1 : 0 ;
  }
  return errors ;
}

int main(int argc, char **argv){
//   int (*fn)() = &w32_compare ;
  int npts = 4096 ;
  wstream_u be_u ;
//   wstream le_u ;
  wstream_s be_s ;
//   wstream le_s ;
  uint32_t *packed_u, *packed_s ;
  uint32_t *unpacked_u ;
  int32_t *unpacked_s ;
  uint32_t *restored_u ;
  int32_t *restored_s ;
  int i, nbits, masks ;
  uint32_t maskn ;

  packed_u   = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  packed_s   = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  unpacked_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  unpacked_s = (int32_t  *) malloc(npts * sizeof(int32_t) ) ;
  restored_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  restored_s = (int32_t  *) malloc(npts * sizeof(int32_t) ) ;

  for(nbits=4 ; nbits <12 ; nbits+=4) {

    maskn = ~((0xFFFFFFFFu) << (32-nbits)) ;
    maskn >>= 1 ;
    masks = (-1) << (nbits - 1) ;
    for(i=0 ; i<npts ; i++) { 
      unpacked_u[i] = maskn & i ;
      unpacked_s[i] = unpacked_u[i] | masks ;
    } ;
    fprintf(stderr, "orign__u :") ;
    for(i=0 ; i<8 ; i++) { fprintf(stderr, " %8x", unpacked_u[i]) ; } ; fprintf(stderr, "\n") ;
    fprintf(stderr, "orign__s :") ;
    for(i=0 ; i<8 ; i++) { fprintf(stderr, " %8x", unpacked_s[i]) ; } ; fprintf(stderr, "\n") ;

    fprintf(stderr, "\nbi_endian pack macros (FILL_FROM_TOP) %2d bits\n", nbits) ;
// big and little endian packing macros
#define FILL_FROM_TOP
#include <rmn/bit_pack_macros.h>
    BE64_INSERT_BEGIN(be_u.accum_in, be_u.insert) ; be_u.stream = packed_u ;
    BE64_INSERT_BEGIN(be_s.accum_in, be_s.insert) ; be_s.stream = packed_s ;
    for(i=0 ; i<npts ; i++) {
      BE64_PUT_NBITS(be_u.accum_in, be_u.insert, (unpacked_u[i]), nbits, (be_u.stream)) ;
      BE64_PUT_NBITS(be_s.accum_in, be_s.insert, unpacked_s[i], nbits, be_s.stream) ;
    }
    BE64_INSERT_FINAL(be_u.accum_in, be_u.insert, be_u.stream) ;
    BE64_INSERT_FINAL(be_s.accum_in, be_s.insert, be_s.stream) ;
    fprintf(stderr, "insert = %6d accum = %16.16lx\n", be_u.insert, be_u.accum_in) ;
    fprintf(stderr, "packed_u :") ;
    for(i=0 ; i<8 ; i++) { fprintf(stderr, " %8.8x", packed_u[i]) ; } ; fprintf(stderr, "\n") ;
    fprintf(stderr, "packed_s :") ;
    for(i=0 ; i<8 ; i++) { fprintf(stderr, " %8.8x", packed_s[i]) ; } ; fprintf(stderr, "\n") ;

    be_u.stream = packed_u ; BE64_XTRACT_BEGIN(be_u.accum_out, be_u.extract, be_u.stream) ; 
    be_s.stream = packed_s ; BE64_XTRACT_BEGIN(be_s.accum_out, be_s.extract, be_s.stream) ; 
    for(i=0 ; i<npts ; i++) {
      BE64_GET_NBITS(be_u.accum_out, be_u.extract, restored_u[i], nbits, be_u.stream)
      BE64_GET_NBITS(be_s.accum_out, be_s.extract, restored_s[i], nbits, be_s.stream)
    }
    fprintf(stderr, "restor_u :") ;
    for(i=0 ; i<8 ; i++) { fprintf(stderr, " %8x", restored_u[i]) ; } ; fprintf(stderr, "\n") ;
    fprintf(stderr, "restor_s :") ;
    for(i=0 ; i<8 ; i++) { fprintf(stderr, " %8x", restored_s[i]) ; } ; fprintf(stderr, "\n") ;
#if 0
    fprintf(stderr, "\nbi_endian pack macros (FILL_FROM_BOTTOM) %2d bits\n", nbits) ;
// big and little endian packing macros
#define FILL_FROM_BOTTOM
#include <rmn/bit_pack_macros.h>
#endif
    fprintf(stderr, "\n") ;
  }
  return 0;
}
