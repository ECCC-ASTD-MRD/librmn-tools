// Hopefully useful code for C
// Copyright (C) 2022  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//

#include <stdio.h>
#include <rmn/tile_encoders.h>

int32_t encode_ints(void *f, int ni, int nj, bitstream *s){
  int32_t *fi = (int32_t *) f ;
  uint32_t *fu = (uint32_t *) f ;
  uint32_t fe[64] ;
  int i, nbtot, nbits, nbits0, nshort, nzero, nbits_short, nbits_zero, nbits_full ;
  int nij = ni * nj ;
  tile_header th ;
  uint32_t vneg, vpos, vmax, vmin, w32, mask0 ;
  uint64_t accum   = s->acc_i ;
  int32_t  insert  = s->insert ;
  uint32_t *stream = s->in ;

  nbtot = 0 ;
  if( (ni < 1) || (nj < 1) || (ni > 8) || (nj > 8) ) goto error ;
  if( (f == NULL) || (s == NULL) ) goto error ;

  th.s = 0 ;
  th.h.npti = ni-1 ;             // block dimensions (1->8)
  th.h.nptj = nj-1 ;
  // ==================== analysis phase ====================
  // can be accelerated using SIMD instructions
  vneg = 0xFFFFFFFFu ;
  vpos = 0xFFFFFFFFu ;
  vmin = 0xFFFFFFFFu ;
  vmax = 0 ;
  for(i=0 ; i<nij ; i++){
    vneg &= fu[i] ;              // MSB will be 1 only if all values < 0
    vpos &= (~fu[i]) ;           // MSB will be 1 only if all values >= 0
    fe[i] = to_zigzag_32(fi[i]) ;
    vmax = (fe[i] > vmax) ? fe[i] : vmax ;  // largest ZigZag value
    vmin = (fe[i] < vmin) ? fe[i] : vmin ;  // smallest ZigZag value
  }
  nbits = 32 - lzcnt_32(vmax) ;  // vmax controls nbits (if vmax is 0, nbits will be 0)
  if(vmax == 0) goto zero ;      // special case (block only contains 0s)
  vneg >>= 31 ;                  // 1 if all values < 0
  vpos >>= 31 ;                  // 1 if all values >= 0
  if(vneg || vpos) {             // no need to encode sign, it is the same for all values
    nbits-- ;                    // and will be found in the tile header
    for(i=0 ; i<nij ; i++)
      fe[i] >>= 1 ;              // get rid of sign bit from ZigZag
    th.h.sign = (vneg << 1) | vpos ;
  }else{
    th.h.sign = 3 ;              // both positive and negative values, ZigZag encoding with sign bit
  }
  nbits0 = (nbits + 1) >> 1 ;    // number of bits for "short" values
  mask0 = RMASK31(nbits0) ;      // value & mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;               // keep only the upper bits
fprintf(stderr, "vmax = %8.8x(%8.8x), vneg = %d, vpos = %d, nbits = %d, nbits0 = %d\n", vmax, vmax>>1, vneg, vpos, nbits, nbits0) ;
  th.h.nbts = nbits ;            // number of bits neeeded for encoding
  th.h.encd = 0 ;                // a priori no special encoding
  // check if we should use short/full or 0/full encoding
  nshort = nzero = 0 ;
  for(i=0 ; i<nij ; i++){
    if(fe[i] == 0)           nzero++ ;        // zero value
    if((fe[i] & mask0) == 0) nshort++ ;       // value using nbits0 bits or less
  }
  nbits_full  = nij * nbits ;                                            // all tokens at full length
  nbits_zero  = nzero + (nij - nzero) * (nbits + 1) ;                    // nzero tokens at length 1, others nbits + 1
  nbits_short = nshort * (nbits0 + 1) + (nij - nshort) * (nbits + 1) ;   // short tokens tokens nbits0 + 1, others nbits + 1
  if( (nbits_short <= nbits_zero)  && (nbits_short < nbits_full) ) th.h.encd = 1 ;  // 0 short / 1 full encoding
  if( (nbits_zero  <= nbits_short) && (nbits_zero  < nbits_full) ) th.h.encd = 2 ;  // 0 / 1 full encoding
  //
th.h.encd = 0 ; // force no encoding
  // ==================== encoding phase ====================
  // totally scalar
  w32 = th.s ;
fprintf(stderr, "sign = %d, encoding = %d, nshort = %2d, nzero = %2d\n", th.h.sign, th.h.encd, nshort, nzero) ;
fprintf(stderr, "nbits_full = %4d, nbits_zero = %4d, nbits_short = %4d, mask0 = %8.8x\n", nbits_full, nbits_zero, nbits_short, mask0) ;
  BE64_PUT_NBITS(accum, insert, w32, 16, stream) ; // insert header into packed stream
  nbtot = 16 ;
  // 3 mutually exclusive alternatives
  if(th.h.encd == 1){             // 0 short / 1 full encoding
    for(i=0 ; i<nij ; i++){
      if((fe[i] & mask0) == 0){                                   // value uses nbits0 bits or less
        BE64_PUT_NBITS(accum, insert, fe[i], nbits0+1, stream) ;  // insert nbits0+1 bits into stream
        nbtot += (nbits0+1) ;
      }else{
        fe[i] |= (1 << nbits) ;
        BE64_PUT_NBITS(accum, insert, fe[i],  nbits+1, stream) ;  // insert nbits+1 bits into stream
        nbtot += (nbits+1) ;
      }
    }
    goto end ;
  }
  if(th.h.encd == 2){             // 0 / 1 full encoding
    for(i=0 ; i<nij ; i++){
      if(fe[i] == 0){                                             // value is zero
        BE64_PUT_NBITS(accum, insert,     0,       1, stream) ;   // insert 1 bit (0) into stream
        nbtot += 1 ;
      }else{
        fe[i] |= (1 << nbits) ;                                   // add 1 bit to identify "full" value
        BE64_PUT_NBITS(accum, insert, fe[i], nbits+1, stream) ;   // insert nbits+1 bits into stream
        nbtot += (nbits+1) ;
      }
    }
    goto end ;
  }
  if(th.h.encd == 0){             // no encoding
    for(i=0 ; i<nij ; i++){
      BE64_PUT_NBITS(accum, insert, fe[i], nbits, stream) ;       // insert nbits bits into stream
      nbtot += nbits ;
    }
    goto end ;
  }

end:
  s->acc_i  = accum ;
  s->insert = insert ;
  s->in     = stream ;
  return nbtot ;

error:
  nbtot = -1 ;
  goto end ;

zero:
  th.h.nbts = 0 ;   // 1 bit
  th.h.encd = 3 ;   // constant block
  th.h.sign = 1 ;   // all values >= 0
  w32 = th.s ;
  w32 <<= 1 ;       // header + 1 bit with value 0
  BE64_PUT_NBITS(accum, insert, w32, 17, stream) ; // insert header and 0 into packed stream
  nbtot = 17 ;      // total of 17 bits inserted
  goto end ;
}

