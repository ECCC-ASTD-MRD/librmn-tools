// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2021  Recherche en Prevision Numerique
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
#include <stdint.h>
#include <limits.h>
// #include <float.h>

#include <rmn/misc_operators.h>
#include <rmn/tools_types.h>
#include <rmn/data_info.h>
#include <rmn/ieee_quantize.h>
#include <rmn/ct_assert.h>

#if defined(__GNUC__)
#if ! defined(__INTEL_COMPILER_UPDATE)
#if ! defined(__PGI)
#pragma GCC optimize "O3,tree-vectorize"
#endif
#endif
#endif

#if ! defined(STATIC)
#define STATIC static
#endif

// first analysis pass, find extrema and some sign properties
typedef struct{
  uint32_t allp:1 ,  // all numbers are >= 0
           maxa:31;  // maximum absolute value (never more than 31 bits)
  uint32_t allm:1 ,  // all numbers are <= 0
           mina:31;  // minimum absolute value (never more than 31 bits)
} ieee32_l ;

typedef struct{
  int32_t  maxs ;    // maximum value
  int32_t  mins ;    // minimum value
} ieee32_s ;

// some (useful for quantization) properties of an IEEE float array, 64 bits total
// types ieee32_props and ieee32_p are for internal use, uint64_t is exposed in the interface
//
// type 0 linear quantization header
typedef struct{
  uint32_t bias:32;  // minimum absolute value (actually never more than 31 bits)
  uint32_t npts:16,  // number of points, 0 means unknown
           resv:3 ,
           shft:5 ,  // shift count (0 -> 31) for quanze/unquantize
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32_p ;

// type 1 linear quantization header
typedef struct {
  uint32_t bias:32;  // integer offset reflecting minimum value of packed field
  uint32_t npts:16,  // number of points, 0 means unknown
           efac:8 ,  // exponent for quantization mutliplier
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32_q ;

// type 2 linear quantization header
typedef struct {
  int32_t  bias:32;  // signed integer offset reflecting minimum value of packed field
  uint32_t npts:14,  // number of points, 0 means unknown
           expm:8 ,  // largest IEEE exponent (biased)
           shft:5 ,  // shift count (0 -> 31) for normalization
           nbts:5 ;  // number of bits (0 -> 31) per value
} ieee32_r ;

// pseudo log quantization header
typedef struct {
  uint32_t qmin:32;
  uint32_t emax:8 ,  // exponent of largest absolute value
           emin:8 ,  // exponent of smallest absolute value
           elow:8 ,  // exponent of smallest significant absolute value
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           clip:1 ,  // original emin < elow, clipping may occur
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32_e ;

// access to all properties structs through a single union
typedef union{
  ieee32_p  p  ;     // type 0 linear header
  ieee32_q  q  ;     // type 1 linear header
  ieee32_r  r  ;     // type 2 linear header
  ieee32_l  l  ;     // absolute value analysis
  ieee32_s  s  ;     // signed value analysis
  ieee32_e  e  ;     // pseudo log header
  qhead    *h  ;     // pointer to full quantization header (used by type 3)
  uint64_t  u  ;     // access as a single 64 bit unsigned integer
  uint32_t  w[2]  ;  // access as 2 32 bit unsigned integers
} ieee32_props ;
// make sure that access as a single 64 bit unsigned integer is respected
CT_ASSERT(sizeof(ieee32_props) == sizeof(uint64_t))

// quantize IEEE 32 bit floats
// qs     [OUT]  32 bit integer array that will receive the quantized result
// f       [IN]  32 bit IEEE float array, data to be quantized
// meta [INOUT]  pointer to q_meta structure (quantization metadata)
// nd      [IN]  number of data items to quantize
int64_t IEEE_quantize(void * restrict f, void * restrict q, q_meta *meta,  int nd, int nbits, float error, int mode, void *spval, uint32_t mmask, void *pad)
{
  limits_w32 l32 ;
  ieee32_props h64 ;
  int submode ;
  uint32_t *spval_ = (uint32_t *)spval, *pad_ = (uint32_t *)pad ;

  l32 = IEEE32_extrema_missing(f, nd, spval, mmask, pad) ;   // step 1 : analyze data
  meta->spval.u = spval_ ? *spval_ : 0 ;
  meta->mmask.u = mmask ;
  meta->pad.u   = pad_ ? *pad_ : 0 ;

  if(mode == 0){
    mode = Q_MODE_LINEAR ;                                   // default linear quantizer
  }
  submode = mode & Q_SUBMODE_MASK ;
  if(submode == 0){                                          // automatic submode
    submode = 1 ;
    mode += submode ;
  }
  // step 2 : use requested quantization mode
  if(mode & Q_MODE_LINEAR){
    h64.l.allp = l32.u.allp ;
    h64.l.allm = l32.u.allm ;
    h64.l.maxa = l32.u.maxa ;
    h64.l.mina = l32.u.mina ;
    switch(submode)
    {
      case 1:
        h64.u = IEEE32_linear_prep_0(h64.u, nd, nbits, error) ;  // get quantization parameters (type 0)
        h64.u = IEEE32_quantize_linear_0(f, h64.u, q) ;          // actual quantization (type 0)
        break ;
      case 2:
        break ;
      case 3:
        break ;
      default:
        return 1 ;
    }
  }else if(mode & Q_MODE_LOG){
    switch(submode)
    {
      case 1:
        break ;
      case 2:
        break ;
      default:
        return 1 ;
    }
  }else{
    return 1 ;
  }
  return 0 ;
}

int64_t IEEE_qrestore(void * restrict f, void * restrict q, q_meta *meta,  int nd)
{
  int mode = meta->mode ;
  return 0 ;
}

// ========================================== common functions ==========================================
// prepare for quantization of IEEE floats

// get minimum and maximum absolute values, set all < 0 and none < 0 flags
// f     [IN]  32 bit IEEE float array, data to be quantized
// np    [IN]  number of data items to quantize
uint64_t IEEE32_limits(void * restrict f, int np){
  int32_t *fu = (int32_t *) f ;
  ieee32_props h64 ;
  uint32_t maxa[8], mina[8], ands[8], ors[8], t[8], masksign ;
  int i0, i, ni7 ;
  uint32_t allm, allp ;

  masksign = RMASK31(31) ;       // sign bit is 0, all others are 1
  h64.u = 0 ;
  for(i=0 ; i<8 ; i++){          // prepare mina, maxa, ors, ands for accumulation
    maxa[i] = ors[i]  = 0u ;     // 0
    mina[i] = ands[i] = ~0u ;    // FFFFFFFF
  }
  ni7 = (np & 7) ;
  for(i=0 ; i<8 && i<ni7 ; i++){
//     if(i >= ni7) break ;                  // safety for case where we have less than 8 values
    t[i] = fu[i] & masksign ;             // get rid of sign
    maxa[i]  = MAX( maxa[i], t[i]) ;      // largest absolute value
    mina[i]  = MIN( mina[i], t[i]) ;      // smallest absolute value
    ands[i] &= fu[i] ;                    // all < 0 detection
    ors[i]  |= fu[i] ;                    // all >= 0 detection
  }
  for(i0=ni7 ; i0<np-7 ; i0+=8){          // chunks of 8 values
    for(i=0 ; i<8 ; i++){
      t[i] = fu[i0+i] & masksign ;        // get rid of sign
      maxa[i]  = MAX( maxa[i], t[i]) ;    // largest absolute value
      mina[i]  = MIN( mina[i], t[i]) ;    // smallest absolute value
      ands[i] &= fu[i0+i] ;               // all < 0 detection
      ors[i]  |= fu[i0+i] ;               // all >= 0 detection
    }
  }
  for(i=0 ; i<8 ; i++){        // fold 8 long vectors into single values
    maxa[0]  = MAX( maxa[0], maxa[i]) ;
    mina[0]  = MIN( mina[0], mina[i]) ;
    ands[0] &= ands[i] ;       // will be 1 if all values are < 0, will be 0 if any value is >= 0
    ors[0]  |= ors[i] ;        // will be 0 if all values >= 0, will be 1 if any value is <0
  }
  allm = (ands[0] >> 31) == 1 ;           // all values are negative
  allp = (ors[0] >> 31) == 0 ;            // we have NO negative values
  h64.l.allp = allp ;
  h64.l.allm = allm ;
  h64.l.maxa = maxa[0] ;
  h64.l.mina = mina[0] ;
  return h64.u ;             // return 64 bit aggregate
}

// ========================================== pseudo log quantizer 0 ==========================================

void IEEE32_exp_limits(uint64_t u64, int *emin, int *emax){
  ieee32_props h64 ;
  h64.u = u64 ;
  *emin = (h64.l.mina >> 23) - 127 ;
  *emax = (h64.l.maxa >> 23) - 127 ;
}

// restore floating point numbers quantized with IEEE32_fakelog_quantize_0
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// u64   [IN]  metadata information describing quantization (from IEEE32_fakelog_quantize_0)
// N.B. if values have mixed signs, the sign is stored as the LSB in the quantized integer
int IEEE32_fakelog_unquantize_0(void * restrict f, uint64_t u64, int ni, void * restrict q){
  uint32_t *qi = (uint32_t *) q ;
  float *fo = (float *) f ;
  ieee32_props h64 ;
  int i, nbits ;
  int32_t emax, emin, elow, exp ;
  uint32_t allp, allm, pos_neg, ti, sign ;
  AnyType32 t, x ;
  float fac, qrange, q0 ;

  h64.u = u64 ;
  emax = h64.e.emax - 127 ;
  emin = h64.e.emin - 127 ;
  elow = h64.e.elow ;
  nbits = h64.e.nbts ;
  allp = h64.e.allp ;
  allm = h64.e.allm ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values
  sign = (allm << 31) ;                   // move sign of all values to MSB position
// fprintf(stderr, "allm = %d, allp = %d, pos_neg = %d\n", allm, allp, pos_neg) ;

  x.u = ((emin + 127) << 23) ;
  x.u |= sign ;                           // restore sign (if all numbers are negative)
  q0 = (elow == 0) ? 0.0f : x.f ;

  qrange = (1 << nbits) ;         // range of quantized values (2**nbits)
  t.f = (emax - emin + 1) ;       // maximum possible value of "fake log"
  fac = t.f / qrange ;      // factor to quantize "fake log"
// fprintf(stderr,"nbits = %d, emax-emin+1 = %d, fac = %f, emin = %d, emax = %d, elow = %d, q0 = %f\n", nbits, emax-emin+1, fac, emin, emax, elow, q0) ;
  if(pos_neg){                 // mix of positive and negative numbers
    for(i=0 ; i<ni ; i++){
      ti  = qi[i] ;
      sign = ti << 31 ;        // sign from LSB -> MSB
      ti >>= 1 ;               // remove sign
      t.f = fac * ti ;
      t.f += 1.0f ;
      exp = t.f ;              // exponent
      t.f = t.f - exp + 1.0f ;
      exp += 126 ;
      exp += emin ;
      exp <<= 23 ;
      x.u = t.u & 0x7FFFFF ;   // restore mantissa
      x.u |= exp ;             // restore exponent
      x.u |= sign ;            // restore sign
      fo[i] = (ti == 0) ? q0 : x.f ;
    }
  }else{                       // all numbers have the ame sign
    for(i=0 ; i<ni ; i++){
      t.f = fac * qi[i] ;
      t.f += 1.0f ;
      exp = t.f ;              // exponent
      t.f = t.f - exp + 1.0f ;
      exp += 126 ;
      exp += emin ;
      exp <<= 23 ;
      x.u = t.u & 0x7FFFFF ;   // restore mantissa
      x.u |= exp ;             // restore exponent
      x.u |= sign ;            // restore sign
      fo[i] = (qi[i] == 0) ? q0 : x.f ;
    }
  }
  return 0 ;
}

// quantize IEEE floats
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// ni    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data
// qzero [IN]  smallest value considered significant (if qzero < 0, it will be restored as 0.0)
// N.B. qzero will be reduced to a power of 2 <= qzero
//      if there are mixed signs, the sign is stored in the LSB position
uint64_t IEEE32_fakelog_quantize_0(void * restrict f, int ni, int nbits, float qzero, void * restrict qs){
  float *fu = (float *) f ;
  uint32_t *qo = (uint32_t *) qs ;
  ieee32_props h64 ;
  uint32_t allp, allm, pos_neg ;
//   uint32_t clip ;
  uint32_t maxa, mina, sign, maskbits ;
  int32_t emax, emin, elow ;
  AnyType32 t ;
  float fac, qrange ;   // quantization factor
  int i ;

  h64.u = IEEE32_limits(fu, ni) ;         // get min and max absolute values
  allm = h64.l.allm ;                     // extract all useful values from u64
  allp = h64.l.allp ;
  maxa = h64.l.maxa ;
  mina = h64.l.mina ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values
  nbits -= pos_neg ;                      // one bit is needed for sign ==> one less available for quantized value
// fprintf(stderr, "allm = %d, allp = %d, pos_neg = %d\n", allm, allp, pos_neg) ;
  h64.u = 0 ;                             // initialize metadata
  h64.e.nbts = nbits ;
  h64.e.allp = allp ;
  h64.e.allm = allm ;
  h64.e.clip = 0 ;
  h64.e.qmin = 0 ;

  t.f  = qzero ;                          // lowest significant value
  elow = (t.u >> 23) & 0xFF ;             // will be 0 when qzero == 0.0
  h64.e.elow = elow ; elow -= 127 ;
  emax = (maxa >> 23) & 0xFF ;
  h64.e.emax = emax ; emax -= 127 ;
  emin = (mina >> 23) & 0xFF ;
  emin -= 127 ;                           // original emin may be < elow
//   clip = (emin < elow) ? 1 : 0 ;          // original emin < elow
  if((qzero < 0) && (emin < elow)) h64.e.elow = 0 ;  // will force a restore to zero where quantized value == 0
  emin = (emin < elow) ? elow : emin ;    // emin < elow, set emin to elow for quantization
//   emin = (emin < elow) ? elow - 1 : emin ;     // emin < elow, set emin to elow - 1 for quantization
  h64.e.emin = emin + 127 ;                    // store adjusted value into h64.e.emin

  qrange = (1 << nbits) ;         // range of quantized values (2**nbits)
  t.f = (emax - emin + 1) ;       // maximum possible value of "fake log"
  fac = t.f = qrange / t.f ;      // factor to quantize "fake log"
  maskbits = RMASK31(nbits) ;     // 2**nbits -1, largest allowable quantized value

  if(pos_neg){
    for(i=0 ; i<ni ; i++){
      int exp ;
      AnyType32 x ;
      x.f = fu[i] ;                 // float to quantize
      sign = x.ie32.s ;             // save sign
      exp = (x.u >> 23) & 0xFF ;    // exponent from value (with +127 bias)
      exp = exp - emin ;            // this will have a value of 127 for the lowest "significant" exponent
      x.u &= 0x7FFFFF ;             // get rid of exponent and sign, only keep mantissa
      x.u |= (0x7F << 23) ;         // force 127 as exponent (1.0 <= x.f < 2.0)
      x.f += (exp - 127 - 1) ;      // add value of exponent from original float (0.0 <= x.f < max "fake log")
      // if(i == 0 || i == ni-1) fprintf(stderr,"%14.8f\n", x.f) ;
      qo[i] = x.f * fac + .5f ;     // quantize the fake log
      qo[i] = (x.f <  0.0f) ? 0 : qo[i] ;               // < qzero (exp < emin)
      qo[i] = (x.f == 0.0f) ? 1 : qo[i] ;               // == lowest significant value (emin or elow)
      qo[i] = (qo[i] > maskbits) ? maskbits : qo[i] ;   // clamp at 2**nbits -1
      qo[i] = (qo[i] << 1) | sign ; // sign as LSB
    }
  }else{                            // all values have the same sign
    for(i=0 ; i<ni ; i++){
      int exp ;
      AnyType32 x ;
      x.f = fu[i] ;                 // float to quantize
      exp = (x.u >> 23) & 0xFF ;    // exponent from value (with +127 bias)
      exp = exp - emin ;            // this will have a value of 127 for the lowest "significant" exponent
      x.u &= 0x7FFFFF ;             // get rid of exponent and sign, only keep mantissa
      x.u |= (0x7F << 23) ;         // force 127 as exponent (1.0 <= x.f < 2.0)
      x.f += (exp - 127 - 1) ;      // add value of exponent from original float (0.0 <= x.f < max "fake log")
      // if(i == 0 || i == ni-1) fprintf(stderr,"%14.8f\n", x.f) ;
      qo[i] = x.f * fac + .5f ;     // quantize the fake log
      qo[i] = (x.f <  0.0f) ? 0 : qo[i] ;               // < qzero (exp < emin)
      qo[i] = (x.f == 0.0f) ? 1 : qo[i] ;               // == lowest significant value (emin or elow)
      qo[i] = (qo[i] > maskbits) ? maskbits : qo[i] ;   // clamp at 2**nbits -1
    }
  }
  return h64.u ;
}

// ========================================== linear quantizer 0 ==========================================

// restore floating point numbers quantized with IEEE32_linear_quantize_0
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// u64   [IN]  metadata information describing quantization (from IEEE32_linear_quantize_0)
// N.B. if values have mixed signs, the sign is stored as the LSB in the quantized integer
int IEEE32_linear_unquantize_0(void * restrict q, uint64_t u64, int ni, void * restrict f){
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
  scount = h64.p.shft ;                                 // shift count
  offset = h64.p.bias >> scount ;                       // bias before shifting
  pos_neg = (h64.p.allp || h64.p.allm) ? 0 : 1 ;        // mixed signs
// if(h64.p.cnst) fprintf(stderr, "DEBUG, constant field, pos_neg = %d\n", pos_neg) ;
  ni7 = (ni & 7) ;
  if(q == f) {        // restore IN PLACE
    if(h64.p.nbts == 0){   // constant value, same sign
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
        qi[i] |= sign ;                                  // apply sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){                     // 8 items chnumks
        for(i=0 ; i<8 ; i++){
          temp = qi[i0+i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          qi[i0+i] = (temp + offset) << scount ;         // unquantize
          qi[i0+i] |= sign ;                             // apply sign
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
    if(h64.p.nbts == 0){   // constant value, same sign
      sign = h64.p.allm ? (1u << 31) : 0 ;
      for(i=0 ; i<ni ; i++) fo[i] = h64.p.bias | sign ;
      goto end ;
    }
    if(pos_neg){       // mix of positive and negative values
// fprintf(stderr,"pos neg restore\n");
      for(i=0 ; i<(ni & 7) ; i++){                       // first chunk (0 - > 7 items)
          temp = qi[i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          fo[i] = (temp + offset) << scount ;            // unquantize
          fo[i] |= sign ;                                // apply sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          temp = qi[i0+i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          fo[i0+i] = (temp + offset) << scount ;         // unquantize
          fo[i0+i] |= sign ;                             // apply sign
        }
      }
    }else{
      sign = h64.p.allm << 31 ;
      for(i=0 ; i<(ni & 7) ; i++){                       // first chunk (0 - > 7 items)
        fo[i] = (qi[i] + offset) << scount ;             // unquantize
        fo[i] |= sign ;
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          fo[i0+i] = (qi[i0+i] + offset) << scount ;         // unquantize
          fo[i0+i] |= sign ;
        }
      }
    }
  }
end:
  return 0 ;
}

// evaluate the discretization quantum as a function of nbits (type 0)
FloatPair IEEE32_linear_quantum_0(uint64_t u64, int nbits){
//   ieee32_props h64 ;
//   uint64_t allp, allm, pos_neg ;
//   AnyType32 maxa, mina ;
  FloatPair result = {0.0f, 0.0f } ;

//   h64.u = u64 ;
//   allm = h64.l.allm ;            // extract all useful values from u64
//   allp = h64.l.allp ;
//   maxa.u = h64.l.maxa ;
//   mina.u = h64.l.mina ;
//   pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values

  return result ;
}

// prepare for linear quantization of IEEE floats (type 0)
// normally for cases where all values have the same sign
// and largest/smallest absolute value ratio is relatively small
// u64   [IN]  analysis from IEEE32_limits
// np    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data (0 means use quantum)
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
uint64_t IEEE32_linear_prep_0(uint64_t u64, int np, int nbits, float quant){
  ieee32_props h64 ;
  uint32_t maxa, mina, rangeu, lz ;
  uint32_t pos_neg, allm, allp, cnst ;
  int32_t scount, nbitsmax ;
  AnyType32 fi1, fi2 ;
  float delta ;
  // quant and nbits both automatic makes NO SENSE, set nbits to 15
  if(quant <= 0 && nbits <= 0) nbits = 15 ;

  h64.u = u64 ;
  allm = h64.l.allm ;            // extract all useful values from u64
  allp = h64.l.allp ;
  maxa = h64.l.maxa ;
  mina = h64.l.mina ;

  h64.u = 0 ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values
  rangeu = maxa - mina ;                  // value range (considering ABS(float) as an unsigned integer)
  lz = lzcnt_32(rangeu) ;                 // number of leading zeroes in range
  // 32 - number of most significant 0 bits in rangeu = max possible number of useful bits
  nbitsmax = 32 -lz ;
  cnst = 0 ;                              // a priori not a constant field
  if(rangeu == 0) {                       // special case : constant absolute values
    scount = 0 ;
    nbits = pos_neg ;                     // no bits needed if all values have the same sign
    cnst = 1 ;                            // constant field flag
    goto end ;
  }
  if( (nbits <= 0) ) {                    // automatic determination of nbits, quantum > 0
    int32_t temp ;
    fi1.u = maxa ; fi2.u = mina ; 
    fi1.f = fi1.f - fi2.f ;               // fi1.f = range of data values
    fi2.f = quant ;
    temp = (fi1.u >> 23) - (fi2.u >> 23) ;  // IEEE exponent difference between range and quantum
    nbits = temp + 1 + pos_neg ;
//       fprintf(stderr," provisional nbits(%d), nbitsmax(%d), ratio(%f)", nbits, nbitsmax, fi1.f/fi2.f) ;
  }
  if(pos_neg) {                           // sign bit needs one bit, reduce allowed bit count by 1, increase nbitsmax
    nbitsmax++ ;
    nbits-- ;
  }
  if(nbits > nbitsmax) nbits = nbitsmax ; // no point in exceeding nbitsmax
  scount = 32 - lz - nbits ; 
  scount = (scount < 0) ? 0 : scount ;    // scount CANNOT be < 0
  fi1.u = (maxa >> scount) << scount ;    // drop lower scount bits
  fi2.u = fi1.u - (1 << scount) ;
  delta = fi1.f - fi2.f ;                 // difference between values whose quantization differs by 1 unit
  // if quantum < delta, nbits may need to be increased (except if quantum <= 0, when nbits must be used)
  if( (quant < delta) && (quant > 0.0f) ) {
    while( (quant < delta) && (nbits < nbitsmax) ){
      nbits++ ;
      scount = 32 - lz - nbits ;
      scount = (scount < 0) ? 0 : scount ;
      fi1.u = (maxa >> scount) << scount ;
      fi2.u = fi1.u - (1 << scount) ;
      delta = fi1.f - fi2.f ;             // difference between values whose quantization differs by 1 unit
    }
  }

end:                         // update returned struct
  h64.p.shft = scount ;
  h64.p.nbts = nbits ;
  h64.p.npts = np ;
  h64.p.allp = allp ;
  h64.p.allm = allm ;
  h64.p.cnst = cnst ;
  h64.p.bias = mina ;
//   fi1.u = mina  ;
//   fi2.u = maxa  ;
// fprintf(stderr, " npts = %d, nbits = %d, shft = %d, min = %f, max = %f, allp/allm = %d/%d\n", np, nbits, scount, fi1.f, fi2.f, allp, allm) ;
  return h64.u ;             // return 64 bit aggregate
}

// quantize IEEE floats using information from IEEE32_linear_prep_0
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// u64   [IN]  metadata information describing quantization (from IEEE32_linear_prep_0)
// N.B. if values have mixed signs, the sign is stored as the LSB in the quantized integer
uint64_t IEEE32_quantize_linear_0(void * restrict f, uint64_t u64, void * restrict qs){
  uint32_t *fu = (uint32_t *) f ;
  uint32_t *qo = (uint32_t *) qs ;
  ieee32_props h64 ;
  int i0, i, ni7 ;
  uint32_t pos_neg, maskn, masksign, offset, sign, round ;
  int32_t scount, nbits, ni ;

  h64.u   = u64 ;                           // all properties
  nbits   = h64.p.nbts ;                    // number of bits needed for quantized results
  ni      = h64.p.npts ;                    // number of values
  scount  = h64.p.shft ;                    // shift count
  offset  = h64.p.bias >> scount ;          // offset from smallest absolute value
  round   = scount ? 1 << (scount-1) : 0 ;  // rounding term
  pos_neg = (h64.p.allp || h64.p.allm) ? 0 : 1 ;   // not same sign for all values
  maskn = RMASK31(nbits) ;                  // largest allowed value for quantized results
  masksign = RMASK31(31) ;                  // sign bit is 0, all others are 1

  if(h64.p.cnst){
    if(pos_neg){
      for(i=0 ; i<ni ; i++) qo[i] = fu[i] >> 31 ;
    }
    goto end ;
  }
  ni7 = (ni & 7) ;
  if(f == qs){      // quantize IN PLACE
    if(pos_neg){    // both negative and non negative floats will be quantized
      for(i=0 ; i<(ni & 7) ; i++){                                    // first chunk
        sign = fu[i] >> 31 ;                                          // save sign
        fu[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
        fu[i] = MIN(fu[i],maskn) ;                                    // clip if needed
        fu[i] = (fu[i] << 1) | sign ;                                 // store sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){                                                 // all other chunks are 8 long
          sign = (fu[i0+i] >> 31) ;                                           // save sign
          fu[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          fu[i0+i] = MIN(fu[i0+i],maskn) ;                                    // clip if needed
          fu[i0+i] = (fu[i0+i] << 1) | sign ;                                 // store sign
        }
      }
    }else{          // all floats to be quantized have the same sign
      for(i=0 ; i<(ni & 7) ; i++){
        fu[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
        fu[i] = MIN(fu[i],maskn) ;                                    // clip if needed
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          fu[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          fu[i0+i] = MIN(fu[i0+i],maskn) ;                                    // clip if needed
        }
      }
    }
  }else{            // quantize NOT IN PLACE
    if(pos_neg){    // both negative and non negative floats will be quantized
      for(i=0 ; i<(ni & 7) ; i++){
        sign = fu[i] >> 31 ;                                          // save sign
        qo[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
        qo[i] = MIN(qo[i],maskn) ;                                    // clip if needed
        qo[i] = (qo[i] << 1) | sign ;                                 // store sign
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          qo[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          qo[i0+i] = MIN(qo[i0+i],maskn) ;                                    // clip if needed
          qo[i0+i] = (qo[i0+i] << 1) | (fu[i0+i] >> 31) ;                     // store sign
        }
      }
    }else{          // all floats to be quantized have the same sign
      for(i=0 ; i<(ni & 7) ; i++){
        qo[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;          // quantize
        qo[i] = MIN(qo[i],maskn) ;                                            // clip if needed
      }
      for(i0=ni7 ; i0<ni-7 ; i0+=8){
        for(i=0 ; i<8 ; i++){
          qo[i0+i] = (( (masksign & fu[i0+i]) + round) >> scount) - offset ;  // quantize
          qo[i0+i] = MIN(qo[i0+i],maskn) ;                                    // clip if needed
        }
      }
    }
  }
end:
  return u64 ;
}

// quantize IEEE floats
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// ni    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
uint64_t IEEE32_linear_quantize_0(void * restrict f, int ni, int nbits, float quantum, void * restrict qs){
  ieee32_props h64 ;

  h64.u = IEEE32_limits(f, ni) ;                            // get min and max absolute values
  h64.u = IEEE32_linear_prep_0(h64.u, ni, nbits, quantum) ; // get quantization parameters (type 0)
// fprintf(stderr,"npts = %d, shft = %d, nbts = %d, cnst = %d, allp = %d, allm = %d\n", 
//         h64.p.npts, h64.p.shft, h64.p.nbts, h64.p.cnst, h64.p.allp, h64.p.allm) ;
  h64.u = IEEE32_quantize_linear_0(f, h64.u, qs) ;                  // actual quantization (type 0)
  return h64.u ;
}

// ========================================== linear quantizer 1 ==========================================

// restore floating point numbers quantized with IEEE32_linear_quantize_1
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// u64   [IN]  metadata information describing quantization (from IEEE32_linear_quantize_1)
// N.B. if values have mixed signs, the quantized integers will have mixed signs
//      if all values have the same sign, the quantized integers will all be >= 0
int IEEE32_linear_unquantize_1(void * restrict q, uint64_t u64, int ni, void * restrict f){
  int32_t *qi = (int32_t *) q ;
  float *fo = (float *) f ;
  ieee32_props h64 ;
  int ni7, i0, i ;
  int exp ;
  AnyType32 m1, bias ;
  float fac32, tf, offset ;
  uint32_t allp, allm, pos_neg, sg ;

  h64.u  = u64 ;
  allp  = h64.q.allp ;               // no neggative values
  allm  = h64.q.allm ;               // all negative values
  pos_neg = (allp || allm) ? 0 : 1 ;
  bias.u = h64.q.bias ;
  offset = bias.f ;
  exp = h64.q.efac ; exp -= 127 ;        // get exponent without bias
// fprintf(stderr, "ni = %d, nbits = %d, exp = %d(%d), bias = %f\n", ni, h64.q.nbts, exp, h64.q.efac, bias.f);
  if(exp > -127 && exp < 127){           // "civilized" exponent for largest value
    m1.i = (127 - exp) ;                 // inverse of factor used for quantization
    m1.i <<= 23 ;
    fac32 = m1.f;
  }else{                                 // not a "civilized" exponent
    return 1 ;
  }
// fprintf(stderr, "fac32 = %f, offset = %f, pos_neg = %d\n", fac32, offset, pos_neg);
  ni7 = (ni & 7) ;
  if(pos_neg){
    for(i=0 ; i<(ni & 7) ; i++){
      tf = qi[i] ;
      sg = (qi[i] < 0) ;              // save sign
      tf = sg ? -tf : tf ;            // absolute value
      tf = tf * fac32 + offset ;      // unquantize
      fo[i] = sg ? -tf : tf ;         // restore sign
    }
    for(i0=ni7 ; i0<ni-7 ; i0+=8){
      for(i=0 ; i<8 ; i++){
        tf = qi[i0+i] ;
        sg = (qi[i0+i] < 0) ;         // save sign
        tf = sg ? -tf : tf ;          // absolute value
        tf = tf * fac32 + offset ;    // unquantize
        fo[i0+i] = sg ? -tf : tf ;    // restore sign
      }
    }
  }else{
    for(i=0 ; i<(ni & 7) ; i++){
      tf = qi[i] ;
      tf = tf * fac32 + offset ;      // unquantize
      fo[i] = allm ? -tf : tf ;       // restore sign if all negative
    }
    for(i0=ni7 ; i0<ni-7 ; i0+=8){
      for(i=0 ; i<8 ; i++){
        tf = qi[i0+i] ;
        tf = tf * fac32 + offset ;    // unquantize
        fo[i0+i] = allm ? -tf : tf ;  // restore sign if all negative
      }
    }
  }
  return 0 ;
}

// evaluate the discretization quantum as a function of nbits (type 0)
FloatPair IEEE32_linear_quantum_1(uint64_t u64, int nbits){
//   ieee32_props h64 ;
//   uint64_t allp, allm, pos_neg ;
  FloatPair result = {0.0f, 0.0f } ;
//   AnyType32 maxa, mina ;

//   h64.u = u64 ;
//   allm = h64.l.allm ;            // extract all useful values from u64
//   allp = h64.l.allp ;
//   maxa.u = h64.l.maxa ;
//   mina.u = h64.l.mina ;
//   pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values

  return result ;
}

// prepare for linear quantization of IEEE floats (type 1)
// handles mixed signs well, handles medium exponent range well
// u64   [IN]  analysis from IEEE32_limits
// np    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data (0 means use quantum)
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
// N.B. quant and nbits CANNOT BE 0 at the same time
uint64_t IEEE32_linear_prep_1(uint64_t u64, int np, int nbits, float quant){
  ieee32_props h64 ;
  uint32_t range, efac, erange, allp, allm, pos_neg, emax, emin ;
  AnyType32 q, m1, m2, m3, r, f, t ;

  // deal with quant <= 0 and nbits <= 0 (cannot both be 0)
  if(quant <= 0 && nbits <= 0) nbits = 15 ; // default to 15 bits quantization

  h64.u = u64 ;                      // get inbound metadata
  allp  = h64.l.allp ;               // no negative values
  allm  = h64.l.allm ;               // all negative values
  pos_neg = (allp || allm) ? 0 : 1 ; // mixed signs
  m1.u  = h64.l.maxa ;               // largest absolute value
  emax  = (m1.u >> 23) ;
  m2.u  = pos_neg ? 0 : h64.l.mina ; // smallest absolute value as offset if not mixed signs
  emin  = (m2.u >> 23) ;             // 0 if mixed signs
  q.f   = quant ;
  q.u  &= 0x7F800000 ;               // get rid of quantum mantissa
  quant = q.f ;                      // quant is now a power of 2
  if(emax - emin > nbits) {          // value range too large for nbits
    m2.u = 0 ;                       // max / min > 2**nbits, set minimum absolute value to 0
  }

  if(nbits == 0){                    // adjust nbits to quantum if nbits is "automatic"
    m3.f  = m1.f - m2.f ;            // range of values (float)
    r.f   = m3.f / q.f ;             // range / quantum
    erange = (r.u >> 23) - 127 ;     // exponent of range / quantum
    nbits = erange + 1 ;             // number of bits deemed necessary
    nbits = (nbits < 1) ? 1 : nbits ;
    if(pos_neg) nbits++ ;            // allow extra bit for sign
//         fprintf(stderr, "DEBUG: range is %f, quantum = %f, ratio = %f, nbits = %d(%d)\n",
//                 m3.f, q.f, r.f, nbits, nbits-pos_neg);
  }

adjust_qo:                           // m2.f (minimum absolute value) should be a multiple of quantum
  range = m1.u - m2.u ;              // range as an integer (0 is the only significant value)
  m3.f  = m1.f - m2.f ;              // range of values (float)
  m3.u |= 0x7FFFFF ;                 // force range mantissa to all 1s
  if(quant <= 0){                    // adjust quantum to nbits if <= 0
    int nbits2 = nbits ;
    if(pos_neg) nbits2-- ;
    nbits2 = (nbits2<1) ? 1 : nbits2 ;  // minimum = 1 bit
    q.u = (m3.u >> 23) - nbits2 + 1 ;   // adjust quantum to reflect nbits
    q.u = (q.u << 23) ;                 // quantum is a power of 2
//         fprintf(stderr, "adjusted quantum = %f(%f), m1 = %f, m2 = %f, m3 = %f, nbits2 = %d(%d)\n",
//                 q.f, 1.0f/q.f, m1.f, m2.f, m3.f, nbits2, nbits) ;
  }
  uint32_t ratio = m2.f / q.f ;      // truncated integer value of minimum / quantum
  t.f = ratio * q.f ;
  if(t.f != m2.f){                   // offset is not a multiple of quantum
//         fprintf(stderr, "DEBUG: adjusting offset, %f to %f, quantum = %f\n", m2.f, t.f, q.f) ;
    m2.f = t.f ;                     // offset is now a multiple of q
    goto adjust_qo ;                 // q and offset might need to be readjusted
  }

  r.f   = m3.f / q.f ;               // adjusted range / quantum
  erange = (r.u >> 23) - 127 ;       // unbiased exponent for range / quantum
//         fprintf(stderr, "np = %d, nbits = %d, quant = %f(%f)\n", np, nbits, quant, q.f) ;

  if(nbits <= 0){
    nbits = erange + 1 ;             // adjust nbits to reflect quantum
    nbits = (nbits<1) ? 1 : nbits ;  // minimum = 1 bit
    if(pos_neg) nbits++ ;
//         fprintf(stderr, "adjusted nbits = %d\n", nbits) ;
  }
  f.f   = 1.0f/q.f ;                 // factor to bring (largest number - offset) to 2**nbits -1 ;
  efac  = (f.u >> 23) ;              // exponent of quantization factor (for restore)
// qm1 = (m1.f - m2.f)*f.f + 0.5f ;
//         fprintf(stderr, "quantization factor = %f, qm1 = %d, efac = %d\n", f.f, qm1, efac) ;

  h64.u = 0 ;                         // set outbound metadata
  h64.q.bias = m2.u ;                 // offset (multiple of quantum)
  h64.q.npts = np ;                   // number of values
  h64.q.efac = efac ;                 // exponent of quantization factor (~ range/quantum)
  h64.q.nbts = nbits ;                // + pos_neg not needed ;
  h64.q.cnst = (range == 0) ? 1 : 0 ; // constant field
  h64.q.allm = allm ;                 // all values negative
  h64.q.allp = allp ;                 // no negative value
  return h64.u ;
}

// int64_t maxmin_qo = 0 ;
// quantize IEEE floats using information from IEEE32_linear_prep_1
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// u64   [IN]  metadata information describing quantization (from IEEE32_linear_prep_1)
// quantized output is a set of unsigned integers, needing at most nbts or nbts+1 bits
// if all values to be quantized have the same sign, allp/allm flags are used (nbts bits)
// if mixed signs, sign/magnitude storage is used, the sign is stored as the LSB (nbts+1 bits)
uint64_t IEEE32_quantize_linear_1(void * restrict f, uint64_t u64, void * restrict qs){
  float *ff = (float *) f ;
  uint32_t *qo = (uint32_t *) qs ;
  ieee32_props h64 ;
  uint32_t allm, allp, pos_neg, sg ;
  AnyType32 s, b ;
  int i, npts, nbts ;
  int32_t maxq, maxm ;
//   int32_t maxqo, minqo ;
  float fa ;
  int ia ;

  h64.u = u64 ;
  s.u   = h64.q.efac << 23 ;           // scaling factor for quantization
  npts  = h64.q.npts ;
  allp  = h64.q.allp ;                 // no negative values
  allm  = h64.q.allm ;                 // only negative values
  pos_neg = (allp || allm) ? 0 : 1 ;
  if(pos_neg) h64.q.bias = 0 ;         // set bias to 0 if mixed signs
  b.u   = h64.q.bias ;                 // bias
  nbts  = h64.q.nbts - pos_neg ;
  maxq  = RMASK31(nbts) ;
//   minqo = 0x7FFFFFFF ;
//   maxqo = -minqo ;
//      fprintf(stderr, "nbits = %d(%d), maxq = %d, offset = %f, factor = %f\n", h64.q.nbts, nbts, maxq, b.f, s.f) ;
  if(pos_neg){                         // mix of negative and non negative values
    float ft ;
    int maxm = -maxq ;
    for(i=0 ; i<npts ; i++){
      fa = ff[i] ;
      ft = fa < 0 ? -.5f : .5f ;       // round with -.5 if negative, +.5 if positive
      ia = fa * s.f + ft ;             // ( value * factor ) + round
      ia = (ia > maxq) ? maxq : ia ;   // clip to nbts bits
      ia = (ia < maxm) ? maxm : ia ;
      qo[i] = ia ;
//       maxqo = qo[i] > maxqo ? qo[i] : maxqo ;
//       minqo = qo[i] < minqo ? qo[i] : minqo ;
    }
  }else{                               // all values have the same sign
    for(i=0 ; i<npts ; i++){
      fa = ff[i] ;
      fa = (fa < 0) ? -fa : fa ;       // ABS(fa)
      ia = (fa - b.f) * s.f + 0.5f ;   // quantize ( (value - bias) * factor )
      ia = (ia > maxq) ? maxq : ia ;   // clip to nbts bits
      qo[i] = ia ;
//       maxqo = qo[i] > maxqo ? qo[i] : maxqo ;
//       minqo = qo[i] < minqo ? qo[i] : minqo ;
    }
  }
  // we might want to return the quantized values range q(maxs) - q(mins)
//   maxmin_qo = (uint32_t)minqo ; maxmin_qo = (maxmin_qo << 32) | ((uint32_t)maxqo & 0xFFFFFFFF) ;
  return u64 ;
}

// typedef struct {
//   uint64_t bias:32,  // integer offset reflecting minimum value of packed field
//            npts:16,  // number of points, 0 means unknown
//            efac:8 ,  // exponent for quantization mutliplier
//            nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
//            cnst:1 ,  // range is 0, constant absolute values
//            allp:1 ,  // all numbers are >= 0
//            allm:1 ;  // all numbers are < 0
// } ieee32_q ;
// quantize IEEE floats
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// ni    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
uint64_t IEEE32_linear_quantize_1(void * restrict f, int ni, int nbits, float quantum, void * restrict qs){
  ieee32_props h64 ;

  h64.u = IEEE32_limits(f, ni) ;                            // get min and max absolute values
  h64.u = IEEE32_linear_prep_1(h64.u, ni, nbits, quantum) ; // get quantization parameters (type 0)
  h64.u = IEEE32_quantize_linear_1(f, h64.u, qs) ;          // actual quantization (type 0)
  return h64.u ;
}

// ========================================== linear quantizer 2 ==========================================

// normalize a mantissa to a reference exponent by shifting it right
// src    [IN] : 32 bit IEEE float
// RefExp [IN} : reference exponent
int32_t normalize_mantissa(int32_t src, int32_t RefExp)
{
  int32_t Mantis = (1 << 23) | ( 0x7FFFFF & src );   // extract IEEE mantissa, restore hidden 1
  int32_t Exp    = (src >> 23) & 0xFF;               // extract IEEE float 32 exponent (includes +127 bias)
  if(Exp == 0) return 0 ;                            // denormalized float, return 0
  int32_t Shift  = RefExp - Exp;                     // shift count to normalize mantissa to RefExp exponent
  Shift = (Shift <  0) ?  0 : Shift ;                // Exp > RefExp, cannot normalize (invalid condition)
  Shift = (Shift > 31) ? 31 : Shift ;                // max possible shift count = 31
  Mantis = Mantis >> Shift;                          // normalize mantissa to RefExp exponent
  Mantis = ( src < 0 ) ? -Mantis : Mantis ;          // apply sign
  return Mantis ;
}

// prepare for linear quantization of IEEE floats (type 2)
int64_t IEEE32_linear_prep_2(uint64_t u64, int np, int nbits, float error)
{
  ieee32_props  h64  ;
  AnyType32 fmin ,fmax;
  int32_t MinExp, MaxExp, BigExp, Shift2, Maximum, Minimum, Mask ;

  // nbits and error CANNOT BE both <= 0 (automatic determination) set nbits to 15
  if(nbits <= 0 && error <= 0) nbits = 15 ;

  h64.u = u64 ;
  fmin.i = h64.s.mins ;                          // signed minimum
  fmax.i = h64.s.maxs ;                          // signed maximum
  MaxExp = (fmax.i >> 23) & 0xFF ;               // biased exponent
  MinExp = (fmin.i >> 23) & 0xFF ;               // biased exponent
  BigExp = (MaxExp > MinExp) ? MaxExp : MinExp ; // largest exponent
  Maximum = normalize_mantissa(fmax.i, BigExp) ;
  Minimum = normalize_mantissa(fmin.i, BigExp) ;
  Maximum = Maximum - Minimum;                   // range of normalized mantissas (>= 0)
  Shift2 = 0;
  Mask   = ~( -1 << nbits);                      // right mask of nbits bits
  while ( Maximum > Mask ) {                     // Maximum MUST fit within nbits bits
    Maximum = Maximum >> 1;
    Shift2++;
    }
  // further adjustment may be needed if error <= 0 or nbits <= 0 (auto mode)

  h64.r.npts = np ;               // number of values (0->16385)
  h64.r.expm = BigExp ;           // largest IEEE exponent (including +127 bias)
  h64.r.shft = Shift2 ;           // reflects scaling
  h64.r.bias = Minimum ;          // offset (signed)
  h64.r.nbts = nbits ;            // number of bits needed
  return h64.u ;
}

// restore 1 IEEE float from quantized value
static inline float IEEE32_Q2F_linear_2(int32_t q, int32_t offset, int32_t maxExp, int32_t shift2){
  AnyType32 temp1, temp2 ;
  int32_t hidden1 = 1 << 23 ;
  q = q << shift2 ;                        // scale
  q = q + offset ;                         // add offset
  int32_t sgn = (q >> 31) & 1 ;            // extract sign
  q = sgn ? -q : q ;                       // abs(mnantis)
  q = (q > 0xFFFFFF) ? 0xFFFFFF : q ;      // clip to 24 bits
  temp1.i = (sgn << 31) | (maxExp << 23) ; // restore exponent and sign
  temp2.i = (q & hidden1) ? 0 : temp1.i ;  // 0 if "hidden 1", compensation term if no "hidden 1"
  q &= 0x7FFFFF ;                          // get rid of "hidden 1" in mantissa
  temp1.i = temp1.i | q ;                  // restore mantissa ;
  return temp1.f - temp2.f ;               // compensate for possibly missing "hidden 1"
}
// external version of above
float IEEE32_q2f_linear_2(int32_t q, int32_t offset, int32_t maxExp, int32_t shift2){
  return IEEE32_Q2F_linear_2(q, offset, maxExp, shift2) ;
}

// quantize 1 IEEE float value
static inline int32_t IEEE32_F2Q_linear_2(int32_t Src, int32_t MaxExp, int32_t Shift2, int32_t Minimum, int32_t Mask){
  int32_t Mantis, Exp, Shift ;
  Mantis = (1 << 23) | ( 0x7FFFFF & Src );   // get IEEE mantissa, restore hidden 1
  Exp    = (Src >> 23) & 0xFF;               // extract IEEE float 32 exponent
  Shift  = MaxExp - Exp;                     // shift count to normalize mantissa to largest exponent
  if (Shift > 31) Shift = 31;                // max shift count = 31
  Mantis = Mantis >> Shift;                  // normalize mantissa to largest exponent
  if( Src >> 31 ) Mantis = - Mantis;         // apply sign
  Mantis = Mantis - Minimum;                 // subtract minimum from mantissa, add rounding term
  Mantis = Mantis >> Shift2;                 // force to fit within nbits bits
  if (Mantis > Mask) Mantis = Mask;          // clip to avoid exceeding all nbits bits set
  return Mantis ;
}
// external version of above
int32_t IEEE32_f2q_linear_2(int32_t Src, int32_t MaxExp, int32_t Shift2, int32_t Minimum, int32_t Mask){
  return IEEE32_F2Q_linear_2(Src, MaxExp, Shift2, Minimum, Mask) ;
}
// normalize mantissas to largest exponent
uint64_t IEEE32_quantize_linear_2(void * restrict f, uint64_t u64, void * restrict qs)
{
  int32_t *intsrc= (int32_t *)f;
  int32_t *qu = (int32_t *) qs ;
  int32_t i, npts;
  int32_t MaxExp, Exp, Mantis, Shift, Minimum, Src, Shift2, Round, nbits, Mask;
  ieee32_props h64 ;

  h64.u   = u64 ;
  npts    = h64.r.npts ;
  MaxExp  = h64.r.expm ;
  Shift2  = h64.r.shft ;
  Minimum = h64.r.bias ;
  nbits   = h64.r.nbts ;
  Round = (1 << Shift2) ;
  Round >>= 1 ;
  Minimum = Minimum - Round ;
  Mask = RMASK31(nbits) ;

// Lib_Log(APP_LIBRMN,APP_DEBUG,"%f: axExp=%d min=%f fmin.i=%X max=%f Minimum=%d Maximum=%\n",__func__,MaxExp,fmin.f,fmin.i,fmax.f,Minimum,Maximum); */
  for(i=0 ; i<npts ; i++){                     // transform input floating point into integers
    qu[i] = IEEE32_F2Q_linear_2(intsrc[i], MaxExp, Shift2, Minimum, Mask) ;  // use inline function
//     Src = intsrc[i];
//     Mantis = (1 << 23) | ( 0x7FFFFF & Src );   // get IEEE mantissa, restore hidden 1
//     Exp    = (Src >> 23) & 0xFF;               // extract IEEE float 32 exponent
//     Shift  = MaxExp - Exp;                     // shift count to normalize mantissa to largest exponent
//     if (Shift > 31) Shift = 31;                // max shift count = 31
//     Mantis = Mantis >> Shift;                  // normalize mantissa to largest exponent
//     if( Src >> 31 ) Mantis = - Mantis;         // apply sign
//     Mantis = Mantis - Minimum;                 // subtract minimum from mantissa, add rounding term
//     Mantis = Mantis >> Shift2;                 // force to fit within nbits bits
//     if (Mantis > Mask) Mantis = Mask;          // clip to avoid exceeding all nbits bits set
//     qu[i] = Mantis ;
    }
  return u64 ;
}

uint64_t IEEE32_linear_quantize_2(void * restrict f, int ni, int nbits, float quantum, void * restrict qs){
  ieee32_props h64 ;

  limits_w32 l32 = IEEE32_extrema(f, ni);                   // get min and max signed values
  h64.s.mins = l32.i.mins ;
  h64.s.maxs = l32.i.maxs ;
  h64.u = IEEE32_linear_prep_2(h64.u, ni, nbits, quantum) ; // get quantization parameters (type 0)
  h64.u = IEEE32_quantize_linear_2(f, h64.u, qs) ;          // actual quantization (type 0)
  return h64.u ;
}

int IEEE32_linear_unquantize_2(void * restrict q, uint64_t u64, int ni, void * restrict f){
  int32_t *qu = (int32_t *) q ;
  float *dest = (float *)f;
  ieee32_props h64 ;
  int32_t i, mantis, sgn, hidden1 = 1 << 23 ;

  h64.u = u64 ;
  int32_t offset  = h64.r.bias ;
  int32_t maxExp  = h64.r.expm ;
  int32_t shift2  = h64.r.shft ;
  int32_t npts    = h64.r.npts ;
  AnyType32 temp1, temp2 ;

  if (maxExp == 0) {
      for(i = 0 ; i < ni ; i++) dest[i] = 0.0 ;
      return 0;
  }
  for(i = 0 ; i < ni ; i++){
    dest[i] = IEEE32_Q2F_linear_2(qu[i], offset, maxExp, shift2) ;  // use inline function
//     mantis = qu[i] ;
//     mantis = mantis << shift2 ;                        // scale
//     mantis = mantis + offset ;                         // add offset
//     sgn = (mantis >> 31) & 1 ;                         // extract sign
//     mantis = sgn ? -mantis : mantis ;                  // abs(mnantis)
//     mantis = (mantis > 0xFFFFFF) ? 0xFFFFFF : mantis ; // clip to 24 bits
//     temp1.i = (sgn << 31) | (maxExp << 23) ;           // restore exponent and sign
//     temp2.i = (mantis & hidden1) ? 0 : temp1.i ;       // 0 if "hidden 1", compensation term if no "hidden 1"
//     mantis &= 0x7FFFFF ;                               // get rid of "hidden 1" in mantissa
//     temp1.i = temp1.i | mantis ;                       // restore mantissa ;
//     dest[i] = temp1.f - temp2.f ;                      // compensate for possibly missing "hidden 1"
  }

  return 0 ;
}

// ========================================== quantizer type 3 (exp+mantissa) ==========================================

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
void IEEE32_clip(void *f, int n, int nbits){
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
STATIC inline int32_t IEEE32_f_to_q(float f, int32_t e0, int32_t round, float t0, int nm, int32_t limit, int32_t sbit){
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
STATIC inline float IEEE32_q_to_f_2(int32_t q, float t1, float t2, int nm, int32_t sbit, int32_t neg){
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
STATIC inline float IEEE32_q_to_f_1(int32_t q, float t1t2, int nm, int32_t sbit, int32_t neg){
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
// the lower (nbits - nexp) bits contain a mantissa
// the "hidden one" and "denormalization" features of IEEE 754 are used
// example : nbits = 16, nexp = 4
//           the largest value  0 eeeeeeee mmmmmmmmmmmmxxxxxxxxxxx becomes
//                                    1111 mmmmmmmmmmmm
//           denormalized value 0 00000000 ddddddddddddxxxxxxxxxxx becomes
//                                    0000 dddddddddddd
// numbers 0.0 -> *fmaxa get converted to range
//         0.0 -> 2 ** (e[nexp] + 1 -127)
//         taking advantage of denormalized numbers to extend the dynamic range
int32_t IEEE32_quantize(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
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
    q[i] = IEEE32_f_to_q(f[i], e0, round, t0.f, nm, limit, sbit) ;
    min = (q[i] < min) ? q[i] : min ;
    max = (q[i] > max) ? q[i] : max ;
  }
  if(h != NULL){
    h->e0 = e0 ;             // true exponent of largest absolute value float
    h->nbits = nbits ;       // number of bits per token
    h->nexp = nexp ;         // number of exponent bits
//     h->min = min ;
//     h->max = max ;
    h->max = IEEE32_f_to_q(h->fmax, e0, round, t0.f, nm, limit, sbit) ;  // largest quantized signed value
    h->min = IEEE32_f_to_q(h->fmin, e0, round, t0.f, nm, limit, sbit) ;  // lowest quantized signed value
    h->limit = limit ;       // keep limit mask
  }
  return e0 ;  // retourner e0/nbits/nexp/sbit/negative ?  8/8/8/4/4 bits ?
}

// vector version of above
int32_t IEEE32_quantize_v4(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
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
      q[j+i] = IEEE32_f_to_q(f[j+i], e0, round, t0.f, nm, limit, sbit) ;
    }
    j += vl ;
    vl = 4 ;
  }
  if(h != NULL){
    h->e0 = e0 ;
    h->nbits = nbits ;
    h->nexp = nexp ;
    h->max = IEEE32_f_to_q(h->fmax, e0, round, t0.f, nm, limit, sbit) ;
    h->min = IEEE32_f_to_q(h->fmin, e0, round, t0.f, nm, limit, sbit) ;
    h->limit = limit ;       // keep limit mask
  }
  return e0 ;
}

// restore float values from quantized (ieee style) values
// utiliser un composite e0/nbits/nexp/sbit/negative  ( 8/8/8/4/4 bits ) plutot que h ?
int32_t IEEE32_unquantize(float *f,      // restored array (IEEE 754 32 bit float) (OUTPUT)
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
  e0 = h->e0 ;                               // reference exponent (from IEEE32_quantize)
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
      f[i] = IEEE32_q_to_f_2(q[i], t1.f, t2.f, nm, sbit, neg) ;
    }
  }else{                                     // can use 1 factor if e0 <= e[nexp]
fprintf(stdout,"BOP\n");
    t1.u = ((254 - e[nexp] + e0) << 23) ;
    for(i = 0 ; i < n ; i++) {
      f[i] = IEEE32_q_to_f_1(q[i], t1.f, nm, sbit, neg) ;
    }
  }
  return 1 ;
}

// ========================================== pseudo IEEE floats ==========================================

// IEEE 32 bit floating point to half precision (16 bit) IEEE floating point
// any number >= 65520 will be coded as infinity in FP16
STATIC inline uint16_t IEEE32_fp32_to_fp16(float f){
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
  for(i = 0 ; i < n ; i++) q[i] = IEEE32_fp32_to_fp16(f[i]) ;
}

STATIC inline uint32_t IEEE32_fp32_to_fp24(float f){
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
      t[i] = IEEE32_fp32_to_fp24(f[i0+i]) ;
    }
    // pack tokens big endian style
    q[0] = (t[0] <<  8) | (t[1] >> 16) ;  // t0 , upper 8 bits of t1
    q[1] = (t[1] << 16) | (t[2] >>  8) ;  // lower 16 bits of t1, upper 16 bits fo t2
    q[2] = (t[2] << 24) | (t[3]) ;        // lower 8 bits of t2, t3
    q += 3 ;
  }
  for(i=0 ; i<4 ; i++) t[i] = 0 ;
  for(i=0 ; i0<n ; i0++, i++){
    t[i] = IEEE32_fp32_to_fp24(f[i0]) ;
  }
  if(i>0) q[0] = (t[0] <<  8) | (t[1] >> 16) ;  // t0 , upper 8 bits of t1
  if(i>1) q[1] = (t[1] << 16) | (t[2] >>  8) ;  // lower 16 bits of t1, upper 16 bits fo t2
  if(i>2) q[2] = (t[2] << 24) | (t[3]) ;        // lower 8 bits of t2, t3
//   for(i = 0 ; i < n ; i++) q[i] = IEEE32_fp32_to_fp16(f[i]) ;
}

// scaled IEEE 32 bit floating point to half precision (16 bit) IEEE floating point
void fp32_to_fp16_scaled(float *f, uint16_t *q, int n, float scale){
  int i ;
  for(i = 0 ; i < n ; i++) q[i] = IEEE32_fp32_to_fp16(f[i]*scale) ;
}

// scaled IEEE 32 bit floating point to 3/4 precision (24 bit) IEEE style floating point
// 4 values in f for each group of 4 q values
void fp32_to_fp24_scaled(float *f, uint32_t *q, int n, float scale){
  int i ;
  for(i = 0 ; i < n ; i++) q[i] = IEEE32_fp32_to_fp24(f[i]*scale) ;
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
STATIC inline float IEEE32_fp16_to_fp32(uint16_t q, uint32_t infinity){
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
  for(i = 0 ; i < n ; i++) f[i] = IEEE32_fp16_to_fp32(q[i], Inf) ;
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
  for(i = 0 ; i < n ; i++) f[i] = rscale * IEEE32_fp16_to_fp32(q[i], Inf) ;
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

