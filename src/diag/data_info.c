//  Library of hopefully useful functions for C and FORTRAN
//  Copyright (C) 2022-2023  Recherche en Prevision Numerique
//                           Environnement Canada
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation,
//  version 2.1 of the License.
// 
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
// specific optimization option for gcc, try to avoid unrecognized pragma warning
#if defined(__GNUC__) && ! defined(__INTEL_COMPILER) && ! defined(__PGI) && ! defined(__clang__)
// #pragma GCC optimize "O3,tree-vectorize"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <float.h>

#include <rmn/misc_operators.h>
#include <rmn/data_info.h>

// compute maxa for 32 bit integers
// l32  [IN] limits structure
static uint32_t INT32_maxa(limits_w32 l32)
{
  int32_t maxs = l32.i.maxs ;
  int32_t mins = l32.i.mins ;
  maxs = (maxs < 0) ? -maxs : maxs ;          // ABS(maxs)
  mins = (mins < 0) ? -mins : mins ;          // ABS(mins)
  return (maxs > mins) ? maxs : mins ;        // MAX(ABS(maxs), ABS(mins))
}

// compute maxa for 32 bit IEEE floats
// l32  [IN] limits structure
static uint32_t IEEE32_maxa(limits_w32 l32)
{
  uint32_t maxs = l32.i.maxs & 0x7FFFFFFF ;          // ABS(maxs)
  uint32_t mins = l32.i.mins & 0x7FFFFFFF ;          // ABS(mins)
  return (maxs > mins) ? maxs : mins ;               // MAX(ABS(maxs), ABS(mins))
}

// replace "missing" values in 32 bit array
// f    [INOUT]  array of 32 bit values
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
int W32_replace_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad)
{
  uint32_t *fu = (uint32_t *) f, missf, *fill = (uint32_t *) pad, *miss = (uint32_t *) spval, repl ;
  int i ;

  if(f == NULL || spval == NULL || mmask == 0xFFFFFFFFu || pad == NULL) return -1 ;  // nothing can be done, error
  mmask = ~mmask ;
  missf = mmask & *miss ;                            // masked "missing" value
  repl  = *fill ;                                    // replacement value
  for(i=0 ; i<np ; i++){
    fu[i] = ( (fu[i] & mmask) == missf ) ? repl : fu[i] ;
  }
  return np ;
}

// get unsigned integer 32 extrema
// f     [IN]  32 bit unsigned integer array
// np    [IN]  number of data items
// return an extrema limits_w32 struct
limits_w32 UINT32_extrema(void * restrict f, int np)
{
  uint32_t *fu = (uint32_t *) f ;
  int i ;
  uint32_t maxa, mina, min0, tu, t0 ;
  limits_w32 l ;

  maxa = mina = fu[0] ;
  min0 = 0xFFFFFFFFu ;
  for(i=0 ; i<np ; i++){
    tu = fu[i] ;
    t0 = (tu == 0) ? 0xFFFFFFFEu : tu ;
    mina = (tu < mina) ? tu : mina ;
    min0 = (t0 < min0) ? t0 : min0 ;
    maxa = (tu > maxa) ? tu : maxa ;
  }
  l.i.maxs = 0 ;     // signed maximum makes no sense for unsigned integers, using unsigned value
  l.i.mins = 1 ;     // signed minimum makes no sense for unsigned integers, using unsigned value
  l.u.mina = mina ;
  l.u.min0 = min0 ;
  l.u.maxa = maxa ;
  l.u.allm = 0 ;
  l.u.allp = 1 ;
  return l ;
}