// encode 64 values (eventually SIMD version)
int32_t encode_64_ints(void *f, bitstream *stream){
  return encode_ints(f, 8, 8, stream) ;
}

// decode 64 values (SIMD version)
int32_t decode_64_ints(void *f, bitstream *stream, uint16_t h16){
  tile_header th ;
  th.s = h16 ;
  return 0 ;
}

int32_t decode_ints(void *f, int *ni, int *nj, bitstream *s){
  int32_t *fi = (int32_t *) f ;
  uint32_t *fu = (uint32_t *) f ;
  uint32_t fe[64] ;
  tile_header th ;
  uint32_t w32 ;
  int i, nbits, nij ;
  uint64_t accum   = s->acc_x ;
  int32_t  xtract  = s->xtract ;
  uint32_t *stream = s->out ;

  if( (f == NULL) || (stream == NULL) ) goto error ;
  BE64_GET_NBITS(accum, xtract, w32, 16, stream) ;
  th.s = w32 ;
  *ni = th.h.npti+1 ;
  *nj = th.h.nptj+1 ;
  nij = (*ni) * (*nj) ;
  nbits = th.h.nbts ;
  if(th.h.encd == 3) goto constant ;

//   BeStreamXtract(stream, fe, nbits, nij) ;   // extract nij tokens
  if(th.h.encd == 1){             // 0 short / 1 full encoding
    for(i=0 ; i<nij ; i++){
    }
    goto transfer ;
  }
  if(th.h.encd == 2){             // 0 / 1 full encoding
    for(i=0 ; i<nij ; i++){
    }
    goto transfer ;
  }
  if(th.h.encd == 0){             // no encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, fe[i], nbits, stream) ;
    }
    goto transfer ;
  }
transfer:
  if(th.h.sign == 1){    // all values >= 0
    for(i=0 ; i<nij ; i++){ fi[i] = fe[i] ; }
  }
  if(th.h.sign == 2){    // all values < 0
    for(i=0 ; i<nij ; i++){ fi[i] = ~fe[i] ; }  // undo shifted ZigZag
  }
  if(th.h.sign == 3){    // ZigZag
    for(i=0 ; i<nij ; i++){ fi[i] = from_zigzag_32(fe[i]) ; }
  }

end:
  s->acc_x  = accum ;
  s->xtract = xtract ;
  s->out    = stream ;
  return 0 ;

error:
  return -1 ;

constant:
  BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;            // extract nbits bits
  for(i=0 ; i<nij ; i++){ fi[i] = w32 ; }                        // plus constant value
  goto end ;
}

