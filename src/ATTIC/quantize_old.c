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
// ========================================== fake log quantization type 0 ==========================================

// restore floating point numbers quantized with IEEE32_fakelog_quantize_0
// q     [IN]  32 bit integer array containing the quantized data
// f    [OUT]  32 bit IEEE float array that will receive restored floats
// ni    [IN]  number of data items (used for checking purposes only)
// desc  [IN]  information describing quantization (from IEEE32_fakelog_quantize_0)
// N.B. if values have mixed signs, the sign is stored as the LSB in the quantized integer
//
// N.B. fake log quantizer 1 is much faster than fake log quantizer 0
//      fake log quantizer 0 does not behave well in cyclical test
//      for now, this version is kept for reference
q_desc  IEEE32_fakelog_restore_0(void * restrict f, int ni, q_desc desc, void * restrict q){
  int32_t *qi = (uint32_t *) q ;
  float *fo = (float *) f ;
  q_desc q64 = {.u = 0 } ;  // invalid restored state
//   ieee32_props h64 ;
  int i, nbits ;
  int32_t emax, emin, exp ;
//   int32_t elow ;
  uint32_t allp, allm, pos_neg, sign ;
  int32_t ti ;
  AnyType32 t, x ;
  float fac, qrange, q0 ;

  if(desc.q.type  != Q_FAKE_LOG_0) goto error ;  // wrong quantizer
  if(desc.q.state != QUANTIZED)    goto error ;  // must be quantized data

  q64.u = desc.u ;
  emax = desc.q.offset.emax - 127 ;
  emin = desc.q.emin - 127 ;
//   elow = desc.q.offset.elow ;
  nbits = desc.q.nbits ;
  allp = desc.q.allp ;
  allm = desc.q.allm ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values
  sign = (allm << 31) ;                   // move sign of all values to MSB position
// fprintf(stderr, "allm = %d, allp = %d, pos_neg = %d\n", allm, allp, pos_neg) ;

  x.u = ((emin + 127) << 23) ;
  x.u |= sign ;                           // restore sign (if all numbers are negative)
//   q0 = (elow == 0) ? 0.0f : x.f ;
  q0 = (desc.q.clip == 1) ? 0.0f : x.f ;

  qrange = (1 << (nbits - pos_neg)) ;         // range of quantized values (2**nbits)
  t.f = (emax - emin + 1) ;       // maximum possible value of "fake log"
  fac = t.f / qrange ;      // factor to quantize "fake log"
// fprintf(stderr,"nbits = %d, emax-emin+1 = %d, fac = %f, emin = %d, emax = %d, elow = %d, q0 = %f\n", nbits, emax-emin+1, fac, emin, emax, elow, q0) ;
// restore values using: t.f, fac, sign, pos_neg, 
//                       emax, emin, nbits, allm, allp
  if(pos_neg){                 // mix of positive and negative numbers
    emin += 126 ;
    i = 0 ;
// N.B. clang(and llvm based compilers) seem to vectorize the code and not need explicit AVX2 code
#if defined(__AVX2__) && ! defined(__clang__) && ! defined(__PGIC__)
    __m256i vti, vz0, vc1, vm0, vex, vem, vfo, vmk, vsg, vmg ;
    __m256 vtf, vf1, vfa ;
    vz0 = _mm256_xor_si256(vz0, vz0) ;                    // 0
    vm0 = _mm256_cmpeq_epi32(vz0, vz0) ;                  // -1
    vc1 = _mm256_sub_epi32(vz0, vm0) ;                    // 0 - -1 = 1
    vem = _mm256_set1_epi32(emin) ;                       // emin
    vf1 = _mm256_set1_ps(1.0f) ;                          // 1.0
    vfa = _mm256_set1_ps(fac) ;                           // fac
    vmk = _mm256_set1_epi32(0x7FFFFF) ;                   // mask for mantissa
    vmg = _mm256_set1_epi32(1 << 31) ;                    // mask for sign
    for( ; i<ni-7 ; i+=8){
      vti = _mm256_loadu_si256((__m256i *)(qi + i)) ;
      vsg = _mm256_and_si256(vti, vmg) ;                  // -1 if vti < 0, 0 otherwise
      vti = _mm256_abs_epi32(vti) ;                       // |vti|
      vtf = _mm256_cvtepi32_ps( _mm256_add_epi32(vti, vm0) ) ;   // float(vti - 1)
      vtf = _mm256_fmadd_ps(vtf, vfa, vf1) ;               // vtf * fac + 1.0
      vex = _mm256_cvttps_epi32(vtf) ;                     // int(above)
      vtf = _mm256_sub_ps(vtf, _mm256_cvtepi32_ps( _mm256_add_epi32(vex, vm0) ) ) ;  // vtf = vtf - float(exp -1)
      vex = _mm256_slli_epi32( _mm256_add_epi32(vex, vem) , 23) ;                    // exp = (exp + emin) << 23
      vfo = _mm256_and_si256((__m256i)vtf, vmk) ;          // restore mantissa
      vfo = _mm256_or_si256(vfo, vex) ;                    // restore exponent
      vfo = _mm256_or_si256(vfo, vsg) ;                    // restore sign
      _mm256_storeu_ps(fo + i, (__m256)vfo) ;
    }
#endif
    for( ; i<ni ; i++){
      ti  = qi[i] ;
      sign = ti & 0x80000000u ;
      ti = (ti < 0) ? -ti : ti ;  // remove sign
      t.f = fac * (ti - 1) + 1.0f ;
      exp = t.f ;              // exponent
      t.f = t.f - (exp - 1) ;
      exp += emin ;
      exp <<= 23 ;
      x.u = t.u & 0x7FFFFF ;   // restore mantissa
      x.u |= exp ;             // restore exponent
      x.u |= sign ;            // restore sign
      fo[i] = (ti == 0) ? q0 : x.f ;
    }
  }else{                       // all numbers have the ame sign
    emin += 126 ;
    i = 0 ;
// N.B. clang(and llvm based compilers) seem to vectorize the code and not need explicit AVX2 code
#if defined(__AVX2__) && ! defined(__clang__) && ! defined(__PGIC__)
    __m256i vti, vz0, vc1, vm0, vex, vem, vfo, vmk, vsg ;
    __m256 vtf, vf1, vfa ;
    vz0 = _mm256_xor_si256(vz0, vz0) ;                    // 0
    vm0 = _mm256_cmpeq_epi32(vz0, vz0) ;                  // -1
    vc1 = _mm256_sub_epi32(vz0, vm0) ;                    // 0 - -1 = 1
    vem = _mm256_set1_epi32(emin) ;                       // emin
    vf1 = _mm256_set1_ps(1.0f) ;                          // 1.0
    vfa = _mm256_set1_ps(fac) ;                           // fac
    vmk = _mm256_set1_epi32(0x7FFFFF) ;                   // mask for mantissa
    vsg = _mm256_set1_epi32(sign) ;                       // sign
    for( ; i<ni-7 ; i+=8){
      vti = _mm256_loadu_si256((__m256i *)(qi + i)) ;
      vtf = _mm256_cvtepi32_ps( _mm256_add_epi32(vti, vm0) ) ;   // float(vti - 1)
      vtf = _mm256_fmadd_ps(vtf, vfa, vf1) ;               // vtf * fac + 1.0
      vex = _mm256_cvttps_epi32(vtf) ;                     // int(above)
      vtf = _mm256_sub_ps(vtf, _mm256_cvtepi32_ps( _mm256_add_epi32(vex, vm0) ) ) ;  // vtf = vtf - float(exp -1)
      vex = _mm256_slli_epi32( _mm256_add_epi32(vex, vem) , 23) ;                    // exp = (exp + emin) << 23
      vfo = _mm256_and_si256((__m256i)vtf, vmk) ;          // restore mantissa
      vfo = _mm256_or_si256(vfo, vex) ;                    // restore exponent
      vfo = _mm256_or_si256(vfo, vsg) ;                    // restore sign
      _mm256_storeu_ps(fo + i, (__m256)vfo) ;
    }
#endif
    for( ; i<ni ; i++){
      ti  = qi[i] ;
      t.f = fac * (ti - 1) + 1.0f ;
      exp = t.f ;              // exponent
      t.f = t.f - (exp - 1) ;
      exp += emin ;
      exp <<= 23 ;
      x.u = t.u & 0x7FFFFF ;   // restore mantissa
      x.u |= exp ;             // restore exponent
      x.u |= sign ;            // restore sign
      fo[i] = (ti == 0) ? q0 : x.f ;
    }
  }
  q64.u = desc.u ;
  q64.r.npts = ni ;
  q64.r.state = RESTORED ;
  return q64 ;

error:
  q64.u = 0 ;
  exit(1) ;
  return q64 ;
}

