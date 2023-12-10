//
// scalar and SIMD fast exponential / log functions
//
// inspired by / borrowed from code by Paul Mineiro
// original license from said code below
/*
 *=====================================================================*
 *                   Copyright (C) 2011 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>                            *
 *=====================================================================*/

/*
 * function names have been changed
 * vector constants have been implemented differently
 * not all functions from original code are present
 * AVX2 functions have been added by M.Valin Environment Canada 2023
*/

#ifndef __FAST_APPROX_H_
#define __FAST_APPROX_H_

#include <rmn/x86-simd.h>
#include <stdint.h>
static inline float FastLog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f
           - 1.498030302f * mx.f 
           - 1.72587999f / (0.3520887068f + mx.f);
}

static inline float FastLog (float x)
{
  return 0.69314718f * FastLog2 (x);
}

static inline float  FasterLog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
}

static inline float FasterLog (float x)
{
//  return 0.69314718f * FasterLog2 (x);

  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 8.2629582881927490e-8f;
  return y - 87.989971088f;
}

#if defined(__AVX2__)

// constant vectors used by 128 bit (SSE2|3|4) and 256 bit (AVX2) functions
static v48mf c_124_22551499 = { v8sfl1 (124.22551499f) };
static v48mf c_1_498030302  = { v8sfl1 (1.498030302f)  };
static v48mf c_1_725877999  = { v8sfl1 (1.72587999f)   };
static v48mf c_0_3520087068 = { v8sfl1 (0.3520887068f) };

static inline v4sf V4FastLog2 (v4sf x)
{
  union { v4sf f; v4si i; } vx = { x };
  union { v4si i; v4sf f; } mx; mx.i = (vx.i & v4sil1 (0x007FFFFF)) | v4sil1 (0x3f000000);
  v4sf y = v4si_to_v4sf (vx.i);
  y *= v4sfl1 (1.1920928955078125e-7f);
  return y - c_124_22551499.v128[0]
           - c_1_498030302.v128[0] * mx.f 
           - c_1_725877999.v128[0] / (c_0_3520087068.v128[0] + mx.f);
}

static inline v8sf V8FastLog2 (v8sf x)
{
  union { v8sf f; v8si i; } vx = { x };
  union { v8si i; v8sf f; } mx; mx.i = (vx.i & v8sil1(0x007FFFFF)) | v8sil1(0x3f000000);
  v8sf y = v8si_to_v8sf (vx.i);
  y *= v8sfl1 (1.1920928955078125e-7f);
  return y - c_124_22551499.v256
           - c_1_498030302.v256 * mx.f 
           - c_1_725877999.v256 / (c_0_3520087068.v256 + mx.f);
}

static inline v4sf V4FastLog (v4sf x)
{
  const v4sf c_0_69314718 = v4sfl1 (0.69314718f);
  return c_0_69314718 * V4FastLog2 (x);
}

static inline v8sf V8FastLog (v8sf x)
{
  const v8sf c_0_69314718 = v8sfl1 (0.69314718f);
  return c_0_69314718 * V8FastLog2 (x);
}

#endif   // __AVX2__

static inline float FastPow2 (float p)
{
  float offset = (p < 0) ? 1.0f : 0.0f;
  float clipp = (p < -126) ? -126.0f : p;
  int w = clipp;
  float z = clipp - w + offset;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z) ) };

  return v.f;
}

static inline float FastExp (float p)
{
  return FastPow2 (1.442695040f * p);
}

static inline float FasterPow2 (float p)
{
  float clipp = (p < -126) ? -126.0f : p;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) * (clipp + 126.94269504f) ) };
  return v.f;
}

static inline float FasterExp (float p)
{
  return FasterPow2 (1.442695040f * p);
}

#if defined(__AVX2__)

#define _mm256_cmplt_ps(a, b) _mm256_cmp_ps(a, b, _CMP_LT_OQ)

static v48mf c_121_2740838 = { v8sfl1 (121.2740575f) } ;
static v48mf c_27_7280233  = { v8sfl1 (27.7280233f)  } ;
static v48mf c_4_84252568  = { v8sfl1 (4.84252568f)  } ;
static v48mf c_1_49012907  = { v8sfl1 (1.49012907f)  } ;

static inline v4sf vfastpow2 (const v4sf p)
{
  v4sf ltzero = _mm_cmplt_ps (p, v4sfl1 (0.0f));
  v4sf offset = _mm_and_ps (ltzero, v4sfl1 (1.0f));
  v4sf lt126  = _mm_cmplt_ps (p, v4sfl1 (-126.0f));
  v4sf clipp  = _mm_or_ps (_mm_andnot_ps (lt126, p), _mm_and_ps (lt126, v4sfl1 (-126.0f)));
  v4si w = v4sf_to_v4si (clipp);
  v4sf z = clipp - v4si_to_v4sf (w) + offset;

  union { v4si i; v4sf f; } v = {
    v4sf_to_v4si (
      v4sfl1 (1 << 23) * 
      (clipp + c_121_2740838.v128[0] + c_27_7280233.v128[0] / (c_4_84252568.v128[0] - z) - c_1_49012907.v128[0] * z)
    )
  };

  return v.f;
}

static inline v4sf vfastexp (const v4sf p)
{
  const v4sf c_invlog_2 = v4sfl1 (1.442695040f);

  return vfastpow2 (c_invlog_2 * p);
}

static inline v4sf vfasterpow2 (const v4sf p)
{
  const v4sf c_126_94269504 = v4sfl1 (126.94269504f);
  v4sf lt126 = _mm_cmplt_ps (p, v4sfl1 (-126.0f));
  v4sf clipp = _mm_or_ps (_mm_andnot_ps (lt126, p), _mm_and_ps (lt126, v4sfl1 (-126.0f)));
  union { v4si i; v4sf f; } v = { v4sf_to_v4si (v4sfl1 (1 << 23) * (clipp + c_126_94269504)) };
  return v.f;
}

static inline v4sf vfasterexp (const v4sf p)
{
  const v4sf c_invlog_2 = v4sfl1 (1.442695040f);

  return vfasterpow2 (c_invlog_2 * p);
}

static inline v8sf V8FasterPow2 (const v8sf p)
{
  const v8sf c_126_94269504 = v8sfl1 (126.94269504f);
  v8sf lt126 = _mm256_cmplt_ps (p, v8sfl1 (-126.0f));
  v8sf clipp = _mm256_or_ps (_mm256_andnot_ps (lt126, p), _mm256_and_ps (lt126, v8sfl1 (-126.0f)));
  union { v8si i; v8sf f; } v = { v8sf_to_v8si (v8sfl1 (1 << 23) * (clipp + c_126_94269504)) };
  return v.f;
}

#endif   // __AVX2__

#endif    //   __FAST_APPROX_H_

