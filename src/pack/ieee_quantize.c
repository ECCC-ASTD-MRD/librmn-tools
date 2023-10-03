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

// not much of a slowdown with CLIP_TO_NBITS defined
#define CLIP_TO_NBITS

// first analysis pass, find absolute value extrema and sign properties
typedef struct{
  uint32_t allp:1 ,  // all numbers are >= 0
           maxa:31;  // maximum absolute value (never more than 31 bits)
  uint32_t allm:1 ,  // all numbers are <= 0
           mina:31;  // minimum absolute value (never more than 31 bits)
} ieee32_l ;

// signed value analysis
// typedef struct{
//   int32_t  maxs ;    // maximum value
//   int32_t  mins ;    // minimum value
// } ieee32_s ;

// equivalent to above ieee32_s, using floats
// typedef struct{
//   float  maxs ;    // maximum value
//   float  mins ;    // minimum value
// } ieee32_f ;

// some (useful for quantization) properties of an IEEE float array, 64 bits total
// types ieee32_props and ieee32_p are for internal use, uint64_t is exposed in the interface
//
// type 0 linear quantization header
typedef struct{
  uint32_t bias:32;  // minimum absolute value (actually never more than 31 bits)
  uint32_t npts:14,  // number of points, 0 means unknown
           resv:5 ,
           shft:5 ,  // shift count (0 -> 31) for quantize/restore
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ,  // all numbers are < 0
           nbts:5 ;  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
} ieee32_p ;

// type 1 linear quantization header
typedef struct {
  uint32_t bias:32;  // integer offset reflecting minimum value of packed field
  uint32_t npts:14,  // number of points, 0 means unknown
           resv:2 ,
           efac:8 ,  // exponent for quantization multiplier
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ,  // all numbers are < 0
           nbts:5 ;  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
} ieee32_q ;

// type 2 linear quantization header
typedef struct {
  int32_t  bias:32;  // signed integer offset reflecting minimum value of packed field
  uint32_t npts:14,  // number of points, 0 means unknown
           expm:8 ,  // largest IEEE exponent (biased)
           shft:5 ,  // shift count (0 -> 31) for normalization
           nbts:5 ;  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
} ieee32_r ;

// fake log quantization header (type 0)
typedef struct {
  uint32_t qmin:32;
  uint32_t emax:8 ,  // exponent of largest absolute value
           emin:8 ,  // exponent of smallest absolute value
           elow:8 ,  // exponent of smallest significant absolute value
           clip:1 ,  // original emin < elow, clipping may occur
           allp:1 ,  // all numbers are >= 0
           allm:1 ,  // all numbers are < 0
           nbts:5 ;  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
} ieee32_e ;

// fake log quantization header (type 1)
typedef struct {
  uint32_t qmin:32;  // offset
  uint32_t emax:8 ,  // exponent of largest absolute value
           emin:8 ,  // exponent of smallest significant absolute value
           mbts:8 ,  // number of effective mantissa bits (shift = 23 - mbts)
           clip:1 ,  // original emin < elow, clipping may occur
           allp:1 ,  // all numbers are >= 0
           allm:1 ,  // all numbers are < 0
           nbts:5 ;  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
} ieee32_f ;

// access to all properties structs through a single 64 bit wide union
typedef union{
  ieee32_p  p  ;     // type 0 linear header
  ieee32_q  q  ;     // type 1 linear header
  ieee32_r  r  ;     // type 2 linear header
  ieee32_l  l  ;     // absolute value analysis
//   ieee32_s  s  ;     // signed value analysis
//   ieee32_s  f  ;     // float reflecting above signed value analysis
  ieee32_e  e  ;     // type 0 fake log header
  ieee32_f  f  ;     // type 1 fake log header
  qhead    *h  ;     // pointer to full quantization header (used by type 3)
  uint64_t  u  ;     // access as a single 64 bit unsigned integer
  uint32_t  w[2]  ;  // access as 2 32 bit unsigned integers
} ieee32_props ;
// make sure that access as a single 64 bit unsigned integer is respected
CT_ASSERT(sizeof(ieee32_props) == sizeof(uint64_t))

// ========================================== common functions ==========================================
// "normalize" a mantissa to a reference exponent by shifting it right (after adding hidden 1)
// apply sign of input float to mantissa
// src    [IN] : 32 bit IEEE float
// RefExp [IN} : reference exponent
// return signed "normalized" value
int32_t normalize_mantissa(int32_t src, int32_t RefExp)
{
  int32_t Mantis = (1 << 23) | ( 0x7FFFFF & src );   // extract IEEE mantissa, restore hidden 1
  int32_t Exp    = (src >> 23) & 0xFF;               // extract IEEE float 32 exponent (includes +127 bias)
  if(Exp == 0) return 0 ;                            // denormalized float, return 0
  int32_t Shift  = RefExp - Exp;                     // shift count to normalize mantissa to RefExp exponent
  Shift = (Shift <  0) ?  0 : Shift ;                // Exp > RefExp, cannot normalize (invalid condition, should not happen)
  Shift = (Shift > 31) ? 31 : Shift ;                // max possible shift count = 31
  Mantis = Mantis >> Shift;                          // normalize mantissa to RefExp exponent
  Mantis = ( src < 0 ) ? -Mantis : Mantis ;          // apply sign
  return Mantis ;
}

