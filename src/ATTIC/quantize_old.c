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

