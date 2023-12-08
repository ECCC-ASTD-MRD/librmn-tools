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
static inline float 
FastLog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f
           - 1.498030302f * mx.f 
           - 1.72587999f / (0.3520887068f + mx.f);
}

static inline float
FastLog (float x)
{
  return 0.69314718f * FastLog2 (x);
}

static inline float 
FasterLog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
}

static inline float
FasterLog (float x)
{
//  return 0.69314718f * FasterLog2 (x);

  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 8.2629582881927490e-8f;
  return y - 87.989971088f;
}

#if defined(__AVX2__)

// constant vectors used by 128 bit (SSE2|3|4) and 256 bit (AVX2) functions
static vm8f c_124_22551499 = { v8sfl (124.22551499f) };
static vm8f c_1_498030302  = { v8sfl (1.498030302f)  };
static vm8f c_1_725877999  = { v8sfl (1.72587999f)   };
static vm8f c_0_3520087068 = { v8sfl (0.3520887068f) };

static inline v4sf V4FastLog2 (v4sf x)
{
  union { v4sf f; v4si i; } vx = { x };
  union { v4si i; v4sf f; } mx; mx.i = (vx.i & v4sil (0x007FFFFF)) | v4sil (0x3f000000);
  v4sf y = v4si_to_v4sf (vx.i);
  y *= v4sfl (1.1920928955078125e-7f);
  return y - c_124_22551499.v128[0]
           - c_1_498030302.v128[0] * mx.f 
           - c_1_725877999.v128[0] / (c_0_3520087068.v128[0] + mx.f);
}

static inline v8sf V8FastLog2 (v8sf x)
{
  union { v8sf f; v8si i; } vx = { x };
  union { v8si i; v8sf f; } mx; mx.i = (vx.i & v8sil(0x007FFFFF)) | v8sil(0x3f000000);
  v8sf y = v8si_to_v8sf (vx.i);
  y *= v8sfl (1.1920928955078125e-7f);
  return y - c_124_22551499.v256
           - c_1_498030302.v256 * mx.f 
           - c_1_725877999.v256 / (c_0_3520087068.v256 + mx.f);
}

static inline v4sf V4FastLog (v4sf x)
{
  const v4sf c_0_69314718 = v4sfl (0.69314718f);
  return c_0_69314718 * V4FastLog2 (x);
}

static inline v8sf V8FastLog (v8sf x)
{
  const v8sf c_0_69314718 = v8sfl (0.69314718f);
  return c_0_69314718 * V8FastLog2 (x);
}

#endif   // __AVX2__

static inline float
FastPow2 (float p)
{
  float offset = (p < 0) ? 1.0f : 0.0f;
  float clipp = (p < -126) ? -126.0f : p;
  int w = clipp;
  float z = clipp - w + offset;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z) ) };

  return v.f;
}

static inline float
FastExp (float p)
{
  return FastPow2 (1.442695040f * p);
}

static inline float
FasterPow2 (float p)
{
  float clipp = (p < -126) ? -126.0f : p;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) * (clipp + 126.94269504f) ) };
  return v.f;
}

static inline float
FasterExp (float p)
{
  return FasterPow2 (1.442695040f * p);
}

#if defined(__AVX2__)

#endif   // __AVX2__

#endif    //   __FAST_APPROX_H_