// prepare for quantization of IEEE floats (combined type 0/1/2)
int IEEE32_linear_prep(limits_w32 l32, int np, int nbits, float errmax, uint64_t u64[3], int hint){
  ieee32_props h64_0, h64_1, h64_2 ;
  uint32_t maxa, mina, allm, allp, pos_neg, rangeu, lz ;
  int32_t mins, maxs, nbits0, nbits1, nbits2 ;
  AnyType32 fmis ,fmas, q, fmiu, fmau ;
  float quant, quant0, quant1, quant2 ;

  // deal with errmax <= 0 and nbits <= 0 (cannot both be <= 0)
  if(nbits <= 0) nbits = 15 ;             // default to 15 bits quantization
  if(errmax > 0) nbits = 0 ;              // will use quant to compute nbits
  nbits0 = nbits1 = nbits2 = nbits ;

  errmax = (errmax < 0.0f) ? 0.0f : errmax ;
  q.f    = errmax * 2.0f  ;
  q.u   &= 0x7F800000 ;                   // get rid of quantum mantissa
  quant  = q.f ;                          // quant is now a positive power of 2 (or 0)

  maxa = l32.i.maxa ;
  mina = l32.i.mina ;
  maxs = l32.i.maxs ;
  mins = l32.i.mins ;
  allm = l32.i.allm ;                     // get all useful values from l32
  allp = l32.i.allp ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values
//   if(pos_neg) mina = 0 ;                  // if mixed signs, set minimum absolute value to 0

  h64_0.u = h64_1.u = h64_2.u = 0 ;
  u64[0]  = u64[1]  = u64[2]  = 0 ;
  rangeu = maxa - mina ;                  // value range (considering ABS(float) as an unsigned integer)
  if(rangeu == 0) goto constant ;         // special case : constant absolute values
  fmiu.u = mina ;                         // smallest absolute value
  fmau.u = maxa ;                         // largest absolute vlue
  fmis.i = mins ;                         // signed minimum
  fmas.i = maxs ;                         // signed maximum

// ======================================= type 0 =======================================
  if(hint == Q_MODE_LINEAR_1 || hint == Q_MODE_LINEAR_2) goto skip0 ;  // direct hint points to another type
  {
  AnyType32 fi1, fi2 ;
  int32_t nbitsmax, scount ;
  float delta ;
  if(nbits0 == 0) {                        // compute nbits0 from quantum, type 0
    int32_t temp ;
    fi1.u = maxa ; fi2.u = mina ; 
    fi1.f = fi1.f - fi2.f ;                // fi1.f = range of data values
    fi2.f = quant ;
    temp = (fi1.u >> 23) - (fi2.u >> 23) ; // IEEE exponent difference between range and quantum
    nbits0 = temp + 1 + pos_neg ;
    nbits0 = (nbits0 < 1) ? 1 : nbits0 ;   // at least 1 bit
  }
  // type 0 needs uint32_t bias, np, shft0, cnst, allp, allm, nbits0
  lz = lzcnt_32(rangeu) ;                  // number of leading zeroes in range
  nbitsmax = 32 - lz ;
  if(pos_neg) {                            // sign bit needs one bit, reduce allowed bit count by 1, increase nbitsmax
    nbitsmax++ ;
    nbits0-- ;
  }
  if(nbits0 > nbitsmax) nbits0 = nbitsmax ; // no point in exceeding nbitsmax
  scount = 32 - lz - nbits0 ; 
  scount = (scount < 0) ? 0 : scount ;     // scount CANNOT be < 0
  fi1.u = (maxa >> scount) << scount ;     // drop lower scount bits
  fi2.u = fi1.u - (1 << scount) ;
  delta = fi1.f - fi2.f ;                  // difference between values whose quantization differs by 1 unit
  // if quantum < delta, nbits0 may need to be increased (except if quantum == 0, when nbits0 must be used)
  while( (quant < delta) && (quant > 0.0f) && (nbits0 < nbitsmax) ){
    nbits0++ ;
    scount = 32 - lz - nbits0 ;
    scount = (scount < 0) ? 0 : scount ;
    fi1.u = (maxa >> scount) << scount ;
    fi2.u = fi1.u - (1 << scount) ;
    delta = fi1.f - fi2.f ;             // difference between values whose quantization differs by 1 unit
  }
  quant0 = delta ;                      // quantum for type 0
  h64_0.p.shft = scount ;
  h64_0.p.nbts = nbits0 ;
  h64_0.p.npts = np ;
  h64_0.p.allp = allp ;
  h64_0.p.allm = allm ;
  h64_0.p.cnst = 0 ;
  h64_0.p.bias = mina ;
  u64[0] = h64_0.u ;
//   if(hint == 0) h64.u = h64_0.u ;       // direct hint, this will be the function output
  }
skip0:
// ======================================= type 1 =======================================
  if(hint == Q_MODE_LINEAR_0 || hint == Q_MODE_LINEAR_2) goto skip1 ;  // direct hint points to another type
  {
  uint32_t emax, emin, efac, erange ;
  AnyType32 m3, t, q1 ;
  emax = fmau.u >> 23 ;
  emin = fmiu.u >> 23 ;

  if(nbits1 == 0){                      // compute nbits1 from quantum, type 1
    AnyType32 r ;
adjust_nb:
    r.f    = (fmau.f - fmiu.f) / quant ;// abs range / quantum
    erange = (r.u >> 23) - 127 ;        // exponent of range / quantum
    nbits1 = erange + 1 ;               // number of bits deemed necessary for this range of values
    nbits1 = (nbits1 < 1) ? 1 : nbits1 ;// at least 1 bit
    if(pos_neg) nbits1++ ;              // add an extra bit for the sign
    if(emax - emin > nbits1 && fmiu.u != 0) {
      fmiu.u = 0 ; emin = 0 ;           // value exponent range exceeds nbits1, set minimum value to 0
      goto adjust_nb ;                  // one more adjustment pass is needed
    }
  }
  // type 1 needs uint32_t bias, np, efac, cnst, allp, allm, nbits1
  if(emax - emin > nbits1) {            // value exponent range exceeds nbits1
    fmiu.u = 0 ; emin = 0 ;             // max / min > 2**nbits1, set minimum absolute value to 0
  }
  q1.f = quant ;
adjust_qo:                              // fmiu.f (minimum absolute value) should be a multiple of quantum
  m3.f  = fmau.f - fmiu.f ;             // range of values (float)
  m3.u |= 0x7FFFFF ;                    // force range mantissa to all 1s
  if(quant == 0){                       // compute quantum using nbits1 if quant == 0
    int nbitst = nbits1 ;
    if(pos_neg) nbitst-- ;
    nbitst = (nbitst<1) ? 1 : nbitst ;  // minimum = 1 bit
    q1.u = (m3.u >> 23) - nbitst + 1 ;  // compute quantum to reflect nbits1
    q1.u = (q1.u << 23) ;               // quantum is a power of 2
  }
  uint32_t ratio = fmiu.f / q1.f ;      // truncated integer value of minimum / quantum
  t.f = ratio * q1.f ;
  if(t.f != fmiu.f){                    // offset is not a multiple of quantum
    fmiu.f = t.f ;                      // offset is now a multiple of q
    goto adjust_qo ;                    // q and offset might need to be readjusted
  }

  quant1 = q1.f ;                       // quantum for type 1
  t.f   = 1.0f/q1.f ;                   // factor to bring (largest number - offset) to 2**nbits1 -1 ;
  efac  = (t.u >> 23) ;                 // exponent of quantization factor (for restore)

  h64_1.q.bias = fmiu.u ;               // offset (multiple of quantum)
  h64_1.q.npts = np ;                   // number of values
  h64_1.q.efac = efac ;                 // exponent of quantization factor (~ range/quantum)
  h64_1.q.nbts = nbits1 ;               // + pos_neg not needed ;
  h64_1.q.cnst = 0 ;                    // constant field
  h64_1.q.allm = allm ;                 // all values negative
  h64_1.q.allp = allp ;                 // no negative value
  u64[1] = h64_1.u ;
//   if(hint == 1) h64.u = h64_1.u ;       // direct hint
  }
skip1:
// ======================================= type 2 =======================================
  if(hint == Q_MODE_LINEAR_0 || hint == Q_MODE_LINEAR_1) goto skip2 ;  // direct hint points to another type
  {
  int32_t MinExp, MaxExp, BigExp, Shift2, Maximum, Minimum, Mask, Range ;
  if(nbits2 == 0){                                // compute nbits2 from errmax, type 2
    AnyType32 r, e ;
    int32_t needbits ;
    r.f = fmas.f - fmis.f ;
    r.i |= 0x7FFFFF ;
    e.f = errmax ;
    e.i &= 0x7F800000u ;
    needbits = (r.i >> 23) - (e.i >> 23) ;
    nbits2 = needbits > 0 ? needbits : 1 ;   // at least 1 bit
  }
  // type 2 needs int32_t bias, np, expm, shft2, nbits2
  MaxExp = (fmas.i >> 23) & 0xFF ;               // biased exponent of signed maximum
  MinExp = (fmis.i >> 23) & 0xFF ;               // biased exponent of signed minimum
  BigExp = (MaxExp > MinExp) ? MaxExp : MinExp ; // largest exponent
  Maximum = normalize_mantissa(fmas.i, BigExp) ; // normalize mantissa of maximum to largest exponent
  Minimum = normalize_mantissa(fmis.i, BigExp) ; // normalize mantissa of minimum to largest exponent
  Range = Maximum - Minimum ;                    // range of normalized mantissas (>= 0)

  Shift2 = 0 ;
  Mask   = ~( -1 << nbits2) ;                    // right mask of nbits2 bits
  while ( Range > Mask ) {                       // Range MUST fit within nbits2 bits
    Range = Range >> 1;
    Shift2++;
    }
  quant2 = quant ;                  // quantum for type 2
  h64_2.r.npts = np ;               // number of values (0->16385)
  h64_2.r.expm = BigExp ;           // largest IEEE exponent (including +127 bias)
  h64_2.r.shft = Shift2 ;           // shift count reflecting scaling
  h64_2.r.bias = Minimum ;          // offset (signed)
  h64_2.r.nbts = nbits2 ;           // number of bits needed
  u64[2] = h64_2.u ;
//   if(hint == 2) h64.u = h64_2.u ;   // direct hint
  }
skip2:
  return hint ;

constant :
  return Q_MODE_CONSTANT ;   // all ones in returned value, all 0s in u64
}

