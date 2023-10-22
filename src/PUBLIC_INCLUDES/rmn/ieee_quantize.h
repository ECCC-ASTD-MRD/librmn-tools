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
#if ! defined(IEEE_QUANTIZE_INCLUDES)
#define IEEE_QUANTIZE_INCLUDES

#include <stdint.h>
#include <rmn/data_info.h>
#include <rmn/ct_assert.h>

// full quantization header
typedef struct{
  int32_t e0 ;       // reference exponent (used at restore time) (ieee quantization)
                     // true exponent of largest absolute value
  int32_t nbits ;    // maximum number of bits retained in quantized token
  int32_t nexp ;     // number of bits for the exponenent (ieee quantization)
  int32_t min ;      // used for minimum quantized value (all quantizations)
  int32_t max ;      // used for maximum quantized value (all quantizations)
  float amin ;       // smallest non zero absolute value (setup)
  float fmin ;       // minimum signed value (setup)
  float fmax ;       // largest signed value (setup)
  float fmaxa ;      // largest absolute value (setup)
  float rng ;        // range (power of 2) (setup)
  float rnga ;       // range of absolute values (power of 2) (setup)
  float epsi ;       // lowest absolute value considered as non zero
  float quant ;      // quantization unit (power of 2) (linear quantization)
  int32_t sbit ;     // 1 if sign bit needed
  int32_t negative ; // all numbers are negative
  uint32_t limit ;   // maximum absolute value possible
} qhead ;            // quantization information header

#define Q_SUBMODE_MASK  0xF
#define Q_MODE_LINEAR    64
#define Q_MODE_LINEAR_0  65
#define Q_MODE_LINEAR_1  66
#define Q_MODE_LINEAR_2  67
#define Q_MODE_LOG      128
#define Q_MODE_CONSTANT 256
typedef union{
    int32_t  i ;
    uint32_t u ;
    float    f ;
} i_u_f ;

typedef struct{
  uint32_t bias:32;  // minimum value
  uint32_t npts:14,  // number of points, 0 means unknown
           resv:13,
           nbts:5 ;  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
} ieee32_x ;

typedef union{
  ieee32_x x ;
  uint64_t u ;
} ieee32_u ;

typedef struct{
  int32_t mode ;
  i_u_f   bias ;
  i_u_f   spval ;
  i_u_f   mmask ;
  i_u_f   pad ;
  i_u_f   mins ;
  i_u_f   maxs ;
  int32_t nbits ;
  int32_t sign ;
  int32_t npts ;
} q_meta ;

// quantization states (0 is invalid)
#define TO_QUANTIZE  1
#define QUANTIZED    2
#define RESTORED     3

// quantization types
#define Q_FAKE_LOG_1   2
#define Q_LINEAR_0     4
#define Q_LINEAR_1     5
#define Q_LINEAR_2     6
// NOTES: Q_FAKE_LOG_1 is faster than Q_FAKE_LOG_0

// structs frules/quant_out/restored should have a matching layout where there are common elements
typedef struct{          // quantization rules, input to quantizing functions
  float    ref ;         // quantization quantum or max significant value
  uint32_t rng2:   8 ,   // base 2 exponent range (may override ref for fake log quantization)
           rng10:  6 ,   // base 10 exponent range (may override ref for fake log quantization)
           type:   3 ,   // quantization type (0 is invalid)
           clip:   1 ,   // quantized value of 0 is restored as 0
           nbits:  5 ,   // max number of bits for quantized value
           mbits:  5 ,   // significant bits (fake log quantizers)
           allp:   1 ,   // all values are >= 0
           allm:   1 ,   // all values are <  0
           state:  2 ;   // invalid/to_quantize/quantized/restored (should be TO_QUANTIZE)
} q_rules ;

typedef struct{          // quantization outcome, input to restore functions
  union{
    int32_t  i ;         // signed offset
    uint32_t u ;         // unsigned offset
    uint32_t emax: 8,    // fake log quantizer 0 uses unused offset to store emax
             resv:24 ;
  }offset ;              // quantization offset (minimum raw quantized value)
  uint32_t resv:   5 ,   // reserved, should be 0
           cnst:   1 ,   // constant field flag
           exp0:   8 ,   // IEEE exponent of minimum absolute value or quantization multiplier
           type:   3 ,   // quantization type (0 is invalid)
           clip:   1 ,   // quantized value of 0 is restored as 0
           nbits:  5 ,   // max number of bits for quantized value
           mbits:  5 ,   // significant bits (fake log quantizers)
                         // shift count (linear quantizer 0 and 2)
           allp:   1 ,   // all values are >= 0
           allm:   1 ,   // all values are <  0
           state:  2 ;   // invalid/to_quantize/quantized/restored (should be QUANTIZED)
} q_encode ;

