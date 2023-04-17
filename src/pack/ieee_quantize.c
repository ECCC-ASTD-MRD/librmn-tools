/* Hopefully useful routines for C and FORTRAN
 * Copyright (C) 2021  Recherche en Prevision Numerique
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>

#include <rmn/misc_operators.h>
#include <rmn/tools_types.h>
#include <rmn/ieee_quantize.h>

#if ! defined(STATIC)
#define STATIC static
#endif

// some (useful for quantization) properties of an IEEE float array, 64 bits total
// types ieee32_props and ieee32_p are kept internal, uint64_t is exposed in the interface
typedef struct{
  uint64_t bias:32,  // minimum absolute value (not more thatn 31 bits)
           npts:16,  // number of points, 0 means unknown
           shft:8 ,  // shift count (0 -> 31) for quanze/unquantize
           nbts:6 ,  // number of bits (0 -> 31) per value
           allp:1,   // all numbers are >= 0
           allm:1;   // all numbers are < 0
} ieee32_p;

typedef union{    // the union allows to transfer the whole 64 bit contents in one shot
  ieee32_p p ;
  uint64_t u ;
} ieee32_props ;

// restore floating point numbers quantized with linear_quantize_ieee32
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// nbits [IN]  number of bits in quantized data items (used for checking purposes only)
// u64   [IN]  metadata information describing quantization (from linear_quantize_ieee32)
int linear_unquantize_ieee32(void * restrict q, uint64_t u64, int ni, int nbits, void * restrict f){
  int i0, i, ni7 ;
  int scount ;
  int32_t offset ;
  uint32_t *qi = (uint32_t *) q ;
  int32_t *fo = (int32_t *) f ;
  int pos_neg ;  // not all >=0
  int sign ;
  uint32_t temp ;
  ieee32_props h64 ;

  h64.u = u64 ;
  if(h64.p.npts == 0) h64.p.npts = ni ;
  if(h64.p.npts != ni) {
    fprintf(stderr, "ERROR : inconsistent number of points, expecting %d, got %d\n", ni, h64.p.npts) ;
    return 2 ;      // inconsistent number of values
  }
  scount = h64.p.shft ;                // shift count
  offset = h64.p.bias >> scount ;      // bias before shifting
  pos_neg = (! h64.p.allp) ;  // not all >=0

  ni7 = (ni & 7) ;
  if(q == f) {        // restore IN PLACE
    if(h64.p.nbts == 0){
      sign = h64.p.allm ? (1u << 31) : 0 ;
      for(i=0 ; i<ni ; i++) qi[i] = h64.p.bias | sign ;
      goto end ;
    }
    if(pos_neg){       // mix of positive and negative values
      for(i=0 ; i<(ni & 7) ; i++){                       // first chunk (0 - > 7 items)
        temp = qi[i] ;
        sign = (temp & 1) << 31 ;                        // get sign
        temp >>= 1 ;                                     // remove sign
        qi[i] = (temp + offset) << scount ;              // unquantize
        qi[i] |= sign ;                                  // propagate sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){                     // 8 items chnumks
        for(i=0 ; i<8 ; i++){
          temp = qi[i0+i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          qi[i0+i] = (temp + offset) << scount ;         // unquantize
          qi[i0+i] |= sign ;                             // propagate sign
        }
      }
    }else{              // positive values only
      for(i=0 ; i<(ni & 7) ; i++){                       // first chunk (0 - > 7 items)
        qi[i] = (qi[i] + offset) << scount ;             // unquantize
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){                     // 8 items chnumks
        for(i=0 ; i<8 ; i++){
          qi[i0+i] = (qi[i0+i] + offset) << scount ;     // unquantize
        }
      }
    }
  }else{        // restore NOT IN PLACE
    if(h64.p.nbts == 0){
      sign = h64.p.allm ? (1u << 31) : 0 ;
      for(i=0 ; i<ni ; i++) fo[i] = h64.p.bias | sign ;
      goto end ;
    }
    if(pos_neg){
      for(i=0 ; i<(ni & 7) ; i++){                       // first chunk (0 - > 7 items)
          temp = qi[i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          fo[i] = (temp + offset) << scount ;            // unquantize
          fo[i] |= sign ;                                // propagate sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=ni7, ni7=8){
        for(i=0 ; i<8 ; i++){
          temp = qi[i0+i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          fo[i0+i] = (temp + offset) << scount ;         // unquantize
          fo[i0+i] |= sign ;                             // propagate sign
        }
      }
    }else{
      for(i=0 ; i<(ni & 7) ; i++){                       // first chunk (0 - > 7 items)
        fo[i] = (qi[i] + offset) << scount ;             // unquantize
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=ni7, ni7=8){
        for(i=0 ; i<8 ; i++){
          fo[i0+i] = (qi[i0+i] + offset) << scount ;         // unquantize
        }
      }
    }
  }
end:
  return 0 ;
}

// quantize IEEE floats
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// ni    [IN]  number of data items toquantize
// nbits [IN]  number of bits to use for quantized data
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
uint64_t linear_quantize_ieee32(void * restrict f, int ni, int nbits, float quantum, void * restrict qs){
//   float q = quantum_adjust(quantum) ;
  uint32_t *fu = (uint32_t *) f ;
  uint32_t *qo = (uint32_t *) qs ;
  int i0, i, ni7 ;
  uint32_t maxu[8], minu[8], t[8], ands[8], ors[8], rangeu, lz, offset, round, maskn, masksign ;
  int32_t scount, nbitsmax ;
  ieee32_props h64 ;
  uint32_t pos_neg, allm, allp, sign ;
  float delta ;
  FloatInt fi1, fi2 ;

// ==================================== analyze ====================================

  masksign = RMASK31(31) ;  // sign bit is 0, all others are 1
  h64.u = 0 ;
  for(i=0 ; i<8 ; i++){
    maxu[i] = ors[i]  = 0u ;     // 0
    minu[i] = ands[i] = ~0u ;    // FFFFFFFF
  }
  ni7 = (ni & 7) ? (ni & 7) : 8 ;
  for(i0=0 ; i0<ni-7 ; i0+=ni7, ni7=8){
    for(i=0 ; i<8 ; i++){
      t[i] = fu[i0+i] & masksign ;        // get rid of sign
      maxu[i]  = MAX( maxu[i], t[i]) ;    // largest absolute value
      minu[i]  = MIN( minu[i], t[i]) ;    // smallest absolute value
      ands[i] &= fu[i0+i] ;
      ors[i]  |= fu[i0+i] ;
    }
  }
  for(i=0 ; i<8 ; i++){        // fold 8 long vector into single value
    maxu[0]  = MAX( maxu[0], maxu[i]) ;
    minu[0]  = MIN( minu[0], minu[i]) ;
    ands[0] &= ands[i] ;       // will be 1 if all values are < 0, will be 0 if any value is >= 0
    ors[0]  |= ors[i] ;        // will be 0 if all values >= 0, will be 1 if any value is <0
  }
  allm = ands[0] >> 31 ;
  pos_neg = ors[0] >> 31 ;
  allp = ! pos_neg ;
  rangeu = maxu[0] - minu[0] ;
  lz = lzcnt_32(rangeu) ;
  nbitsmax = 32 -lz ;        // 32 - number of most significant 0 bits in rangeu = max number of effective bits

  if(rangeu == 0) {          // special case : constant absolute values
    scount = 0 ;
    nbits = pos_neg ;
    if(pos_neg){
      if(f == qs){
        for(i=0 ; i<ni ; i++) fu[i] = (fu[i] >> 31) ;
      }else{
        for(i=0 ; i<ni ; i++) qo[i] = (fu[i] >> 31) ;
      }
    }
    goto end ;
  }else{
    if( (nbits <= 0) ) {    // automatic determination of nbits
      int32_t temp ;
      fi1.u = maxu[0] ; fi2.u = minu[0] ; fi1.f = fi1.f - fi2.f ;
      fi2.f = quantum ;
      if(fi2.u <= 0){          // quantum <= 0
        nbits = pos_neg + 1 ;  // make sure nbits does not become zero if positive and negative values are present
      }else{
        temp = (fi1.u >> 23) - (fi2.u >> 23) ;  // IEEE exponent difference between range and quantum
        nbits = temp + 1 + pos_neg ;
        fprintf(stderr,"provisional nbits = %d\n", nbits) ;
      }
    }
  }
  if(pos_neg) {       // sign bit needs one bit, reduce allowed bit count by 1, increase nbitsmax
    nbitsmax++ ;
    nbits-- ;
  }
  if(nbits > nbitsmax) nbits = nbitsmax ;

  scount = 32 - lz - nbits ; scount = (scount < 0) ? 0 : scount ;
  round = scount ? 1 << (scount-1) : 0 ;
  offset = minu[0] >> scount ;
  if(rangeu ==0){          // constant absolute values
    delta = 0.0f ;
  }else{
    fi1.u = (offset << scount) ; fi2.u = ((offset+1) << scount) ;
    delta = fi2.f - fi1.f ;  // difference between values whose quntization differs by 1 unit
  }
// fprintf(stderr,"nbits = %d, nbitsmax = %d, range = %d, scount = %d, round = %d, delta = %8.4g, ni7 = %d, pos_neg = %d, minu[0] = %d, allp/m = %d/%d\n", 
//         nbits, nbitsmax, rangeu, scount, round, delta, ni&7, pos_neg, minu[0], allp, allm) ;
// adjust nbits if a non zero value was given for quantum nbits must be such that delta <= quantum if possible
  if( (quantum < delta) && (quantum > 0.0f) ) {
    fprintf(stderr,"quantum (%g) < delta (%g), nbits may need to be adjusted\n", quantum, delta) ;
    while( (quantum<delta) && (nbits<nbitsmax) ){
      nbits++ ;
      scount = 32 - lz - nbits ; scount = (scount < 0) ? 0 : scount ;
      round = scount ? 1 << (scount-1) : 0 ;
      offset = minu[0] >> scount ;
      fi1.u = (offset << scount) ; fi2.u = ((offset+1) << scount) ;
      delta = fi2.f - fi1.f ;  // difference between values whose quntization differs by 1 unit
    }
    fprintf(stderr,"adjusted nbits = %d, scount = %d, round = %d, delta = %8.2g\n", nbits, scount, round, delta) ;
  }else{
//     fprintf(stderr,"quantum (%g) >= delta (%g) or quantum == 0.0, no adjustment needed\n", quantum, delta);
  }
  
// ==================================== quantize ====================================

  maskn = RMASK31(nbits) ;
  ni7 = (ni & 7) ;
  if(f == qs){      // quantize IN PLACE
    if(pos_neg){
      for(i=0 ; i<(ni & 7) ; i++){
        sign = fu[i] >> 31 ;                                          // save sign
        fu[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
        fu[i] = MIN(fu[i],maskn) ;
        fu[i] = (fu[i] << 1) | sign ;                                 // store sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          sign = (fu[i0+i] >> 31) ;                                           // save sign
          fu[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          fu[i0+i] = MIN(fu[i0+i],maskn) ;
          fu[i0+i] = (fu[i0+i] << 1) | sign ;                                 // store sign
        }
      }
    }else{
      for(i=0 ; i<(ni & 7) ; i++){
        fu[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
        fu[i] = MIN(fu[i],maskn) ;
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          fu[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          fu[i0+i] = MIN(fu[i0+i],maskn) ;
        }
      }
    }
  }else{      // quantize NOT IN PLACE
    if(pos_neg){
      for(i=0 ; i<(ni & 7) ; i++){
        sign = fu[i] >> 31 ;                                          // save sign
        qo[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
        qo[i] = MIN(qo[i],maskn) ;
        qo[i] = (qo[i] << 1) | sign ;                                 // store sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          qo[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          qo[i0+i] = MIN(qo[i0+i],maskn) ;
          qo[i0+i] = (qo[i0+i] << 1) | (fu[i0+i] >> 31) ;                     // store sign
        }
      }
    }else{
      for(i=0 ; i<(ni & 7) ; i++){
        qo[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;          // quantize
        qo[i] = MIN(qo[i],maskn) ;
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          qo[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          qo[i0+i] = MIN(qo[i0+i],maskn) ;
        }
      }
    }
  }
end:                         // update returned struct
  h64.p.shft = scount ;
  h64.p.nbts = nbits ;
  h64.p.npts = ni ;
  h64.p.allp = allp ;
  h64.p.allm = allm ;
  h64.p.bias = minu[0] ;
  return h64.u ;             // return 64 bit aggregate
}

// largest exponent as a function of exponent bit field width
static int32_t e[] = {0, 1, 3, 7, 15, 31, 63, 127, 255} ;

#define VL 8

// vectorised by VL (POWER of 2) version
void quantize_setup(float *z,            // array to be quantized (IEEE 754 32 bit float) (INPUT)
                        int n,           // number of data elements
                        qhead *h)        // quantization control information (OUTPUT)
{
  int i, j, j0 ;
  float mi[VL], ma[VL], mia[VL], za[VL], z0[VL] ;
  FloatInt rng ;
  float maxabs, minabs ;

  for(i=0 ; i<VL && i<n ; i++) z0[i] = z[i] ;
  for(    ; i<VL ; i++) z0[i] = z[0] ;              // if less than VL values, pad with z0[0]

  for(i=0 ; i < VL ; i++){
    mi[i]  = z0[i] ;                                // minimum value
    ma[i]  = z0[i] ;                                // maximum value
    mia[i] = (z0[i] > 0) ? z0[i] : -z0[i] ;         // smallest absolute value (larger than 0)
  }

  j0 = n & (VL-1) ;                               // skip first mod(n,VL) values
  if(j0 == 0) j0 = VL ;                           // n was a multiple of VL
  for(j=j0 ; j<(n-VL+1) ; j+=VL){
    for(i=0; i<VL ; i++){                         // loops not fused to help gcc optimizer
      ma[i] = MAX(ma[i] , z[j+i]);                // maximum signed value
      mi[i] = MIN(mi[i] , z[j+i]);                // minimum signed value
    }
    for(i=0; i<VL ; i++){                         // loops not fused to help gcc optimizer
      za[i]  = z[j+i]  > 0 ? z[j+i] : -z[j+i] ;   // absolute value of z[j+i]
      za[i]  = z[j+i] != 0 ? za[i]  : mia[i] ;    // if 0, replace with current absolute value minimum
      mia[i] = MIN(mia[i], za[i]) ;               // minimum non zero absolute value 
    }
  }
  for(i=1; i<VL ; i++) {               // final folding pass
    ma[0]  = MAX(ma[0] , ma[i]);
    mi[0]  = MIN(mi[0] , mi[i]);
    mia[0] = MIN(mia[0] , mia[i]);
  }
  h->fmax = ma[0];                             // highest value
  h->fmin = mi[0];                             // lowest value
  h->amin = mia[0] ;                           // smallest non zero absolute value
  h->nbits = 0 ;                               // to be set later
  h->sbit = (h->fmax * h->fmin < 0) ? 1 : 0 ;  // a sign bit is needed, there are positive and negative numbers
  h->negative = ((h->fmax * h->fmin >= 0) && (h->fmin < 0)) ? 1 : 0 ;  // all values are negative
  rng.f = h->fmax - h->fmin ;                     // signed range
  rng.u = ((rng.u >> 23) + 1) << 23 ;             // next power of 2 > rng.f
  h->rng = rng.f ;                                // signed range
  maxabs = h->fmax >= 0 ? h->fmax : -h->fmax ;    // |maxval|
  minabs = h->fmin >= 0 ? h->fmin : -h->fmin ;    // |minval|
  maxabs = maxabs > minabs ? maxabs : minabs ;    // max( |maxval| , |minval| )
  h->fmaxa = maxabs ;
  rng.f = maxabs - h->amin ;
  rng.u = ((rng.u >> 23) + 1) << 23 ;             // next power of 2 > rng.f
  h->rnga = rng.f ;                               // range of absolute values
}

// only keep nbits in the mantissa of IEEE 754 floating point numbers
void ieee_clip(void *f, int n, int nbits){
  int i, j ;
  uint32_t *fi = (uint32_t *) f ;
  uint32_t mask = 0xFFFFFFFF ;

  mask >>= (32 -(23-nbits))  ; // lower 23 -nbits bits
  mask = ~mask ;
  for(i=0 ; i<4 ; i++) fi[i] = fi[i] & mask ;
  for(j=(n&3) ; j<n ; j+=4){
    for(i=0; i<4 ; i++) fi[i+j] = fi[i+j] & mask ;
  }
}

// transform a float into an integer (ieee style)
// f     : float to be quantized
// e0    : true IEEE exponent of largest absolute value (no 127 bias)
// round : mantissa rounding (integer, single bit in appropriate potition)
// t0    : float scaling factor
// nm    : number of effective mantissa bits ( 1 - 23 )
// limit : maximum permitted absolute value once quantized (normally right mask of nbits bits)
// sbit  : (0/1) mask for sign bit. if 0, sign will be ignored
//
// return signed quantized (ieee style) integer value for f
// the absolute value of the input float is converted to a new floating format
// with a reduced size exponent and a reduced size mantissa (nm bits)
// this new "reduced size float" is treated as an integer when restoring sign if needed
STATIC inline int32_t ieee_f_to_q(float f, int32_t e0, int32_t round, float t0, int nm, int32_t limit, int32_t sbit){
  FloatInt z, x1, y ;
  int sign, ex ;
  int q ;                       // final result
  z.f = f ;                     // float will be mostly manipulated as an unsigned integer
  sign = z.u >> 31 ;            // get sign bit (most significant bit)
  sign = sign & sbit ;          // possibly ignore sign bit
  z.u &= 0x7FFFFFFF ;           // suppress FP sign bit
  z.u += round ;                // apply mantissa rounding (may cause an exponent increase by 1)
  ex = (z.u >> 23) ;            // get exponent (including IEEE bias of 127)
  x1.u = ((ex - e0) << 23) |    // alter exponent (largest value becomes 127), keep mantissa intact
         (z.u & 0x7FFFFF) ;     // (equivalent to dividing by 2**e0)
  y.f = x1.f * t0 ;             // apply scaling (may produce a denormalized float)
  q = y.u >> (23 - nm) ;        // get rid of unused rightmost mantissa bits
  q = q > limit ? limit : q ;   // limit is expected to have the nbits rightmost bits set
  q = sign ? -q : q ;           // restore sign by negating integer quantized value if necessary
  return q ;
}

// restore the float value from the transformed value
// q    : quantized value representing a float
// t1   : first scaling factor
// t2   : second scaling factor
// nm   : number of effective mantissa bits ( 1 - 23 )
// sbit : (0/1) mask for sign bit. if 0, sign will be ignored
// neg  : (0/1) restored float value will be negative if 1 (usually sbit is 0 if neg is 1)
//
// return restored float value
// t1 * t2 might generate an overflow, which is why they MUST be applied separately using 2 multiplies
STATIC inline float ieee_q_to_f_2(int32_t q, float t1, float t2, int nm, int32_t sbit, int32_t neg){
  int sign ;
  FloatInt q1 ;
  float f ;                     // final result
  sign = (q < 0) ? 1 : 0 ;      // get sign
  sign = sign & sbit ;          // possibly unsigned quantized value
  q1.u = sign ? -q : q ;        // get absolute value if signed
  q1.u = q1.u << (23 - nm) ;    // shift left into proper place
  f = (q1.f * t1) * t2 ;        // apply the 2 scaling factors
  sign = sign | neg ;
  f = sign ? -f : f ;           // restore sign if negative
  return f ;
}
// single factor version if t1 * t2 are known not to create an overflow
STATIC inline float ieee_q_to_f_1(int32_t q, float t1t2, int nm, int32_t sbit, int32_t neg){
  int sign ;
  FloatInt q1 ;
  float f ;                     // final result
  sign = (q < 0) ? 1 : 0 ;      // get sign
  sign = sign & sbit ;          // possibly unsigned quantized value
  q1.u = sign ? -q : q ;        // get absolute value if signed
  q1.u = q1.u << (23 - nm) ;    // shift left into proper place
  f = q1.f * t1t2 ;             // apply the combined scaling factor
  sign = sign | neg ;
  f = sign ? -f : f ;           // restore sign if negative
  return f ;
}

// transform IEEE 754 floating point numbers into signed integers
// a positive float value will be transformed into a nbits integer number
// the upper nexp bits contain a reduced binary exponent
// the lower (nbits -nexp) bits contain a mantissa
// the "hidden one" and "denormalization" features of IEEE 754 are used
// example : nbits = 16, nexp = 4
//           the largest value  0 eeeeeeee mmmmmmmmmmmmxxxxxxxxxxx becomes
//                                    1111 mmmmmmmmmmmm
//           denormalized value 0 00000000 ddddddddddddxxxxxxxxxxx becomes
//                                    0000 dddddddddddd
// numbers 0.0 -> *fmaxa get converted to range
//         0.0 -> 2 ** (e[nexp] + 1 -127)
//         taking advantage of denormalized numbers to extend the dynamic range
int32_t ieee_quantize(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
                      int32_t *q,      // quantized data (OUTPUT)
                      int n,           // number of data elements (INPUT)
                      int nexp,        // number of bits for the exponent part of quantized data (INPUT)
                      int nbits,       // number of bits in quantized data (INPUT)
                      qhead *h)        // quantization setup information (INPUT+OUTPUT)
{
  int i, e0, nm ;
  FloatInt z0, t0 ;
  int32_t round, min, max ;
  int32_t limit, sbit ;
  float fmaxa ;    // largest absolute value in array

  e0 = -128 ;                                // invalid true exponent
  if(h == NULL) return e0 ;
  fmaxa = h->fmaxa ;
  if(nexp < 1 || nexp > 8) return e0 ;       // nexp too small or too large
  sbit = h->sbit ;
  nbits = nbits - sbit ;                     // need for a sign bit ? (if so reduce nbits)
  limit = ~((-1) << nbits) ;
  nm = nbits - nexp ;                        // number of effective mantissa bits
  if(nm < 1 || nm >23) return e0 ;           // too few or too many mantissa bits

  z0.f = fmaxa ;
  e0 = (z0.u >> 23) - 127 ;                  // true exponent of largest absolute value
  t0.u = e[nexp] << 23 ;                     // final scaling factor : (1 << nexp) - 1
  round = (1 << (23 + nexp - nbits -1)) ;    // rounding term

  min = INT_MAX ; max = INT_MIN ;
  for(i = 0 ; i < n ; i++) {
    q[i] = ieee_f_to_q(f[i], e0, round, t0.f, nm, limit, sbit) ;
    min = (q[i] < min) ? q[i] : min ;
    max = (q[i] > max) ? q[i] : max ;
  }
  if(h != NULL){
    h->e0 = e0 ;             // true exponent of largest absolute value float
    h->nbits = nbits ;       // number of bits per token
    h->nexp = nexp ;         // number of exponent bits
//     h->min = min ;
//     h->max = max ;
    h->max = ieee_f_to_q(h->fmax, e0, round, t0.f, nm, limit, sbit) ;  // largest quantized signed value
    h->min = ieee_f_to_q(h->fmin, e0, round, t0.f, nm, limit, sbit) ;  // lowest quantized signed value
    h->limit = limit ;       // keep limit mask
  }
  return e0 ;  // retourner e0/nbits/nexp/sbit/negative ?  8/8/8/4/4 bits ?
}

// vector version of above
int32_t ieee_quantize_v4(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
                      int32_t *q,      // quantized data (OUTPUT)
                      int n,           // number of data elements (INPUT)
                      int nexp,        // number of bits for the exponent part of quantized data (INPUT)
                      int nbits,       // number of bits in quantized data (INPUT)
                      qhead *h)        // quantization control information (INPUT+OUTPUT)
{
  int i, j, e0, nm ;
  FloatInt z0, t0 ;
  int32_t round ;
  int32_t limit, sbit ;
  int32_t vl ;
  float fmaxa ;    // largest absolute value in array

  e0 = -128 ;                                // invalid true exponent
  if(h == NULL) return e0 ;
  fmaxa = h->fmaxa ;
  if(nexp < 1 || nexp > 8) return e0 ;       // nexp too small or too large
  sbit = h->sbit ;
  nbits = nbits - sbit ;                     // need for a sign bit ?
  limit = ~((-1) << nbits) ;
  nm = nbits - nexp ;                        // number of effective mantissa bits
  if(nm < 1 || nm >23) return e0 ;           // too few or too many mantissa bits

  z0.f = fmaxa ;
  e0 = (z0.u >> 23) - 127 ;                  // true exponent of largest absolute value
  t0.u = e[nexp] << 23 ;                     // final scaling factor
  round = (1 << (23 + nexp - nbits -1)) ;

  vl = (n & 3) ; vl = (vl == 0) ? 4 : vl ;
  for(j = 0 ; j < n-3 ;){          // n is ASSUMED TO BE > 3
    for(i = 0 ; i < 4 ; i++){
      q[j+i] = ieee_f_to_q(f[j+i], e0, round, t0.f, nm, limit, sbit) ;
    }
    j += vl ;
    vl = 4 ;
  }
  if(h != NULL){
    h->e0 = e0 ;
    h->nbits = nbits ;
    h->nexp = nexp ;
    h->max = ieee_f_to_q(h->fmax, e0, round, t0.f, nm, limit, sbit) ;
    h->min = ieee_f_to_q(h->fmin, e0, round, t0.f, nm, limit, sbit) ;
    h->limit = limit ;       // keep limit mask
  }
  return e0 ;
}

// restore float values from quantized (ieee style) values
// utiliser un composite e0/nbits/nexp/sbit/negative  ( 8/8/8/4/4 bits ) plutot que h ?
int32_t ieee_unquantize(float *f,      // restored array (IEEE 754 32 bit float) (OUTPUT)
                        int32_t *q,    // quantized array (INPUT)
                        int n,         // number of data elements (INPUT)
                        qhead *h)      // quantization control information (INPUT)
{
  int i ;
  FloatInt t1, t2 ;
  int nm ;
  int nexp, e0, nbits ;
  int32_t sbit, neg ;

  if(h == NULL) return 0 ;
  nbits = h->nbits ;                         // number of bits in quantized data
  nexp = h->nexp ;                           // number of bits for the exponent part of quantized data
  if(nexp < 1 || nexp > 8) return 0 ;        // nexp too small or too large
  e0 = h->e0 ;                               // reference exponent (from ieee_quantize)
  if(e0 > 127 || e0 < -127 ) return 0 ;      // invalid reference exponent

  nm = nbits - nexp ;                        // number of effective mantissa bits
  if(nm < 1 || nm >23) return 0 ;

  sbit = h->sbit ;
  neg  = h->negative ;
  if(e0 > e[nexp]) {                         // must use 2 factors if e0 > e[nexp]
fprintf(stdout,"BEEP\n");
    t1.u = ((254 - e[nexp]) << 23) ;
    t2.u = ((127 + e0)      << 23) ;         // t1.f * t2.f would be too large ( > 2**128 )
    for(i = 0 ; i < n ; i++) {
      f[i] = ieee_q_to_f_2(q[i], t1.f, t2.f, nm, sbit, neg) ;
    }
  }else{                                     // can use 1 factor if e0 <= e[nexp]
fprintf(stdout,"BOP\n");
    t1.u = ((254 - e[nexp] + e0) << 23) ;
    for(i = 0 ; i < n ; i++) {
      f[i] = ieee_q_to_f_1(q[i], t1.f, nm, sbit, neg) ;
    }
  }
  return 1 ;
}

// IEEE 32 bit floating point to half precision (16 bit) IEEE floating point
// any number >= 65520 will be coded as infinity in FP16
STATIC inline uint16_t ieee_fp32_to_fp16(float f){
  FloatInt z, y ;
  uint32_t sign ;
  uint32_t round = 0x1000 ;
  uint32_t limit = ((127+16) << 23) | 0x7FFFFF ; // largest representable FP16
  z.f = f ;                     // float will be mostly manipulated as an integer
  sign = (z.u >> 16) & 0x8000 ; // position of FP16 sign bit
  z.u &= 0x7FFFFFFF ;           // suppress FP sign bit
  z.u += round ;                // apply mantissa rounding
  z.u = (z.u > limit) ? limit : z.u ;
  y.u = 15 << 23 ;              // scale by 2 ** (15 -127)
  y.f *= z.f ;
  y.u = (y.u >> 13) & 0xFFFF ;  // suppress lower 16 bits of mantissa & reduce exp + mantissa to 15 bits
  y.u |= sign ;                 // apply sign
  return y.u ;
}

// IEEE 32 bit floating point to half precision (16 bit) IEEE floating point
void fp32_to_fp16(float *f, uint16_t *q, int n){
  int i ;
  for(i = 0 ; i < n ; i++) q[i] = ieee_fp32_to_fp16(f[i]) ;
}

STATIC inline uint32_t ieee_fp32_to_fp24(float f){
  FloatInt z ;
  uint32_t mant, exp, sign ;
  uint32_t round = 1 << 7 ;
  z.f = f ;
  z.u += round ;
  sign = z.u >> 31 ;                      // sign bit
  exp = ((z.u >> 23) & 0xFF) -127 + 63 ;  // exponent now biased by 63 instead of 127
  if(exp <= 0) exp = 0 ;
  if(exp > 127) exp = 127 ;
  mant = (z.u >> 8) & 0xFFFFFF ;          // 16 bit mantissa
  return (sign << 23) | (exp << 16) | mant ;
}

// IEEE 32 bit floating point to 3/4 precision (24 bit) IEEE style floating point
// 24 bit format : 1/7/16  sign/exp/mantissa
// 3 entries in q for each group of 4 f values
void fp32_to_fp24(float *f, uint32_t *q, int n){
  int i0, i ;
  uint32_t t[4] ;
  for(i0=0 ; i0<n-3 ; i0+=4){
    for(i=0 ; i<4 ; i++){
      t[i] = ieee_fp32_to_fp24(f[i0+i]) ;
    }
    // pack tokens big endian style
    q[0] = (t[0] <<  8) | (t[1] >> 16) ;  // t0 , upper 8 bits of t1
    q[1] = (t[1] << 16) | (t[2] >>  8) ;  // lower 16 bits of t1, upper 16 bits fo t2
    q[2] = (t[2] << 24) | (t[3]) ;        // lower 8 bits of t2, t3
    q += 3 ;
  }
  for(i=0 ; i<4 ; i++) t[i] = 0 ;
  for(i=0 ; i0<n ; i0++, i++){
    t[i] = ieee_fp32_to_fp24(f[i0]) ;
  }
  if(i>0) q[0] = (t[0] <<  8) | (t[1] >> 16) ;  // t0 , upper 8 bits of t1
  if(i>1) q[1] = (t[1] << 16) | (t[2] >>  8) ;  // lower 16 bits of t1, upper 16 bits fo t2
  if(i>2) q[2] = (t[2] << 24) | (t[3]) ;        // lower 8 bits of t2, t3
//   for(i = 0 ; i < n ; i++) q[i] = ieee_fp32_to_fp16(f[i]) ;
}

// scaled IEEE 32 bit floating point to half precision (16 bit) IEEE floating point
void fp32_to_fp16_scaled(float *f, uint16_t *q, int n, float scale){
  int i ;
  for(i = 0 ; i < n ; i++) q[i] = ieee_fp32_to_fp16(f[i]*scale) ;
}

// scaled IEEE 32 bit floating point to 3/4 precision (24 bit) IEEE style floating point
// 4 values in f for each group of 4 q values
void fp32_to_fp24_scaled(float *f, uint32_t *q, int n, float scale){
  int i ;
  for(i = 0 ; i < n ; i++) q[i] = ieee_fp32_to_fp24(f[i]*scale) ;
}

// IEEE 32 bit floating point to brain float (16 bit)
// current code will not behave correctly if upper 16 bits after sign bit in float are 1 (NaN)
// removing comments makes code safe
void fp32_to_bf16(float *f, uint16_t *q, int n){
  FloatInt z ;
  uint32_t round = 1 << 15 ;
//   uint32_t sign ;
  int i ;
  for(i = 0 ; i < n ; i++) {
    z.f = f[i] ;
//     sign = z.u & (~0x7FFFFFFF) ;
//     z.u &= 0x7FFFFFFF ;
    z.u += round ;
//     z.u &= 0x7FFFFFFF ;
//     z.u |= sign ;
    q[i] = z.u >> 16 ;
  }
}

// half precision (16 bit) IEEE floating point to IEEE 32 bit floating point
// the infinity argment value is used as a result if the FP16 exponent is 31
// (the sign of the FP16 value will be preserved)
STATIC inline float ieee_fp16_to_fp32(uint16_t q, uint32_t infinity){
  FloatInt z, y ;
  int sign ;
  int e0;
  sign = q & 0x8000 ;    // position of FP16 sign bit
  sign <<= 16 ;          // position of FP32 sign bit
  z.u = q & 0x7FFF ;     // suppress FP16 sign bit
  e0 = z.u >> 10 ;       // FP16 exponent (with bias 15)
  z.u <<= 13 ;
  y.u = (254 - 15) << 23 ;       // scale by 2 ** (127 - 15)
  z.f *= y.f ;
  z.u = (e0 == 31) ? infinity : z.u ;  // infinity if biased FP16 exponent == 31
  z.u |= sign ;
  return z.f ;
}

// half precision (16 bit) IEEE floating point to IEEE 32 bit floating point
// if inf is not NULL, *inf is used instead of IEEE Infinity (sign of FP value is preserved)
// 0X7F800000 is infinity, 0X7F800001 -> 0X7FFFFFFF is a NaN (not a number)
// f  : 32 bit IEEE floating point array
// q  : 16 bit IEEE floating point array
void fp16_to_fp32(float *f, void *f16, int n, void *inf){
  int i ;
  uint32_t Inf = 0X7F800000 ; //  IEEE Infinity
  uint16_t *q = f16 ;

  Inf = inf ? *(uint32_t *)inf : Inf ;
  for(i = 0 ; i < n ; i++) f[i] = ieee_fp16_to_fp32(q[i], Inf) ;
}

void fp24_to_fp32(float *f, void *f24, int n, void *inf){
  int i0, i ;
//   int mant, exp, sign ;
//   uint32_t *q = f24 ;
//   FloatInt z[4] ;
//   uint32_t Inf = 0X7F800000 ; //  IEEE Infinity

  for(i0=0 ; i0<n-3 ; i0+=4){
    for(i=0 ; i<4 ; i++){
    }
  }
}


#if defined(__clang__) || defined(__ICC) || defined(__PGIC__) || defined(VANILLA)
#else
// NOTE: strange method to defeat some overzealous optimizers at O2 and above
// static uint32_t Fetch_32(uint32_t *what) {  // return dereferenced what or IEEE Infinity if what is NULL
//   return what ? *what : 0X7F800000 ; //  IEEE Infinity
// }
// uint32_t (*FudgedWordFetch)(uint32_t *what) = &Fetch_32 ;
// NOTE: strange method to defeat some overzealous optimizers at O2 and above
static float Reciprocal(float what){  // return reciprocal of a float
  return 1.0f / what ;
}
float (*FudgeFloatReciprocal)(float what) = &Reciprocal ;
#endif

// scaled half precision (16 bit) IEEE floating point to IEEE 32 bit floating point
// if inf is not NULL, *inf is used instead of IEEE Infinity (sign of FP value is preserved)
// 0X7F800000 is infinity, 0X7F800001 -> 0X7FFFFFFF is a NaN (not a number)
void fp16_to_fp32_scaled(float *f, void *f16, int n, void *inf, float scale){
  int i ;
  uint32_t Inf ;
  uint16_t *q = f16 ;
  float rscale ;

#if defined(__clang__) || defined(__ICC) || defined(__PGIC__) || defined(VANILLA)
  rscale = 1.0f / scale ;
  Inf = inf ? *((uint32_t *)inf) : 0X7F800000 ;    // IEEE infinity
#else
  rscale = (*FudgeFloatReciprocal)(scale) ;  // NOTE: defeat some overzealous at O2 and above
//   Inf = (*FudgedWordFetch)(inf) ;         // NOTE: defeat some overzealous at O2 and above
  Inf = inf ? *((uint32_t *)inf) : 0X7F800000 ;    // IEEE infinity
#endif
  for(i = 0 ; i < n ; i++) f[i] = rscale * ieee_fp16_to_fp32(q[i], Inf) ;
}

// brain float (16 bit) to IEEE 32 bit floating point
void bf16_to_fp32(float *f, uint16_t *q, int n){
  FloatInt z ;
  int i ;
  for(i = 0 ; i < n ; i++){
    z.u = q[i] ;
    z.u <<= 16 ;   // shift to upper 16 bits
    q[i] = z.f ;
  }
}