// quantize IEEE 32 bit floats
// qs     [OUT]  32 bit integer array that will receive the quantized result
// f       [IN]  32 bit IEEE float array, data to be quantized
// meta [INOUT]  pointer to q_meta structure (quantization metadata)
// nd      [IN]  number of data items to quantize
int64_t IEEE_quantize(void * restrict f, void * restrict q, q_meta *meta,  int nd, int nbits, float errmax, 
                      int mode, void *spval, uint32_t mmask, void *pad)
{
  limits_w32 l32 ;
  ieee32_props h64 ;
  int submode ;
  uint32_t *spval_ = (uint32_t *)spval, *pad_ = (uint32_t *)pad, spmask = 0 ;
  int special ;
  int samesign;
  int i ;

  errmax = (errmax < 0.0f) ? 0.0f : errmax ;
  l32 = IEEE32_extrema_missing(f, nd, spval, mmask, pad) ;   // step 1 : analyze data
  meta->maxs.i = l32.i.maxs ;
  meta->mins.i = l32.i.mins ;
  meta->spval.u = spval_ ? *spval_ : 0 ;
  meta->mmask.u = mmask ;
  meta->pad.u   = pad_ ? *pad_ : 0 ;

  mmask = (~mmask) ;
  special = (spval != NULL) || (mmask == 0) ;
  if(special) spmask = mmask & *spval_ ;
  samesign = l32.i.allp || l32.i.allm ;

  // special h64 item for constant array ?
  if(l32.i.maxa == l32.i.min0){  // constant array (maxa == min0)
    uint32_t *fu = (uint32_t *) f ;
    int32_t  *fi = (int32_t  *) f ;
    uint32_t *qu = (uint32_t *) q ;
    // maxs == mins, same sign, no special     : 0 bit
    if((l32.i.maxs == l32.i.mins) && (! special)) { 
      meta->nbits = 0 ;
      return 0 ;
    }
    // mina == min0, not same sign, no special : 1 bit
    if((l32.i.mina == l32.i.min0) && (! special) && (! samesign)){
      meta->nbits = 1 ;
      for(i=0 ; i<nd ; i++) qu[i] = (fu[i] >> 31) ;   // sign = unsigned_value >> 31
      return 0 ;
    }
    // mina != min0, same sign, no special     : 1 bit
    if((l32.i.mina != l32.i.min0) && (! special) && samesign){
      meta->nbits = 1 ;
      for(i=0 ; i<nd ; i++) qu[i] = (fu[i] == 0) ? 0 : 1 ;  // ( 0 : zero,   1 : !zero)
      return 0 ;
    }
    // mina == min0, same sign, special value  : 1 bit  ( 0 : value,  1 : special)
    if((l32.i.mina == l32.i.min0) && special && samesign){
      meta->nbits = 1 ;
      for(i=0 ; i<nd ; i++) qu[i] = ((fu[i] & mmask) == spmask) ? 1 : 0 ;  // (==special ? 1 : 0)
      return 0 ;
    }
    // not same sign, no special value         : 2 bits (00 : zero,  01 : >zero, 10 : <zero 11 : 11 : N/A)
    // not same sign, special value            : 2 bits (00 : zero,  01 : >zero, 10 : <zero 11 : 11 : special)
    meta->nbits = 2 ;
    if(special){
      for(i=0 ; i<nd ; i++) {
        qu[i]  = ((fu[i] & mmask) == spmask) ? 3 : 0 ;  // ( ==special ? 3 : 0)
        qu[i] |= ((fi[i] > 0) ? 1 : 0) ;                // | ( >0 ? 1 : 0)
        qu[i] |= ((fi[i] < 0) ? 2 : 0) ;                // | ( <0 ? 2 : 0)
      }
    }else{
      for(i=0 ; i<nd ; i++) {
        qu[i]  = (fi[i] > 0) ? 1 : 0 ;                  // | ( >0 ? 1 : 0)
        qu[i] |= (fi[i] < 0) ? 2 : 0 ;                  // | ( <0 ? 2 : 0)
      }
    }
    return 0 ;
  }
  if(mode == 0){
    mode = Q_MODE_LINEAR ;                                     // default to a linear quantizer
  }
  // step 2 : use requested quantization mode
  if(mode & Q_MODE_LINEAR){                                    // one of the linear quantizers
    uint64_t u64[3] ;
    int hint ;
    hint = IEEE32_linear_prep(l32, nd, nbits, errmax, u64, mode) ;  // prep for 1 or more linear modes
    switch(hint)
    {
      case Q_MODE_LINEAR:                                      // default linear mode is type 0
      case Q_MODE_LINEAR_0:
        h64.u = u64[0] ;                                       // quantization parameters (type 0)
        h64.u = IEEE32_quantize_linear_0(f, h64.u, q) ;        // actual quantization (type 0)
        break ;
      case Q_MODE_LINEAR_1:
        h64.u = u64[1] ;                                       // quantization parameters (type 1)
        h64.u = IEEE32_quantize_linear_1(f, h64.u, q) ;        // actual quantization (type 1)
        break ;
      case Q_MODE_LINEAR_2:
        h64.u = u64[2] ;                                       // quantization parameters (type 2)
        h64.u = IEEE32_quantize_linear_2(f, h64.u, q) ;        // actual quantization (type 2)
        break ;
      default:                                                 // probably constant array, should have been detected before
        return -1 ;   // error
    }
  }else if(mode & Q_MODE_LOG){
    switch(submode)
    {
      case 1:
        break ;
      case 2:
        break ;
      default:
        return -1 ;
    }
  }else{
    return -1 ;
  }
  return 0 ;
}

int64_t IEEE_qrestore(void * restrict f, void * restrict q, q_meta *meta,  int nd)
{
  int mode = meta->mode ;
  return 0 ;
}
// ========================================== fake log quantizer 0 ==========================================