typedef struct{          // after restore operation
  uint32_t npts ;        // number of points restored
  uint32_t resv:   6 ,   // reserved, should be 0
           exp0:   8 ,   // IEEE exponent of minimum absolute value or quantization multiplier
           type:   3 ,   // quantization type (0 is invalid)
           clip:   1 ,   // quantized value of 0 is restored as 0
           nbits:  5 ,   // max number of bits for quantized value
           mbits:  5 ,   // significant bits (fake log quantizers)
           allp:   1 ,   // all values are >= 0
           allm:   1 ,   // all values are <  0
           state:  2 ;   // invalid/to_quantize/quantized/restored (should be RESTORED)
} q_decode ;

typedef union{
  uint64_t u ;             // used to access everything as a single 64 bit item
  q_rules  f ;
  q_encode q ;
  q_decode r ;
}q_desc ;
// make sure that q_desc does not need more than 64 bits
CT_ASSERT(sizeof(q_desc) <= sizeof(uint64_t))

static q_desc q_desc_0 = {.u = 0 } ;

// "normalize" a mantissa to a reference exponent by shifting it right (after adding IEEE hidden 1)
// apply sign of input float to mantissa
// src    [IN] : 32 bit IEEE float (passed as a signed integer)
// RefExp [IN} : reference exponent (includes +127 bias)
// return signed "normalized" value
static int32_t normalize_mantissa(int32_t src, int32_t RefExp)
{
  int32_t Mantis = (1 << 23) | ( 0x7FFFFF & src );   // extract IEEE mantissa, restore hidden 1
  int32_t Exp    = (src >> 23) & 0xFF;               // extract IEEE float 32 exponent (includes +127 bias)
  if(Exp == 0) return 0 ;                            // denormalized float, return 0
  int32_t Shift  = RefExp - Exp;                     // shift count to normalize mantissa to RefExp exponent
  Shift = (Shift <  0) ?  0 : Shift ;                // Exp > RefExp, cannot normalize (invalid condition, should not happen)
  Shift = (Shift > 31) ? 31 : Shift ;                // max possible shift count = 31 (24 really)
  Mantis = Mantis >> Shift;                          // normalize mantissa to RefExp exponent
  Mantis = ( src < 0 ) ? -Mantis : Mantis ;          // apply sign
  return Mantis ;                                    // "normalized" mantissa
}

static void printf_quant_out(FILE *file, q_encode d){
  if(d.state != QUANTIZED) { fprintf(file, "ERROR: invalid state, expected %d, got %d\n", QUANTIZED, d.state) ; return ; }
  int pos_neg = (d.allp == 0) && (d.allm == 0) ;
  int ebits = d.nbits-d.mbits-pos_neg ;
  if(d.type != Q_FAKE_LOG_1) ebits = 0 ;
  fprintf(file, "type = %d, clip = %d, nbits[ebits](mbits) = %d[%d](%d), allp/allm = %d/%d, offset = %8.8x\n", 
                d.type, d.clip, d.nbits, ebits, d.mbits, d.allp, d.allm, d.offset.u) ;
}

typedef q_encode quantizer_function(void * restrict f, int ni, q_rules rules, void * restrict q, limits_w32 *limits, special_value *s) ;
typedef q_encode (*quantizer_fnptr)(void * restrict f, int ni, q_rules rules, void * restrict q, limits_w32 *limits, special_value *s) ;

// https://gcc.gnu.org/onlinedocs/gcc/Compound-Literals.html
static quantizer_function linear_quantizer_init ;
static q_encode linear_quantizer_init(void * restrict f, int ni, q_rules rules, void * restrict q, limits_w32 *limits, special_value *s){
  q_desc qr ;
  qr.u = q_desc_0.u ;
  qr.f = rules ;
  qr = (q_desc) {.u = q_desc_0.u } ;  // compound literal
  qr = (q_desc) {.q.state = rules.state, .f.ref = rules.ref } ;
  qr = q_desc_0 ;
  return qr.q ;
}
static q_encode linear_q_init(void * restrict f, int ni, q_rules rules, void * restrict q, limits_w32 *limits, special_value *s){
  return linear_quantizer_init(f, ni, rules, q, NULL, NULL) ;
}