// quantize IEEE floats
// q    [OUT]  32 bit integer array that will receive the quantized result
// f     [IN]  32 bit IEEE float array, data to be quantized
// ni    [IN]  number of data items to quantize
// rule  [IN]  quantization rules
// N.B. if there are mixed signs, the sign is stored in the LSB position
//
// N.B. fake log quantizer 1 is much faster than fake log quantizer 0
//      fake log quantizer 0 does not behave well in cyclical test
q_desc IEEE32_fakelog_quantize_0(void * restrict f, int ni, q_desc rule, void * restrict q, limits_w32 *limits){
  float *fu = (float *) f ;
  uint32_t *qo = (uint32_t *) q ;
  q_desc q64 = {.u = 0 } ;  // set invalid output state
  uint32_t allp, allm, pos_neg ;
  uint32_t maxa, mina, sign ;
  int32_t emax, emin, ebits, erange ;
//   int32_t elow, maskbits ;
  AnyType32 z0 ;
  float fac, qrange ;   // quantization factor
  int i ;
  int nbits = rule.f.nbits, mbits = rule.f.mbits ;
  int rng10 = rule.f.rng10, rng2 = rule.f.rng2 ;
  float qzero = rule.f.ref ;
  limits_w32 l32, *l32p = &l32 ;
int debug = 0 ;

  if(rule.f.state != TO_QUANTIZE)            goto error ;       // invalid state
  if(rule.f.nbits == 0 && rule.f.mbits == 0) goto error ;       // cannot be both set to 0

  if(limits == NULL){
    l32 = IEEE32_extrema(fu, ni) ;
  }else{
    l32p = limits ;
  }
  allm = l32p->i.allm ;                   // extract all useful values from u64
  allp = l32p->i.allp ;
  maxa = l32p->i.maxa ;
  mina = l32p->i.mina ;
  pos_neg = (allp || allm) ? 0 : 1 ;      // we have both positive and negative values

// fill result struct
  q64.u       = 0 ;                       // set all fields to 0
  q64.q.allp  = allp ;                    // all values >= 0 flag
  q64.q.allm  = allm ;                    // all values  < 0 flag

  qzero = rule.f.ref ;
  z0.f  = (qzero < 0) ? -qzero : qzero ;  // ABS(smallest significant value)
  qzero = z0.f ;
  if(rng10 > 0) rng2 = rng10 * 3.33334f ; // 2**10 ~= 10**3
  if(rng2 > 0){
    AnyType32 z1 = { .u = 0 } ;
    int mina2 = maxa - (rng2 << 23) ;
    if(mina2 > 0) z1.u = mina2 ;
    if(z1.u > z0.u) z0.u = z1.u ;
    fprintf(stderr, "z1 = %f, maxa = %8.8x, mina2 = %8.8x, rng2 = %d\n", z1.f, maxa, mina2, rng2) ;
  }
if(debug) fprintf(stderr, "z0 = %f, rng2 = %d, rng10 = %d, clip = %d, maxa = %8.8x, mina = %8.8x, allp = %d, allm = %d\n",
                           z0.f, rule.f.rng2, rule.f.rng10, rule.f.clip, maxa, mina, allp, allm);
  emax = (maxa >> 23) & 0xFF ;
  if(mina < z0.u){                         // if mina < lowest significant absolute value
    mina = z0.u ;                          // set it to that value
    q64.q.clip = rule.f.clip ;            // will force a restore to zero if quantized value == 0
  }
  emin = (mina >> 23) & 0xFF ;
  erange = emax - emin ;                  // exponent range
  ebits = 32 - lzcnt_32(erange) ;         // number of bits needed to encode exponent range (0 -> 8)

// determine / adjust mbits and nbits
  int nbits0 = nbits ;                    // original value
  int mbits0 = mbits ;                    // original value
  mbits = (mbits > 22) ? 22 : mbits ;     // make sure mbits <= 22
  if(nbits > 0 && nbits < mbits+pos_neg)  // a smaller value for nbits cannot make sense
    nbits = mbits + pos_neg ;

  if(mbits == 0){                         // nbits > 0, mbits == 0
    mbits = nbits - ebits - pos_neg ;     // left after removing sign (if necessary) and exponent bits
    mbits = (mbits > 22) ? 22 : mbits ;   // make sure mbits <= 22
    mbits = (mbits <  1) ?  1 : mbits ;   // make sure mbits > 0
if(debug) fprintf(stderr, "(mbits == 0)\n") ;

  }else if(nbits == 0){                   // mbits > 0, nbits == 0
    mbits = (mbits > 22) ? 22 : mbits ;   // make sure mbits <= 22
if(debug) fprintf(stderr, "(nbits == 0)") ;

  }else{                                  // mbits > 0, nbits > 0
    mbits = (mbits > 22) ? 22 : mbits ;   // make sure mbits <= 22
    while(nbits < ebits + mbits + pos_neg){
      nbits++ ;                           // increase nbits
      if(nbits >= ebits + mbits + pos_neg) break ;
      if(mbits > 1) mbits-- ;             // decrease ebits
      if(nbits >= ebits + mbits + pos_neg) break ;
      nbits++ ;                           // increase nbits
    }
if(debug) fprintf(stderr, "(nbits,mbits)") ;
  }
  nbits = mbits + ebits + pos_neg ;       // (re)compute nbits
if(debug) fprintf(stderr, " nbits = %d(%d), mbits = %d(%d), ebits = %d, pos_neg = %d\n", nbits, nbits0, mbits, mbits0, ebits, pos_neg) ;
  q64.q.nbits = nbits ;
  q64.q.mbits = mbits ;

  q64.q.offset.emax = emax ;
  q64.q.emin = emin ;                    // store adjusted value of emin
  emax -= 127 ;
  emin -= 127 ;

  qrange = (1 << (nbits - pos_neg)) ;         // range of quantized values (2**nbits)
  z0.f = (emax - emin + 1) ;       // maximum possible value of "fake log"
  fac = z0.f = qrange / z0.f ;      // factor to quantize "fake log"
//   maskbits = RMASK31(nbits) ;     // 2**nbits -1, largest allowable quantized value
if(debug) fprintf(stderr, "fac = %f\n", fac) ;
  q64.q.type  = Q_FAKE_LOG_0 ;            // identify quantizing algorithm
  q64.q.state = QUANTIZED ;               // everything O.K.

// quantize values using: fac(nbits,emax,emin), emin, pos_neg(allm,allp)
// restore will need:     emax, emin, nbits, allm, allp
  if(pos_neg){
    emin += 128 ;                   // combining with (exp - 127 - 1)
    i = 0 ;
// N.B. the AVX2 version does not provide a large speed increase with clang(and llvm based compilers)
#if defined(__AVX2__)
    __m256i vfu, vz0, vc1, vm0, vm1, vqo, vex, vma, vme, vmi, vsg ;
    __m256 v15, vfa ;
    vz0 = _mm256_xor_si256(vz0, vz0) ;                    // 0
    vm0 = _mm256_cmpeq_epi32(vz0, vz0) ;                  // -1
    vc1 = _mm256_sub_epi32(vz0, vm0) ;                    // 0 - -1 = 1
    vm0 = _mm256_srli_epi32(vm0, 1) ;                     // mask for sign 0x7FFFFFFF
    vm1 = _mm256_srli_epi32(vm0, 8) ;                     // mask for mantissa or exponent 0x007FFFFF
    vme = _mm256_set1_epi32(127 << 23) ;                  // 127 << 23  0x3F800000
    vmi = _mm256_set1_epi32(emin) ;
    v15 = _mm256_set1_ps(1.5f) ;
    vfa =  _mm256_set1_ps(fac) ;
    for( ; i<ni-7 ; i+=8){
      vfu = _mm256_loadu_si256((__m256i *)(fu + i)) ;
      vsg = _mm256_srai_epi32(vfu, 31) ;                  // sign, -1 if fu < 0, 0 if fu >= 0
      vfu = _mm256_and_si256(vfu, vm0) ;                  // |vfu|
      vex = _mm256_andnot_si256(vm1, vfu) ;               // exponent << 23
      vex = _mm256_srli_epi32(vex, 23) ;
      vex = _mm256_sub_epi32(vex, vmi) ;                  // exponent - emin
      vex = (__m256i) _mm256_cvtepi32_ps(vex) ;           // convert to float
      vma = _mm256_and_si256(vfu, vm1) ;                  // mantissa
      vma = _mm256_or_si256(vma, vme) ;                   // mantissa + 127 as exponent (1.0 <= vma < 2.0)
      vex = (__m256i)_mm256_add_ps((__m256)vex, (__m256) vma) ;  // fake log = exponent + vma - 1 (0.0 <= vex < exponent + 1)
      vex = (__m256i)_mm256_fmadd_ps((__m256)vex, vfa, v15) ;    // fake_log * fac + 1.5
      vma = _mm256_srai_epi32((__m256i)vex, 31) ;         // propagate sign to all bits
      vqo = _mm256_cvttps_epi32((__m256)vex) ;            // truncate to integer
      vqo = _mm256_max_epi32(vqo, vc1) ;                  // max(1, vqo)
      vqo = _mm256_andnot_si256(vma, vqo) ;               // zero where vma == -1
      vqo = _mm256_xor_si256(vqo, vsg) ;                  // restore sign = vqo ^ sign - sign (2's complement negate)
      vqo = _mm256_sub_epi32(vqo, vsg) ;
      _mm256_storeu_si256((__m256i *)(qo + i), vqo) ;
    }
#endif
    for(  ; i<ni ; i++){
      int exp ;
      AnyType32 x ;
      x.f = fu[i] ;                 // float to quantize
      sign = (x.f <  0.0f) ? 1 : 0 ;
      exp = (x.u >> 23) & 0xFF ;    // exponent from value (with +127 bias)
      exp = exp - emin ;            // this will have a value of 127 for the lowest "significant" exponent
      x.u &= 0x7FFFFF ;             // get rid of exponent and sign, only keep mantissa
      x.u |= (0x7F << 23) ;         // force 127 as exponent (1.0 <= x.f < 2.0)
      x.f += exp ;                  // add value of exponent from original float (0.0 <= x.f < max "fake log")
      qo[i] = x.f * fac + 1.5f ;     // quantize the fake log
      qo[i] = (x.f <  0.0f) ? 0 : qo[i] ;               // < qzero (exp < emin)
      qo[i] = (x.f == 0.0f) ? 1 : qo[i] ;               // == lowest significant value (emin)
      qo[i] = (sign == 0) ? qo[i] : -qo[i] ;   // restore sign
    }
  }else{                            // all values have the same sign
    emin += 128 ;                   // combining with (exp - 127 - 1)
    i = 0 ;
// N.B. the AVX2 version does not provide a large speed increase with clang(and llvm based compilers)
#if defined(__AVX2__)
    __m256i vfu, vz0, vc1, vm0, vm1, vqo, vex, vma, vme, vmi ;
    __m256 v15, vfa ;
    vz0 = _mm256_xor_si256(vz0, vz0) ;                    // 0
    vm0 = _mm256_cmpeq_epi32(vz0, vz0) ;                  // -1
    vc1 = _mm256_sub_epi32(vz0, vm0) ;                    // 0 - -1 = 1
    vm0 = _mm256_srli_epi32(vm0, 1) ;                     // mask for sign 0x7FFFFFFF
    vm1 = _mm256_srli_epi32(vm0, 8) ;                     // mask for mantissa or exponent 0x007FFFFF
    vme = _mm256_set1_epi32(127 << 23) ;                  // 127 << 23  0x3F800000
    vmi = _mm256_set1_epi32(emin) ;                       // emin
    v15 = _mm256_set1_ps(1.5f) ;                          // constant 1.5
    vfa =  _mm256_set1_ps(fac) ;                          // quantization factor
    for( ; i<ni-7 ; i+=8){
      vfu = _mm256_loadu_si256((__m256i *)(fu + i)) ;
      vfu = _mm256_and_si256(vfu, vm0) ;                  // |vfu|
      vex = _mm256_andnot_si256(vm1, vfu) ;               // exponent << 23
      vex = _mm256_srli_epi32(vex, 23) ;
      vex = _mm256_sub_epi32(vex, vmi) ;                  // exponent - emin
      vex = (__m256i) _mm256_cvtepi32_ps(vex) ;           // convert to float
      vma = _mm256_and_si256(vfu, vm1) ;                  // mantissa
      vma = _mm256_or_si256(vma, vme) ;                   // mantissa + 127 as exponent
      vex = (__m256i)_mm256_add_ps((__m256)vex, (__m256) vma) ;  // fake log
      vex = (__m256i)_mm256_fmadd_ps((__m256)vex, vfa, v15) ;   // fake_log * fac + 1.5
      vma = _mm256_srai_epi32((__m256i)vex, 31) ;         // propagate sign to all bits
      vqo = _mm256_cvttps_epi32((__m256)vex) ;            // truncate to integer
      vqo = _mm256_max_epi32(vqo, vc1) ;                  // max(1, vqo)
      vqo = _mm256_andnot_si256(vma, vqo) ;               // zero where vma == -1
      _mm256_storeu_si256((__m256i *)(qo + i), vqo) ;
    }
#endif
    for( ; i<ni ; i++){
      int exp ;
      AnyType32 x ;
      x.f = fu[i] ;                 // float to quantize
      exp = (x.u >> 23) & 0xFF ;    // exponent from value (with +127 bias)
      exp = exp - emin ;            // this will have a value of 127 for the lowest "significant" exponent
      x.u &= 0x7FFFFF ;             // get rid of exponent and sign, only keep mantissa
      x.u |= (0x7F << 23) ;         // force 127 as exponent (1.0 <= x.f < 2.0)
//       x.f += (exp - 127 - 1) ;
      x.f += exp ;                  // add value of exponent from original float (0.0 <= x.f < max "fake log")
      qo[i] = x.f * fac + 1.5f ;     // quantize the fake log
      qo[i] = (x.i == 0) ? 1 : qo[i] ;               // == lowest significant value (emin)
      qo[i] = (x.i <  0) ? 0 : qo[i] ;               // < qzero (exp < emin)
//       qo[i] = (qo[i] > maskbits) ? maskbits : qo[i] ;   // clamp at 2**nbits -1
    }
  }
  return q64 ;

error:
  q64.u = 0 ;
  exit(1) ;
  return q64 ;
}
#if defined(RETIRED_CODE)
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
#endif
#if defined(RETIRED_CODE)
// prepare for linear quantization of IEEE floats (type 1)
// handles mixed signs well, handles medium exponent range well
// l32    [IN]  analysis from IEEE32_extrema
// np     [IN]  number of data items to quantize
// nbits  [IN]  number of bits to use for quantized result (0 means use errmax to compute it)
// errmax [IN]  maximum allowed error (if non zero, it is used to compute nbits)
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
  q.f   = errmax < 0.0f ? 0.0f : errmax * 2.0f  ;   // quantum is twice the maximum error
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
    if(pos_neg) nbits2-- ;           // need 1 bit for sign, one less is available for quantized result
    nbits2 = (nbits2<1) ? 1 : nbits2 ;  // minimum = 1 bit
    q.u = (m3.u >> 23) - nbits2 + 1 ;   // adjust quantum exponent to reflect nbits2(nbits)
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

  if(nbits <= 0){                    // should already have been done
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
#endif
#if defined(RETIRED_CODE)
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

  // deal with errmax == 0 and nbits == 0 (cannot both be 0)
  if(nbits <= 0) nbits = 15 ;             // default to 15 bits quantization
  if(errmax > 0) nbits = 0 ;              // will use errmax(quant) to compute nbits
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
#endif