// restore floating point numbers quantized with IEEE32_fakelog_quantize_0
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// u64   [IN]  metadata information describing quantization (from IEEE32_fakelog_quantize_0)
// N.B. if values have mixed signs, the sign is stored as the LSB in the quantized integer
int IEEE32_fakelog_restore_0(void * restrict f, uint64_t u64, int ni, void * restrict q){
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

  limits_w32 l32 = IEEE32_extrema(fu, ni) ;
  allm = l32.i.allm ;                     // extract all useful values from u64
  allp = l32.i.allp ;
  maxa = l32.i.maxa ;
  mina = l32.i.mina ;
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
// NOTE: we will want to force emin > emax - 64 and store emax - emin and store emin instead of emax
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

// ========================================== fake log quantizer 1 ==========================================

// restore floating point numbers quantized with IEEE32_fakelog_quantize_1
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items
// u64   [IN]  metadata information describing quantization (from IEEE32_fakelog_quantize_0)
// N.B. if values have mixed signs, the quantized data is signed
//      a zero quantized value will be restored either as 0 or the minimum value
//      depending upon flag "clip"
//      IEEE 32 bit floats are processed as if they were signed integers
// int IEEE32_fakelog_restore_1(void * restrict f, uint64_t u64, int ni, void * restrict q){
q_desc IEEE32_fakelog_restore_1(void * restrict f, int ni, q_desc desc, void * restrict q){
  int32_t *qi = (int32_t *) q ;
  int32_t *fo = (int32_t *) f ;
  q_desc q64 = {.u = 0 } ;  // invalid restored state
  int i, nbits, shift, offset, mbits, clip, pos_neg, sign, fa, fs, zero, f0 ;

  if(desc.q.type  != Q_FAKE_LOG_1) goto error ;  // wrong quantizer

  nbits  = desc.q.nbits ;
  offset = desc.q.offset.i ;
  mbits  = desc.q.mbits ;
  clip   = desc.q.clip ;
  shift  = 23 - mbits ;
  pos_neg= (desc.q.allm | desc.q.allp) ? 0 : 1 ;
  sign   = desc.q.allm ;
  sign <<= 31 ;
  zero = clip ? 0 : offset ;

  if(pos_neg){
    for(i=0 ; i<ni ; i++){
      fs = qi[i] & 0x80000000 ;             // save sign
      f0 = (qi[i] < 0) ? -qi[i] : qi[i] ;   // absolute value
      f0-- ;                                // to preserve sign for minimum absolute value
      fa  = f0 << shift ;                   // apply shift
      fa += offset ;                        // add offset term
      fa  = (f0 == 0) ? zero : fa ;         // special case for quantized value == 0
      fa |= fs ;                            // apply sign
      fo[i] = fa ;
    }
  }else{
    for(i=0 ; i<ni ; i++){
      f0  = qi[i] ;
      fa  = f0 << shift ;                   // apply shift
      fa += offset ;                        // add offset term
      fa  = (f0 == 0) ? zero : fa ;         // special case for quantized value == 0
      fa |= sign ;                          // apply sign
      fo[i] = fa ;
    }
  }
  q64.u = desc.u ;
  q64.r.npts = ni ;
  q64.r.state = RESTORED ;
  return q64 ;

error:
  q64.u = 0 ;
  return q64 ;
}

// quantize IEEE floats (fake log type 1)
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// ni    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data
// qzero [IN]  smallest value considered significant (if qzero < 0, it will be restored as 0.0)
// N.B. qzero will be reduced to a power of 2 <= qzero
//      if there are mixed signs, the quantized result will be signed
//      IEEE 32 bit floats are processed as if they were signed integers
  q_desc IEEE32_fakelog_quantize_1(void * restrict f, int ni, q_desc rule, void * restrict q){
  uint32_t *fu = (uint32_t *) f ;
  int32_t *qo = (uint32_t *) q ;
  q_desc q64 = {.u = 0 } ;  // set invalid output state
  uint32_t allp, allm, pos_neg, maxa, mina, round ;
  int32_t emax, emin, elow, ebits, erange, mbits, shift, offset ;
  AnyType32 t ;
  int nbits = rule.f.nbits ;
  float qzero = rule.f.ref ;

  if(rule.f.state != TO_QUANTIZE) goto error ;                  // invalid f state
  if(rule.f.nbits == 0 && rule.f.mbits == 0) goto error ;       // cannot be both set to 0

  limits_w32 l32 = IEEE32_extrema(fu, ni) ;
// extract needed values from l32
  allp = l32.i.allp ;                     // all values >= 0
  allm = l32.i.allm ;                     // all values < 0
  maxa = l32.i.maxa ;                     // largest absolute value
  mina = l32.i.mina ;                     // smallest absolute value
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values

// fill result struct
  q64.q.allp = allp ;                     // all values >= 0 flag
  q64.q.allm = allm ;                     // all values  < 0 flag
// prepare to quantize
  t.f  = qzero ;                          // smallest significant value
  elow = (t.u >> 23) & 0xFF ;             // IEEE exponent of qzero, will be 0 when qzero == 0.0
  emax = (maxa >> 23) & 0xFF ;            // IEEE exponent of largest absolute value
  emin = (mina >> 23) & 0xFF ;            // IEEE exponent of smallest absolute value (emin may be < elow)
  if((qzero < 0) && (emin < elow))        // will force a restore to zero if quantized value == 0
    q64.q.clip = 1 ;
  emin = (emin < elow) ? elow : emin ;    // if emin < elow, set emin to elow for quantization
  q64.q.emin = emin ;                     // store adjusted value of emin into q64
  emax -= 127 ;                           // remove IEEE exponent bias from emax, emin
  emin -= 127 ;
  erange = emax - emin ;                  // exponent range
  ebits = 32 - lzcnt_32(erange) ;         // number of bits needed to encode exponent range (0 -> 8)

  mbits = rule.f.mbits ;
  if(mbits > 0){                          // desired precision has been specified
    mbits = (mbits > 23) ? 23 : mbits ;   // make sure mbits <= 23
    nbits = ebits + mbits ;
  }else{                                  // desired precision has not been specified, but nbits has
    if(nbits < 1 + ebits + pos_neg)       // smallest value of nbits that makes sense
      nbits  = 1 + ebits + pos_neg ;
    if(nbits > 23 + ebits + pos_neg)      // largest value of nbits that makes sense
      nbits  = 23 + ebits + pos_neg ;
    nbits -= pos_neg ;                    // one bit is needed for sign ==> one less available for quantized value
    mbits = nbits - ebits ;               // number of bits of mantissa left (should NEVER be > 23)
  }
  q64.q.nbits = nbits ;
  q64.q.mbits = mbits ;

  round = (mbits == 23) ? 0 : 1 << (22 - mbits) ;     // rounding term (0 if mbits == 23)
  shift = 23 - mbits ;
  offset = (emin + 127) << 23 ;           // emin exponent with 0 mantissa
  offset = (mina > offset) ? mina : offset ;  // minimum absolute value if larger than emin exponent with 0 mantissa
  q64.q.offset.i = offset ;

// quantize values
  int32_t fa, fs ;
  int i ;
  if(pos_neg){
    for(i=0 ; i<ni ; i++){
      fa = fu[i] & 0x7FFFFFFF ;             // absolute value
      fa = (offset > fa) ? offset : fa ;    // max(fa, offset)
      fs = fu[i] >> 31 ;                    // save sign
      fa += round ;                         // add rounding term
      fa -= offset ;                        // subtract offset term
      fa >>= shift ;                        // shift right to fit within nbits
      fa++ ;                                // to be able to preserve sign of minimum absolute value
      qo[i] = fs ? -fa : fa ;               // apply sign
    }
  }else{
    for(i=0 ; i<ni ; i++){
      fa = fu[i] & 0x7FFFFFFF ;             // absolute value
      fa = (offset > fa) ? offset : fa ;    // max(fa, offset)
      fa += round ;                         // add rounding term
      fa -= offset ;                        // subtract offset term
      fa >>= shift ;                        // shift right to fit within nbits
      qo[i] = fa ;
    }
  }
fprintf(stderr, "\n");
  q64.q.state = QUANTIZED ;                 // everything O.K.
  q64.q.type  = Q_FAKE_LOG_1 ;
  return q64 ;

error:
  q64.u = 0 ;
  return q64 ;
}

// ========================================== linear quantizer 0 ==========================================

// restore floating point numbers quantized with IEEE32_linear_quantize_0
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// u64   [IN]  metadata information describing quantization (from IEEE32_linear_quantize_0)
// N.B. if values have mixed signs, the sign is stored as the LSB in the quantized integer
int IEEE32_linear_restore_0(void * restrict q, uint64_t u64, int ni, void * restrict f){
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
      for(i=0 ; i<ni ; i++){                       // first chunk (0 - > 7 items)
        temp = qi[i] ;
        sign = (temp & 1) << 31 ;                        // get sign
        temp >>= 1 ;                                     // remove sign
        qi[i] = (temp + offset) << scount ;              // restore
        qi[i] |= sign ;                                  // apply sign
      }
    }else{              // positive values only
      for(i=0 ; i<ni ; i++){                       // first chunk (0 - > 7 items)
        qi[i] = (qi[i] + offset) << scount ;             // restore
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
      for(i=0 ; i<ni ; i++){                       // first chunk (0 - > 7 items)
          temp = qi[i] ;
          sign = (temp & 1) << 31 ;                      // get sign
          temp >>= 1 ;                                   // remove sign
          fo[i] = (temp + offset) << scount ;            // restore
          fo[i] |= sign ;                                // apply sign
      }
    }else{
      sign = h64.p.allm << 31 ;
      for(i=0 ; i<ni ; i++){                       // first chunk (0 - > 7 items)
        fo[i] = (qi[i] + offset) << scount ;             // restore
        fo[i] |= sign ;
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
// l32   [IN]  analysis from IEEE32_extrema
// np    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data (0 means use quantum)
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
// uint64_t IEEE32_linear_prep_0(uint64_t u64, int np, int nbits, float errmax){
uint64_t IEEE32_linear_prep_0(limits_w32 l32, int np, int nbits, float errmax){
  ieee32_props h64 ;
  uint32_t maxa, mina, rangeu, lz ;
  uint32_t pos_neg, allm, allp, cnst ;
  int32_t scount, nbitsmax ;
  AnyType32 fi1, fi2 ;
  float delta ;
  float quant ;

  // deal with errmax <= 0 and nbits <= 0 (cannot both be 0)
  if(nbits <= 0) nbits = 15 ;             // default to 15 bits quantization
  errmax = (errmax < 0.0f) ? 0.0f : errmax ;
  if(errmax > 0) nbits = 0 ;              // will use quant to compute nbits
  fi1.f = 2.0f * errmax ;
  fi1.u &= 0xFF800000u ;
  quant = fi1.f ;

  allm = l32.i.allm ;                     // get all useful values from l32
  allp = l32.i.allp ;
  maxa = l32.i.maxa ;
  mina = l32.i.mina ;

  h64.u = 0 ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values
  rangeu = maxa - mina ;                  // value range (considering ABS(float) as an unsigned integer)
  lz = lzcnt_32(rangeu) ;                 // number of leading zeroes in range
  // 32 - number of most significant 0 bits in rangeu = max possible number of useful bits
  nbitsmax = 32 - lz ;
  cnst = 0 ;                              // a priori not a constant field
  if(rangeu == 0) {                       // special case : constant absolute values
    scount = 0 ;
    nbits = pos_neg ;                     // no bits needed if all values have the same sign
    cnst = 1 ;                            // constant field flag
    goto end ;
  }
  if(nbits <= 0) {                        // automatic determination of nbits, quantum > 0
    int32_t temp ;
    fi1.u = maxa ; fi2.u = mina ; 
    fi1.f = fi1.f - fi2.f ;               // fi1.f = range of data values
    fi2.f = quant ;
    temp = (fi1.u >> 23) - (fi2.u >> 23) ;  // IEEE exponent difference between range and quantum
    nbits = temp + 1 + pos_neg ;
//       fprintf(stderr, "prep_0 : range = %g, quantum = %g\n", fi1.f , quant) ;
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
//       fprintf(stderr, "delta = %g, quant = %g, nbits = %d", delta, quant, nbits) ;
      nbits++ ;
      scount = 32 - lz - nbits ;
      scount = (scount < 0) ? 0 : scount ;
      fi1.u = (maxa >> scount) << scount ;
      fi2.u = fi1.u - (1 << scount) ;
      delta = fi1.f - fi2.f ;             // difference between values whose quantization differs by 1 unit
//       fprintf(stderr, ", adjusted to delta = %g, nbits = %d\n", delta, nbits) ;
    }
  }

end:                         // return ieee32_props struct
  h64.p.shft = scount ;
  h64.p.nbts = nbits + pos_neg ;
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
// fprintf(stderr, "quantizing with nbits = %d, scount = %d\n", nbits, scount) ;
  if(h64.p.cnst){
    if(pos_neg){
      for(i=0 ; i<ni ; i++) qo[i] = fu[i] >> 31 ;
    }
    goto end ;
  }
  ni7 = (ni & 7) ;
  if(f == qs){      // quantize IN PLACE
    if(pos_neg){    // both negative and non negative floats will be quantized
      for(i=0 ; i<ni ; i++){
        sign = fu[i] >> 31 ;                                          // save sign
        fu[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
#if defined(CLIP_TO_NBITS)
        fu[i] = MIN(fu[i],maskn) ;                                    // clip if needed
#endif
        fu[i] = (fu[i] << 1) | sign ;                                 // store sign
      }
    }else{          // all floats to be quantized have the same sign
      for(i=0 ; i<ni ; i++){
        fu[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
#if defined(CLIP_TO_NBITS)
        fu[i] = MIN(fu[i],maskn) ;                                    // clip if needed
#endif
      }
    }
  }else{            // quantize NOT IN PLACE
    if(pos_neg){    // both negative and non negative floats will be quantized
      for(i=0 ; i<ni ; i++){
        sign = fu[i] >> 31 ;                                          // save sign
        qo[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;  // quantize
#if defined(CLIP_TO_NBITS)
        qo[i] = MIN(qo[i],maskn) ;                                    // clip if needed
#endif
        qo[i] = (qo[i] << 1) | sign ;                                 // store sign
      }
    }else{          // all floats to be quantized have the same sign
      for(i=0 ; i<ni ; i++){
        qo[i] = (( (masksign & fu[i]) + round) >> scount) - offset ;          // quantize
#if defined(CLIP_TO_NBITS)
        qo[i] = MIN(qo[i],maskn) ;                                            // clip if needed
#endif
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
uint64_t u64[3], t64 ;
  limits_w32 l32 = IEEE32_extrema(f, ni);                   // get min and max absolute values

//   h64.u = IEEE32_linear_prep_0(l32, ni, nbits, quantum) ; // get quantization parameters (type 0)
  t64 = IEEE32_linear_prep(l32, ni, nbits, quantum, u64, Q_MODE_LINEAR_0) ;
  h64.u = u64[0] ;
// if(t64 != h64.u) fprintf(stderr, "t64 != h64.u : %16.16lx %16.16lx\n", t64, h64.u);
// else fprintf(stderr, "t64 == h64.u\n");
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
int IEEE32_linear_restore_1(void * restrict q, uint64_t u64, int ni, void * restrict f){
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
    return -0xFFFFFF ;
  }
// fprintf(stderr, "fac32 = %f, offset = %f, pos_neg = %d\n", fac32, offset, pos_neg);
  ni7 = (ni & 7) ; ni7 = (ni7 == 0) ? 8 : ni7 ;
  if(pos_neg){
#if defined(__x86_64__) && defined(__AVX2__)
// useful with some compilers that have difficulties vectorizing the code
    if(ni > 7 && q != f){          // ni is assumed to be at least 8, does not work "inplace"
      __m256i vq, vs, vsign ;
      __m256  vf, vfac, voff ;
      vsign = _mm256_set1_epi32(1u << 31) ;
      vfac  = _mm256_set1_ps(fac32) ;
      voff  = _mm256_set1_ps(offset) ;
      for(i=0 ; i<ni-7 ; i+=ni7, ni7=8){
        vq = _mm256_loadu_si256((__m256i *)(qi + i)) ;  // get quantized values
        vs = _mm256_and_si256(vq, vsign) ;              // save sign
        vq = _mm256_abs_epi32(vq) ;                     // absolute value
        vf = _mm256_cvtepi32_ps(vq) ;                   // convert to float
        vf = _mm256_fmadd_ps(vf, vfac, voff) ;          // * fac32 + offset
        vf = _mm256_or_ps(vf, (__m256)vs) ;             // restore sign
        _mm256_storeu_ps(fo + i, vf) ;                  // store restored floats
      }
      return ni ;
    }
#endif
    for(i=0 ; i<ni ; i++){
      tf = qi[i] ;
      sg = (qi[i] < 0) ;              // save sign
      tf = sg ? -tf : tf ;            // absolute value
      tf = tf * fac32 + offset ;      // restore
      fo[i] = sg ? -tf : tf ;         // restore sign
    }
  }else{
#if defined(__x86_64__) && defined(__AVX2__)
// useful with some compilers that have difficulties vectorizing the code
    if(ni > 7 && q != f){          // ni is assumed to be at least 8, does not work "inplace"
      __m256i vq, vs ;
      __m256  vf, vfac, voff ;
      vfac  = _mm256_set1_ps(fac32) ;
      voff  = _mm256_set1_ps(offset) ;
      vs    = _mm256_set1_epi32(allm << 31) ;           // 1 if allm is true, 0 otherwise
      for(i=0 ; i<ni-7 ; i+=ni7, ni7=8){
        vq = _mm256_loadu_si256((__m256i *)(qi + i)) ;  // get quantized values
        vf = _mm256_cvtepi32_ps(vq) ;                   // convert to float
        vf = _mm256_fmadd_ps(vf, vfac, voff) ;          // * fac32 + offset
        vf = _mm256_or_ps(vf, (__m256)vs) ;             // restore sign
        _mm256_storeu_ps(fo + i, vf) ;                  // store restored floats
      }
      return ni ;
    }
#endif
    for(i=0 ; i<ni ; i++){
      tf = qi[i] ;
      tf = tf * fac32 + offset ;      // restore
      fo[i] = allm ? -tf : tf ;       // restore sign if all negative
    }
  }
  return ni ;
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
// l32   [IN]  analysis from IEEE32_extrema
// np    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data (0 means use quantum)
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
// N.B. quant and nbits CANNOT BE 0 at the same time
uint64_t IEEE32_linear_prep_1(limits_w32 l32, int np, int nbits, float errmax){
  ieee32_props h64 ;
  uint32_t range, efac, erange, allp, allm, pos_neg, emax, emin ;
  AnyType32 q, m1, m2, m3, r, f, t ;
  float quant ;

  // deal with errmax <= 0 and nbits <= 0 (cannot both be 0)
  if(nbits <= 0) nbits = 15 ;        // default to 15 bits quantization
  if(errmax > 0) nbits = 0 ;         // will use quant to compute nbits

  allp  = l32.i.allp ;               // no negative values
  allm  = l32.i.allm ;               // all negative values
  pos_neg = (allp || allm) ? 0 : 1 ; // mixed signs
  m1.u  = l32.i.maxa ;               // largest absolute value
  emax  = (m1.u >> 23) ;
  m2.u  = pos_neg ? 0 : l32.i.mina ; // smallest absolute value as offset if not mixed signs
  emin  = (m2.u >> 23) ;             // smallest absolute value is set to 0 if mixed signs
  q.f   = errmax < 0.0f ? 0.0f : errmax * 2.0f  ;
  q.u  &= 0x7F800000 ;               // get rid of quantum mantissa
  quant = q.f ;                      // quant is now a positive power of 2 (or 0)

  if(quant > 0){                     // compute nbits from quant
adjustqn:
    m3.f  = m1.f - m2.f ;            // range of values (float)
    r.f   = m3.f / q.f ;             // range / quantum
    erange = (r.u >> 23) - 127 ;     // exponent of range / quantum
    nbits = erange + 1 ;             // number of bits deemed necessary for this range of values
    nbits = (nbits < 1) ? 1 : nbits ;// at least 1 bit
    if(pos_neg) nbits++ ;            // add an extra bit for the sign
    if(emax - emin > nbits && m2.u != 0) {
      m2.u = 0 ;                     // value exponent range exceeds nbits, set minimum value to 0
      goto adjustqn ;                // one more adjustment pass is needed
    }
  }
  if(emax - emin > nbits) {          // value exponent range exceeds nbits
    m2.u = 0 ;                       // max / min > 2**nbits, set minimum absolute value to 0
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

  h64.u = 0 ;                         // put outbound metadata into ieee32_q struct
  h64.q.bias = m2.u ;                 // offset (multiple of quantum)
  h64.q.npts = np ;                   // number of values
  h64.q.efac = efac ;                 // exponent of quantization factor (~ range/quantum)
  h64.q.nbts = nbits ;                // + pos_neg not needed ;
  h64.q.cnst = (range == 0) ? 1 : 0 ; // constant field
  h64.q.allm = allm ;                 // all values negative
  h64.q.allp = allp ;                 // no negative value
  return h64.u ;
}

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

//   if(npts > 7) return IEEE32_quantize_linear_1_SIMD(ff, u64, qo, npts, pos_neg, maxq, s.f, b.f) ;

//      fprintf(stderr, "nbits = %d(%d), maxq = %d, offset = %f, factor = %f\n", h64.q.nbts, nbts, maxq, b.f, s.f) ;
  if(pos_neg){                         // mix of negative and non negative values
    float ft ;
    int maxm = -maxq ;
    for(i=0 ; i<npts ; i++){
      fa = ff[i] ;
      ft = fa < 0 ? -.5f : .5f ;       // round with -.5 if negative, +.5 if positive
      ia = fa * s.f + ft ;             // ( value * factor ) + round
#if defined(CLIP_TO_NBITS)
      ia = (ia > maxq) ? maxq : ia ;   // clip to nbts bits
      ia = (ia < maxm) ? maxm : ia ;
#endif
      qo[i] = ia ;
    }
  }else{                               // all values have the same sign
    for(i=0 ; i<npts ; i++){
      fa = ff[i] ;
      fa = (fa < 0) ? -fa : fa ;       // ABS(fa)
      ia = (fa - b.f) * s.f + 0.5f ;   // quantize ( (value - bias) * factor )
#if defined(CLIP_TO_NBITS)
      ia = (ia > maxq) ? maxq : ia ;   // clip to nbts bits
#endif
      qo[i] = ia ;
    }
  }
  // we might want to return the quantized values range q(maxs) - q(mins)
//   maxmin_qo = (uint32_t)minqo ; maxmin_qo = (maxmin_qo << 32) | ((uint32_t)maxqo & 0xFFFFFFFF) ;
  return u64 ;
}

// quantize IEEE floats
// qs   [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// np    [IN]  number of data items to quantize
// nbits [IN]  number of bits to use for quantized data
// quant [IN]  quantization interval (if non zero, it is used instead of nbits)
uint64_t IEEE32_linear_quantize_1(void * restrict f, int np, int nbits, float quantum, void * restrict qs){
  ieee32_props h64 ;
uint64_t u64[3], t64 ;

  limits_w32 l32 = IEEE32_extrema(f, np);                   // get min and max absolute values
//   h64.u = IEEE32_linear_prep_1(l32, np, nbits, quantum) ;   // get quantization parameters (type 0)
  t64 = IEEE32_linear_prep(l32, np, nbits, quantum, u64, Q_MODE_LINEAR_1) ;
  h64.u = u64[1] ;
// if(t64 != h64.u) fprintf(stderr, "t64 != h64.u : %16.16lx %16.16lx\n", t64, h64.u);
// else fprintf(stderr, "t64 == h64.u\n");
  h64.u = IEEE32_quantize_linear_1(f, h64.u, qs) ;          // actual quantization (type 0)
  return h64.u ;
}

// ========================================== linear quantizer type 2 ==========================================

// restore 1 IEEE float from type 2 quantized value
// inline function used by IEEE32_linear_restore_2
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
#if defined(__x86_64__) && defined(__AVX2__)
// X86_64 SIMD version of above, useful with some compilers that have vectorizing difficulties
// n is assumed to be at least 8
static inline int32_t IEEE32_Q2F_linear_2_SIMD(float *f, int32_t *q, int32_t offset, int32_t maxExp, int32_t shift2, int n){
  __m256i voff, vmax, vsh2, vhid, vq, vs, v24b, v1, v2, vt, vz ;
  __m256 vf ;
  int i, n7 ;

  v24b = _mm256_set1_epi32(0xFFFFFF) ;
  voff = _mm256_set1_epi32(offset) ;
  vmax = _mm256_set1_epi32(maxExp << 23) ;
  vsh2 = _mm256_set1_epi32(shift2) ;
  vhid = _mm256_set1_epi32(1<<23) ;
  vz   = _mm256_xor_si256(v24b, v24b) ;
  n7 = n&7 ;
  n7 = (n7 == 0) ? 8 : n7 ;
  for(i=0 ; i<n-7 ; i+=n7, n7=8){
    vq = _mm256_loadu_si256 ((__m256i *)(q + i)) ;        // fetch quantized
    vq = _mm256_sllv_epi32(vq, vsh2) ;                    // scale
    vq = _mm256_add_epi32(vq, voff) ;                     // add offset
    vs = _mm256_srli_epi32(vq, 31) ;                      // extract sign
    vq = _mm256_abs_epi32(vq) ;                           // abs(mantis)
    vq = _mm256_min_epu32(vq, v24b) ;                     // clip to 24 bits
    vs = _mm256_slli_epi32(vs, 31) ;                      // sign in MSB
    v1 = _mm256_or_si256(vs, vmax) ;                      // restore exponent and sign
    vt = _mm256_and_si256(vq, vhid) ;                     // hidden 1 ?
    vt = _mm256_cmpeq_epi32(vt, vz) ;                     // 0 where hidden 1, FFFFFFFF where no hidden 1
    v2 = _mm256_and_si256(vt, v1) ;                       // v1 where no hidden 1, 0 where hidden 1
    vq = _mm256_andnot_si256(vhid, vq) ;                  // get rid of "hidden 1" in mantissa
    v1 = _mm256_or_si256(v1, vq) ;                        // restore mantissa ;
    vf = _mm256_sub_ps((__m256)v1, (__m256)v2) ;          // compensate for possibly missing "hidden 1"
    _mm256_storeu_ps((f + i), vf) ;                       // store restored float
  }
  return n ;
}
#endif

// quantize 1 IEEE float value using type 2 linear quantizer
// inline function used by IEEE32_quantize_linear_2
static inline int32_t IEEE32_F2Q_linear_2(int32_t Src, int32_t MaxExp, int32_t Shift2, int32_t Minimum, int32_t Mask){
  int32_t Mantis, Exp, Shift ;
  Mantis = (1 << 23) | ( 0x7FFFFF & Src );   // get IEEE mantissa, restore hidden 1
  Exp    = (Src >> 23) & 0xFF;               // extract IEEE float 32 exponent
  Shift  = MaxExp - Exp;                     // shift count to normalize mantissa to largest exponent
  if (Shift > 31) Shift = 31;                // max shift count = 31
  Mantis = Mantis >> Shift;                  // normalize mantissa to largest exponent
  if( Src >> 31 ) Mantis = - Mantis;         // apply sign of Src
  Mantis = Mantis - Minimum;                 // subtract minimum from mantissa, add rounding term
  Mantis = Mantis >> Shift2;                 // force to fit within nbits bits
#if defined(CLIP_TO_NBITS)
  Mantis = (Mantis > Mask) ? Mask : Mantis ; // clip to avoid exceeding all nbits bits set
//   if (Mantis > Mask) Mantis = Mask;
#endif
  return Mantis ;
}
// external version of above
int32_t IEEE32_f2q_linear_2(int32_t Src, int32_t MaxExp, int32_t Shift2, int32_t Minimum, int32_t Mask){
  return IEEE32_F2Q_linear_2(Src, MaxExp, Shift2, Minimum, Mask) ;
}

// prepare for linear quantization of IEEE floats (type 2)
// l32    [IN] : analysis from IEEE32_extrema
// np     [IN] : number of values (not used in function, but set in return value)
// nbits  [IN] : number of bits to use for quantization (0 means compute it internally)
// errmax [IN] : maximum absolute error target (if >= 0, use it to adjust nbits)
// return a 64 bit value representing an ieee32_r type struct
// N.B. nbits and errmax should not both be <= 0
// if both nbits and errmax are > 0, errmax has precedence
uint64_t IEEE32_linear_prep_2(limits_w32 l32, int np, int nbits, float errmax)
{
  ieee32_props  h64 ;
  AnyType32 fmin ,fmax, quantum, frange, error ; // , ratio ;
  int32_t MinExp, MaxExp, BigExp, Shift2, Maximum, Minimum, Mask, Range, needbits ;

  // deal with errmax <= 0 and nbits <= 0 (cannot both be 0)
  if(nbits <= 0) nbits = 15 ;             // default to 15 bits quantization
  if(errmax > 0) nbits = 0 ;              // will use quant to compute nbits
  if(np > 16385) return 0L ;                     // error : np MUST NOT exceed 16385

  fmin.i = l32.i.mins ;                          // signed minimum
  fmax.i = l32.i.maxs ;                          // signed maximum
  MaxExp = (fmax.i >> 23) & 0xFF ;               // biased exponent of maximum
  MinExp = (fmin.i >> 23) & 0xFF ;               // biased exponent of minimum
  BigExp = (MaxExp > MinExp) ? MaxExp : MinExp ; // largest exponent
  Maximum = normalize_mantissa(fmax.i, BigExp) ; // normalize mantissa of maximum to largest exponent
  Minimum = normalize_mantissa(fmin.i, BigExp) ; // normalize mantissa of minimum to largest exponent
  Range = Maximum - Minimum ;                    // range of normalized mantissas (>= 0)

  if(errmax > 0){                                // adjust nbits to errmax
    frange.f = fmax.f - fmin.f ;
    frange.i |= 0x7FFFFF ;
    error.f  = errmax ;
    error.i &= 0x7F800000u ;
//     ratio.f = frange.f / error.f ;
//     needbits = (ratio.i >> 23) - 127 ;
    needbits = (frange.i >> 23) - (error.i >> 23) ;
// fprintf(stderr, "errmax = %g(%g), range = %g, nbits = %d, needbits = %d\n", errmax, error.f, frange.f, nbits, needbits) ;
    nbits = needbits > 0 ? needbits : 1 ;
  }

  Shift2 = 0 ;
  Mask   = ~( -1 << nbits) ;                     // right mask of nbits bits
  while ( Range > Mask ) {                       // Range MUST fit within nbits bits
    Range = Range >> 1;
    Shift2++;
    }
// Mask >>= 1 ;  // quantum is the difference between 2 values whose quantized values differ by 1
// quantum.f = IEEE32_Q2F_linear_2(Mask+1, Minimum, MaxExp, Shift2) - IEEE32_Q2F_linear_2(Mask, Minimum, MaxExp, Shift2) ;
// fprintf(stderr, "nbits = %d, range = %g, quantum = %g\n", nbits, frange.f , quantum.f) ;

  h64.r.npts = np ;               // number of values (0->16385)
  h64.r.expm = BigExp ;           // largest IEEE exponent (including +127 bias)
  h64.r.shft = Shift2 ;           // shift count reflecting scaling
  h64.r.bias = Minimum ;          // offset (signed)
  h64.r.nbts = nbits ;            // number of bits needed
  return h64.u ;                  // return value is of the ieee32_r type
}

// quantize IEEE 32 bit floats ny normalizing mantissas to largest exponent
// f    [IN] : float array to quantize
// u64  [IN] : 64 bit value representing an ieee32_r struct (from IEEE32_linear_prep_2)
// qs  [OUT] : quantized values (signed integers)
// return the u64 argument (pass through)
// the IEEE floats are manipulated as if they were integers
uint64_t IEEE32_quantize_linear_2(void * restrict f, uint64_t u64, void * restrict qs)
{
  int32_t *intsrc= (int32_t *)f;
  int32_t *qu = (int32_t *) qs ;
  int32_t i, npts;
  int32_t MaxExp, Exp, Mantis, Shift, Minimum, Src, Shift2, Round, nbits, Mask;
  ieee32_props h64 ;

  h64.u   = u64 ;                              // input is of the ieee32_r type (from IEEE32_linear_prep_2)
  npts    = h64.r.npts ;                       // number of values (0->16385)
  MaxExp  = h64.r.expm ;                       // largest IEEE exponent (including +127 bias)
  Shift2  = h64.r.shft ;                       // shift count reflecting scaling
  Minimum = h64.r.bias ;                       // offset (signed)
  nbits   = h64.r.nbts ;                       // number of bits to be used for quantized values
  Round = (1 << Shift2) ;                      // rounding term
  Round >>= 1 ;
  Minimum = Minimum - Round ;                  // combine rounding term with offset
  Mask = RMASK31(nbits) ;                      // largest permissible quantized value

  for(i=0 ; i<npts ; i++){                     // transform input floating point into integers
    qu[i] = IEEE32_F2Q_linear_2(intsrc[i], MaxExp, Shift2, Minimum, Mask) ;  // use inline function
    }
  return u64 ;
}

// type 2 linear quantizer (normalizes mantissas to largest exponent, deals with IEEE hidden 1)
// f      [IN] : IEEE 32 bits float array to quantize
// np     [IN] : number of values (not used in function, but set in return value)
// nbits  [IN] : number of bits to use for quantization (0 means compute it internally)
// errmax [IN] : maximum absolute error target (if >= 0, use it to adjust nbits)
// qs    [OUT] : quantized values (signed integers)
// return value returned by IEEE32_quantize_linear_2
uint64_t IEEE32_linear_quantize_2(void * restrict f, int np, int nbits, float quantum, void * restrict qs){
  ieee32_props h64 ;
uint64_t u64[3], t64 ;

  limits_w32 l32 = IEEE32_extrema(f, np);                   // get min and max signed values
//   h64.u = IEEE32_linear_prep_2(l32, np, nbits, quantum) ;   // compute quantization parameters (type 2)
  t64 = IEEE32_linear_prep(l32, np, nbits, quantum, u64, Q_MODE_LINEAR_2) ;
  h64.u = u64[2] ;
// if(t64 != h64.u) fprintf(stderr, "t64 != h64.u : %16.16lx %16.16lx\n", t64, h64.u);
// else fprintf(stderr, "t64 == h64.u\n");
  h64.u = IEEE32_quantize_linear_2(f, h64.u, qs) ;          // actual quantization (type 2)
  return h64.u ;
}

// type 2 inverse linear quantizer (restores IEEE 32 bit floats))
// qs   [IN] : quantized values (signed integers)
// u64  [IN] : 64 bit value representing an ieee32_r struct (from IEEE32_linear_quantize_2)
// np   [IN] : number of values (used for consistency check)
// f   [OUT] : restored float array
// return number of restored values. if there is a mismatch, return - expected number of values
int IEEE32_linear_restore_2(void * restrict q, uint64_t u64, int np, void * restrict f){
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

  if(np != npts) return -npts ;    // error, number of values mismatch
  if (maxExp == 0) {
      for(i = 0 ; i < np ; i++) dest[i] = 0.0 ;
      return 0;
  }
#if defined(__x86_64__) && defined(__AVX2__)
  int ni7 = (np & 7) ;
  // call the scalar version for the first ni7 points, call the SIMD version for the rest
  for(i=0 ; i<ni7 ; i++) dest[i] = IEEE32_Q2F_linear_2(qu[i], offset, maxExp, shift2) ;
  IEEE32_Q2F_linear_2_SIMD(dest + ni7, qu + ni7, offset, maxExp, shift2, np - ni7) ;
  return npts ;
//   if(np > 7 && q != f) return IEEE32_Q2F_linear_2_SIMD(dest, qu, offset, maxExp, shift2, np) ;
#endif

  for(i = 0 ; i < np ; i++){
    dest[i] = IEEE32_Q2F_linear_2(qu[i], offset, maxExp, shift2) ;  // use inline function
  }

  return npts ;
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
int32_t IEEE32_restore(float *f,      // restored array (IEEE 754 32 bit float) (OUTPUT)
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