// get unsigned integer 32 extrema, "missing" values are accounted for
// f       [IN]  32 bit integer array
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
// return an extrema limits_w32 struct
limits_w32 UINT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad)
{
  uint32_t *fu = (uint32_t *) f ;
  int i ;
  uint32_t maxa, mina, min0, tu, t0, *miss = (uint32_t *)spval, missf, good, *fill = (uint32_t *)pad ;
  limits_w32 l ;


  mmask = ~mmask ;
  if(spval == NULL || mmask == 0) return UINT32_extrema(f, np) ;

  mina = 0xFFFFFFFFu ;
  maxa = 0 ;
  min0 = 0xFFFFFFFFu ;
  missf = mmask & *miss ;                            // masked "missing" value
  if(fill == NULL){                                  // no explicit replacement value
    good = (uint32_t)fu[0] ;                         // in case no non "missing" value found
    for(i=0 ; i<np ; i++){                           // find first non "missing" value
      good = (uint32_t)fu[i] ;
      if( (good & mmask) != missf) break ;           // non "missing" value found
    }
  }else{
    good = *fill ;                                   // we have an explicit replacement value
  }
  for(i=0 ; i<np ; i++){
    tu = fu[i] ;
    tu   = ((tu & mmask) == missf) ? good : tu ;     // replace "missing" values with "non missing" value
    t0 = (tu == 0) ? 0xFFFFFFFEu : tu ;              // ignore 0 for non zero minimum, replace with HUGE value
    mina = (tu < mina) ? tu : mina ;                 // smallest absolute value
    min0 = (t0 < min0) ? t0 : min0 ;                 // smallest non zero absolute value
    maxa = (tu > maxa) ? tu : maxa ;                 // unsigned largest absolute value
  }
  l.i.maxs = 0 ;     // signed maximum makes no sense for unsigned integers, using unsigned value
  l.i.mins = 1 ;     // signed minimum makes no sense for unsigned integers, using unsigned value
  l.u.mina = mina ;
  l.u.min0 = min0 ;
  l.u.maxa = maxa ;
  l.u.allm = 0 ;
  l.u.allp = 1 ;
  return l ;
}

// get integer 32 extrema (signed and absolute value)
// f     [IN]  32 bit signed integer array
// np    [IN]  number of data items
// l    [OUT]  extrema
limits_w32 INT32_extrema(void * restrict f, int np)
{
  int32_t *fs = (int32_t *) f ;
  int i ;
  uint32_t mina, min0, tu, t0 ;
  int32_t maxs, mins, ts ;
  limits_w32 l ;

  mina = 0x7FFFFFFF ;
  maxs = mins = fs[0] ;
  min0 = 0x7FFFFFFF ;
  for(i=0 ; i<np ; i++){
    ts = fs[i] ;
    tu = (ts < 0) ? -ts : ts ;
    t0 = (tu == 0) ? 0xFFFFFFFEu : tu ;
    mina = (tu < mina) ? tu : mina ;
    min0 = (t0 < min0) ? t0 : min0 ;
    maxs = (ts > maxs) ? ts : maxs ;
    mins = (ts < mins) ? ts : mins ;
  }
  l.i.maxs = maxs ;
  l.i.mins = mins ;
  l.i.mina = mina ;
  l.i.min0 = min0 ;
  l.i.maxa = INT32_maxa(l) ;
  l.i.allm = W32_ALLM(l) ;
  l.i.allp = W32_ALLP(l) ;
  return l ;
}

// get integer 32 extrema (signed and absolute value) "missing" values are accounted for
// f       [IN]  32 bit integer array
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
// return an extrema limits_w32 struct
//
// values with bit pattern (~mmask) & *spval wil be treated as "missing" and ignored
// if mmask == 0 or spval == NULL, the "missing" check is inactivated
limits_w32 INT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad)
{
  int32_t  *fs = (int32_t *) f ;
  int i ;
  uint32_t mina, min0, missf, tu, t0, *miss = (uint32_t *)spval ;
  int32_t maxs, mins, ts, good, *fill = (int32_t *)pad ;
  limits_w32 l ;

  mmask = ~mmask ;
  if(spval == NULL || mmask == 0) return INT32_extrema(f, np) ;

  mina = 0x7FFFFFFFu ;
  maxs = 0x80000000 ;
  mins = 0x7FFFFFFF ;
  min0 = 0xFFFFFFFFu ;
  missf = mmask & *miss ;                            // masked "missing" value
  if(fill == NULL){                                  // no explicit replacement value
    good = (uint32_t)fs[0] ;                         // in case no non "missing" value found
    for(i=0 ; i<np ; i++){                           // find first non "missing" value
      good = (uint32_t)fs[i] ;
      if( (good & mmask) != missf) break ;           // non "missing" value found
    }
  }else{
    good = *fill ;                                   // we have an explicit replacement value
  }
  // gcc seems to have problems vectorizing this loop if using t0   = (tu == 0) ? 0xFFFFFFFF : tu ;
  for(i=0 ; i<np ; i++){
    ts   = fs[i] ;
    ts   = (((uint32_t) ts & mmask) == missf) ? good : ts ;     // replace "missing" values with "non missing" value
    tu   = (ts < 0) ? -ts : ts ;                     // ABS(fs[i])
    t0   = (tu == 0) ? 0xFFFFFFFEu : tu ;             // ignore 0 for non zero minimum, replace with HUGE value
    mina = (tu < mina) ? tu : mina ;                 // smallest absolute value
    min0 = (t0 < min0) ? t0 : min0 ;                 // smallest non zero absolute value
    maxs = (ts > maxs) ? ts : maxs ;                 // signed maximum
    mins = (ts < mins) ? ts : mins ;                 // signed minimum
  }
  l.i.maxs = maxs ;
  l.i.mins = mins ;
  l.i.mina = mina ;
  l.i.min0 = min0 ;
  l.i.maxa = INT32_maxa(l) ;
  l.i.allm = W32_ALLM(l) ;
  l.i.allp = W32_ALLP(l) ;
  return l ;
}

