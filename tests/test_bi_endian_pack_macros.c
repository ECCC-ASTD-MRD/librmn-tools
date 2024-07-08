#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// big and little endian packing macros
#include <rmn/bit_pack_macros.h>

typedef struct{
  uint64_t accum_in, accum_out ;
  uint32_t *stream ;
  uint32_t insert, extract;
}wstream ;

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
  int (*fn)() = &w32_compare ;
  int npts = 4096 ;
  wstream be_u, le_u ;
  wstream be_s, le_s ;
  uint32_t *packed_u, *packed_s ;
  uint32_t *unpacked_u, *restored_u ;
  int32_t *unpacked_s, *restored_s ;
  int i ;

  packed_u   = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  packed_s   = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  unpacked_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  unpacked_s = (int32_t  *) malloc(npts * sizeof(int32_t) ) ;
  restored_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ;
  restored_s = (int32_t  *) malloc(npts * sizeof(int32_t) ) ;

  fprintf(stderr, "test bi_endian pack macros\n") ;
  fprintf(stderr, "address of function = %16.16lx\n", (uint64_t) fn) ;
  fprintf(stderr, "address of function = %16.16lx\n", (uint64_t) &w32_compare) ;
  return 0;
}