typedef q_decode restore_function(void * restrict f, int ni, q_encode desc, void * restrict q) ;
typedef q_decode (*restore_fnptr)(void * restrict f, int ni, q_encode desc, void * restrict q) ;

restore_function linear_restore ;
static q_decode linear_rfunction(void * restrict f, int ni, q_encode rule, void * restrict q){
  return linear_restore(f, ni, rule, q) ;
}

// consistent interfaces for quantize and restore functions
//
// quantizer (linear or log) interface
// uint64_t = quantizer(void * restrict f, int ni, int nbits, float qref, void * restrict qs)
//
// restorer (linear or log) interface
// int restorer(void * restrict q, uint64_t h64, int ni, void * restrict f)

float quantum_adjust(float quantum);

int64_t IEEE_quantize(void * restrict f, void * restrict q, q_meta *meta,  int nd, int nbits, float error, 
                      int mode, void *spval, uint32_t mmask, void *pad);
int64_t IEEE_qrestore(void * restrict f, void * restrict q, q_meta *meta,  int nd);

quantizer_function IEEE32_linear_quantize_0 ;
// uint64_t IEEE32_linear_quantize_0(void * restrict f, int ni, int nbits, float quantum, void * restrict q, special_value *s);
quantizer_function IEEE32_linear_quantize_1 ;
// uint64_t IEEE32_linear_quantize_1(void * restrict f, int ni, int nbits, float quantum, void * restrict q);
quantizer_function IEEE32_linear_quantize_2 ;
// uint64_t IEEE32_linear_quantize_2(void * restrict f, int ni, int nbits, float quantum, void * restrict q, special_value *s);

restore_function IEEE32_linear_restore_0;
// int IEEE32_linear_restore_0(void * restrict q, uint64_t h64, int ni, void * restrict f);
restore_function IEEE32_linear_restore_1;
// int IEEE32_linear_restore_1(void * restrict q, uint64_t h64, int ni, void * restrict f);
restore_function IEEE32_linear_restore_2;
// int IEEE32_linear_restore_2(void * restrict q, uint64_t h64, int ni, void * restrict f);

quantizer_function IEEE32_fakelog_quantize_0 ;
// uint64_t IEEE32_fakelog_quantize_0(void * restrict f, int ni, int nbits, float qzero, void * restrict qs);
quantizer_function IEEE32_fakelog_quantize_1 ;
// uint64_t IEEE32_fakelog_quantize_1(void * restrict f, int ni, int nbits, float qzero, void * restrict qs);

restore_function IEEE32_fakelog_restore_0 ;
// int IEEE32_fakelog_restore_0(void * restrict q, uint64_t h64, int ni, void * restrict f);
restore_function IEEE32_fakelog_restore_1 ;
// int IEEE32_fakelog_restore_1(void * restrict q, uint64_t h64, int ni, void * restrict f);

void quantize_setup(float *z,            // array to be quantized (IEEE 754 32 bit float) (INPUT)
                        int n,           // number of data elements
                        qhead *h);       // quantization control information (OUTPUT)
void IEEE32_clip(void *f, int n, int nbits);
int32_t IEEE32_quantize(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
                      int32_t *q,      // quantized data (OUTPUT)
                      int n,           // number of data elements
                      int nexp,        // number of bits for the exponent part of quantized data (INPUT)
                      int nbits,       // number of bits in quantized data (INPUT)
                      qhead *h);       // quantization control information (OUTPUT)
int32_t IEEE32_quantize_v4(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
                      int32_t *q,      // quantized data (OUTPUT)
                      int n,           // number of data elements
                      int nexp,        // number of bits for the exponent part of quantized data (INPUT)
                      int nbits,       // number of bits in quantized data (INPUT)
                      qhead *h);       // quantization control information (OUTPUT)
int32_t IEEE32_restore(float *f,      // restored array (IEEE 754 32 bit float) (OUTPUT)
                        int32_t *q,    // quantized array (INPUT)
                        int n,         // number of data elements (INPUT)
                        qhead *h);     // quantization control information (INPUT)
void fp32_to_fp16_scaled(float *f, uint16_t *q, int n, float scale);
void fp32_to_fp16(float *f, uint16_t *q, int n);
void fp16_to_fp32(float *f, void *f16, int n, void *inf);
void fp16_to_fp32_scaled(float *f, void *f16, int n, void *inf, float scale);

#endif