// get IEEE 32 extrema (signed and absolute value)
// f     [IN]  32 bit IEEE float array
// np    [IN]  number of data items
// return an extrema limits_w32 struct
// N.B. this code only works for IEEE754 32 bit floats, all processing is done in INTEGER mode
limits_w32 IEEE32_extrema(void * restrict f, int np)
{
  uint32_t *fu = (uint32_t *) f ;
  int32_t *fs = (int32_t *) f ;
  int i ;
  uint32_t mina, min0, t0, tu ;
  int32_t maxs, mins, ts ;
  limits_w32 l ;

  mina = fu[0] & 0x7FFFFFFF ;                      // drop sign
  ts = (fs[0] & 0x7FFFFFFF) ^ (fs[0] >> 31) ;      // signed integer from float
  maxs = mins = ts ;
  min0 = 0x7FFFFFFF ;                              // huge positive value
  for(i=0 ; i<np ; i++){
    tu = fu[i] & 0x7FFFFFFF ;                      // drop sign
    t0 = (tu == 0) ? 0x7FFFFFFF : tu ;             // replace 0 with huge value for min()
    ts = (fs[i] & 0x7FFFFFFF) ^ (fs[i] >> 31) ;    // signed integer from float
    mina = (tu < mina) ? tu : mina ;               // unsigned min
    min0 = (t0 < min0) ? t0 : min0 ;               // unsigned min
    maxs = (ts > maxs) ? ts : maxs ;               // signed max
    mins = (ts < mins) ? ts : mins ;               // signed min
  }
  l.i.maxs = (maxs & 0x7FFFFFFF) ^ (maxs >> 31) ;  // maximum float value (signed)
  l.i.mins = (mins & 0x7FFFFFFF) ^ (mins >> 31) ;  // minimum float value (signed)
  l.i.mina = mina ;                                // smallest float absolute value
  l.i.min0 = min0 ;                                // smallest float non zero absolute value
  l.i.maxa = IEEE32_maxa(l) ;                      // largest float absolute value
  l.i.allm = W32_ALLM(l) ;
  l.i.allp = W32_ALLP(l) ;
  return l ;
}
// get minimum and maximum absolute values, set all <= 0 and all >= 0 flags
// f     [IN]  32 bit IEEE float array
// np    [IN]  number of data items
// return an extrema limits_w32 struct
// N.B. this code only works for IEEE754 32 bit floats, all processing is done in INTEGER mode
limits_w32 IEEE32_extrema_abs(void * restrict f, int np)
{
  uint32_t *fu = (uint32_t *) f ;
  int i ;
  uint32_t mina, maxa, ands, ors, to, ta ;
  limits_w32 l ;

  ands = ors  = fu[0] ;
  mina = maxa = fu[0] & 0x7FFFFFFF ;               // drop sign
  for(i=0 ; i<np ; i++){
    to = fu[i] ;
    ta = (to == 0) ? 0x80000000u : to ;            // +0 will not alter all negative condition
    ands |= ta ;                                   // running and, MSB remains 1 if negative or 0
    ors  |= to ;                                   // running or, MSB stays 0 if positive or 0
    to   &= 0x7FFFFFFF ;                           // drop sign
    mina = (to < mina) ? to : mina ;               // unsigned minimum absolute value
    maxa = (to > maxa) ? to : maxa ;               // unsigned maximum absolute value
  }
  l.f.mins = 1.0f ;                                // dummy (1.0)
  l.f.maxs = -1.0f ;                               // dummy (-1.0)
  l.i.mina = mina ;                                // smallest float absolute value
  l.i.min0 = 0 ;                                   // dummy
  l.i.maxa = maxa ;                                // smallest float non zero absolute value
  l.i.allm = ands >> 31 ;                          // 1 if all values negative or 0
  l.i.allp = (ors >> 31) == 0 ;                    // 1 if all values positive or 0
  return l ;
}

