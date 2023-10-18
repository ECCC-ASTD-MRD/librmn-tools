/*
 * Hopefully useful code for C or Fortran
 * Copyright (C) 2022  Recherche en Prevision Numerique
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 */
#if ! defined(DATA_INFO_PROTOTYPES)
#define DATA_INFO_PROTOTYPES

#if defined(IN_FORTRAN_CODE) || defined(__GFORTRAN__)
type, BIND(C) :: special_value  ! MUST MIRROR C struct special_value
  type(C_PTR) :: spv
  type(C_PTR) :: pad
  integer(C_INT32_T) :: mask
end type

type, BIND(C) :: limits_f
  real(C_FLOAT)      :: mins   ! lowest signed value
  real(C_FLOAT)      :: maxs   ! highest signed value
  real(C_FLOAT)      :: mina   ! smallest absolute value
  real(C_FLOAT)      :: maxa   ! largest absolute value
  real(C_FLOAT)      :: min0   ! smallest non zero absolute value
  integer(C_INT32_T) :: spec   ! number of "special" values
  integer(C_INT8_T)  :: allp   ! 1 if all values are non negative
  integer(C_INT8_T)  :: allm   ! 1 if all values are negative
  integer(C_INT8_T)  :: maxe   ! highest IEEE exponent (with bias)
  integer(C_INT8_T)  :: mine   ! lowest IEEE exponent (with bias)
end type

type, BIND(C) :: limits_i
  integer(C_INT32_T) :: mins   ! lowest signed value
  integer(C_INT32_T) :: maxs   ! highest signed value
  integer(C_INT32_T) :: mina   ! smallest absolute value
  integer(C_INT32_T) :: maxa   ! largest absolute value
  integer(C_INT32_T) :: min0   ! smallest non zero absolute value
  integer(C_INT32_T) :: spec   ! number of "special" values
  integer(C_INT8_T)  :: allp   ! 1 if all values are non negative
  integer(C_INT8_T)  :: allm   ! 1 if all values are negative
  integer(C_INT8_T)  :: maxe   ! highest IEEE exponent (with bias)
  integer(C_INT8_T)  :: mine   ! lowest IEEE exponent (with bias)
end type

interface
  function W32_replace_special(f, np, s) result(n) bind(C, name='W32_replace_special')
    import C_PTR, C_INT32_T, special_value
    type(C_PTR), intent(IN), value :: f
    type(special_value), intent(IN) :: s
    integer(C_INT32_T), intent(IN), value :: np
    integer(C_INT32_T) :: n
  end function
!  function W32_replace_missing(f, np, spval, mmask, pad) result(n) bind(C, name='W32_replace_missing')
!    import C_PTR, C_INT32_T
!    type(C_PTR), intent(IN), value :: f, spval, pad
!    integer(C_INT32_T), intent(IN), value :: np, mmask
!    integer(C_INT32_T) :: n
!  end function
  function IEEE32_extrema(f, np) result(limits) bind(C, name='IEEE32_extrema')
    import :: C_FLOAT, C_INT32_T, limits_f
    implicit none
    real(C_FLOAT), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_f) :: limits
  end function
  function IEEE32_extrema_special(f, np, s ) result(limits) bind(C, name='IEEE32_extrema_special')
    import :: C_FLOAT, C_INT32_T, limits_f, C_PTR, special_value
    implicit none
    real(C_FLOAT), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(special_value), intent(IN) :: s
    type(limits_f) :: limits
  end function
!  function IEEE32_extrema_missing(f, np, missing, mmask, pad ) result(limits) bind(C, name='IEEE32_extrema_missing')
!    import :: C_FLOAT, C_INT32_T, limits_f, C_PTR
!    implicit none
!    real(C_FLOAT), dimension(*), intent(IN) :: f
!    integer(C_INT32_T), intent(IN), value :: np, mmask
!    type(C_PTR), intent(IN), value :: missing, pad
!    type(limits_f) :: limits
!  end function
  function int32_extrema(f, np) result(limits) bind(C, name='INT32_extrema')
    import :: C_INT32_T, limits_i
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_i) :: limits
  end function
  function INT32_extrema_special(f, np, s) result(limits) bind(C, name='INT32_extrema_special')
    import :: C_INT32_T, limits_i, C_PTR, special_value
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(special_value), intent(IN) :: s
    type(limits_i) :: limits
  end function
!  function int32_extrema_missing(f, np, missing, mmask, pad) result(limits) bind(C, name='INT32_extrema_missing')
!    import :: C_INT32_T, limits_i, C_PTR
!    implicit none
!    integer(C_INT32_T), dimension(*), intent(IN) :: f
!    integer(C_INT32_T), intent(IN), value :: np, mmask
!    type(C_PTR), intent(IN), value :: missing, pad
!    type(limits_i) :: limits
!  end function
  function uint32_extrema(f, np) result(limits) bind(C, name='UINT32_extrema')
    import :: C_INT32_T, limits_i
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_i) :: limits
  end function
  function UINT32_extrema_special(f, np, s) result(limits) bind(C, name='UINT32_extrema_special')
    import :: C_INT32_T, limits_i, C_PTR, special_value
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(special_value), intent(IN) :: s
    type(limits_i) :: limits
  end function