// get IEEE 32 extrema (signed and absolute value), "missing" values are accounted for
// f       [IN]  32 bit IEEE float array
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
// return an extrema limits_w32 struct
//
// values with bit pattern (~mmask) & *spval wil be treated as "missing" and ignored
// if mmask == 0 or spval == NULL, the "missing" check is inactivated
// N.B. this code only works for IEEE754 32 bit floats, all processing is done in INTEGER mode
limits_w32 IEEE32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad)
{
  uint32_t *fu = (uint32_t *) f ;
  int i ;
  uint32_t mina, min0, tu, t0, good, missf, *miss = (uint32_t *)spval, *fill = (uint32_t *)pad  ;
  int32_t maxs, mins, ts ;
  limits_w32 l ;

  mmask = ~mmask ;
  if(spval == NULL || mmask == 0) return IEEE32_extrema(f, np) ;   // "missing" option not used

  mins = mina = min0 = 0x7FFFFFFFu ;                 // largest positive value
  maxs = 1 << 31 ;                                   // largest negative value
  missf = mmask & *miss ;                            // masked "missing" value
  if(fill == NULL){                                  // no explicit replacement value
    good = fu[0] ;                                   // in case no non "missing" value found
    for(i=0 ; i<np ; i++){                           // find first non "missing" value
      good = fu[i] ;
      if( (good & mmask) != missf) break ;           // non "missing" value found
    }
  }else{
    good = *fill ;                                   // we have an explicit replacement value
  }
  for(i=0 ; i<np ; i++){
    tu = fu[i] ;
    tu = (tu & mmask) == missf ? good : tu ;         // replace "missing" value with "good" value
    t0 = (tu == 0) ? 0x7FFFFFFF : tu ;               // if tu == 0, set t0 to largest value to compute min0
    ts = (tu & 0x7FFFFFFF) ^ ((int32_t)tu >> 31) ;   // fake signed integer from float
    tu = tu & 0x7FFFFFFF ;                           // ABS(tu)
    mina = (tu < mina) ? tu : mina ;                 // smallest absolute value
    min0 = (t0 < min0) ? t0 : min0 ;                 // smallest non zero absolute value
    maxs = (ts > maxs) ? ts : maxs ;                 // maximum signed value
    mins = (ts < mins) ? ts : mins ;                 // minimum signed value
  }
  l.i.maxs = (maxs & 0x7FFFFFFF) ^ (maxs >> 31) ;  // restore float from fake signed integer
  l.i.mins = (mins & 0x7FFFFFFF) ^ (mins >> 31) ;  // restore float from fake signed integer
  l.i.mina = mina ;                                // no restore needed, mina/min0 are >= 0
  l.i.min0 = min0 ;
//   maxs = l32.i.maxs & 0x7FFFFFFF ;                   // ABS(maxs)
//   mins = l32.i.mins & 0x7FFFFFFF ;                   // ABS(mins)
//   l32.i.maxa = (maxs > mins) ? maxs : mins ;         // MAX(ABS(maxs), ABS(mins))
  l.i.maxa = IEEE32_maxa(l) ;                      // largest float absolute value
  l.i.allm = W32_ALLM(l) ;
  l.i.allp = W32_ALLP(l) ;
  return l ;                                       // return union
}