!  function uint32_extrema_missing(f, np, missing, mmask, pad) result(limits) bind(C, name='UINT32_extrema_missing')
!    import :: C_INT32_T, limits_i, C_PTR
!    implicit none
!    integer(C_INT32_T), dimension(*), intent(IN) :: f
!    integer(C_INT32_T), intent(IN), value :: np, mmask
!    type(C_PTR), intent(IN), value :: missing, pad
!    type(limits_i) :: limits
!  end function
end interface

#else        // defined(IN_FORTRAN_CODE)

#include <stdint.h>
#include <rmn/ct_assert.h>

typedef struct{
  int32_t  mins ;   // lowest signed value
  int32_t  maxs ;   // highest signed value
  uint32_t mina ;   // smallest absolute value
  uint32_t maxa ;   // largest absolute value
  uint32_t min0 ;   // smallest non zero absolute value
  uint32_t spec ;   // number of "special" values
  uint8_t  allp ;   // 1 if all values are non negative
  uint8_t  allm ;   // 1 if all values are negative
  uint8_t  maxe ;   // should be 0 (only makes sense for floats)
  uint8_t  mine ;   // should be 0 (only makes sense for floats)
}limits_i ;         // signed integer limits

typedef struct{
  uint32_t mins ;   // lowest absolute value
  uint32_t maxs ;   // highest absolute value
  uint32_t mina ;   // smallest absolute value (same as mins)
  uint32_t maxa ;   // largest absolute value (same as maxs)
  uint32_t min0 ;   // smallest non zero absolute value
  uint32_t spec ;   // number of "special" values
  uint8_t  allp ;   // 1 if all values are non negative
  uint8_t  allm ;   // 1 if all values are negative
  uint8_t  maxe ;   // should be 0 (only makes sense for floats)
  uint8_t  mine ;   // should be 0 (only makes sense for floats)
}limits_u ;         // unsigned integer limits

typedef struct{
  float mins ;      // IEEE32 bit pattern of lowest signed value
  float maxs ;      // IEEE32 bit pattern of highest signed value
  float mina ;      // IEEE32 bit pattern of smallest absolute value
  float maxa ;      // IEEE32 bit pattern of largest absolute value
  float min0 ;      // IEEE32 bit pattern of smallest non zero absolute value
  uint32_t spec ;   // number of "special" values
  uint8_t  allp ;   // 1 if all values are non negative
  uint8_t  allm ;   // 1 if all values are negative
  uint8_t  maxe ;   // highest IEEE exponent (with bias)
  uint8_t  mine ;   // lowest IEEE exponent (with bias)
}limits_f ;         // IEEE float limits

typedef union{
  limits_i i ;      // signed integer limits
  limits_u u ;      // unsigned integer limits
  limits_f f ;      // IEEE float limits
} limits_w32 ;      // limits union

CT_ASSERT(sizeof(limits_w32) == 28)

// structure used to describe the "special" value, how to recognize it, and how to deal with it
// (*spv & mask) == (value & mask) : a "special" value is recognized
// if pad is not NULL, *pad is used to replace said "special" value for data analysis purposes
//                     the data array is not affected
// function W32_replace_special replaces "special" values with *pad in data array
typedef struct{
  void     *spv ;   // pointer to special value, if NULL, no special value
  void     *pad ;   // pointer to special value replacement , if NULL, no replacement
  uint32_t mask ;   // mask for special value detection, if 0xFFFFFFFF detection is deactivated
} special_value ;

// N.B. some compiler versions fail to compile when the return value of a function is larger than 128 bits

// true if all values are non negative (sign bit of maxs and mins is 0)
#define W32_ALLP(l) (((l.i.maxs | l.i.mins) >> 31) == 0)
// true if all values are negative or 0
#define W32_ALLM(l) ( (l.i.maxs <= 0) & (l.i.mins <= 0) )
// #define W32_ALLM(l) (((l.i.maxs & l.i.mins) >> 31) != 0)

int W32_replace_special(void * restrict f, int np, special_value *sp);
// int W32_replace_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad);

limits_w32 UINT32_extrema(void * restrict f, int np);
limits_w32 UINT32_extrema_special(void * restrict f, int np, special_value *sp);
// limits_w32 UINT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad);

limits_w32 INT32_extrema(void * restrict f, int np);
limits_w32 INT32_extrema_special(void * restrict f, int np, special_value *sp);
// limits_w32 INT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad);

limits_w32 IEEE32_extrema(void * restrict f, int np);
limits_w32 IEEE32_extrema_abs(void * restrict f, int np);
limits_w32 IEEE32_extrema_special(void * restrict f, int np, special_value *sp);
// limits_w32 IEEE32_extrema_missing(void * restrict f, int np, void * spval, uint32_t mmask, void *pad);

void IEEE32_exp_limits(limits_w32 l32, int *emin, int *emax);

#endif        // defined(IN_FORTRAN_CODE)

#endif        // ! defined(DATA_INFO_PROTOTYPES)
