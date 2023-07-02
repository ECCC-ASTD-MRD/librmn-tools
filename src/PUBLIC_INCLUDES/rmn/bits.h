/*
Hopefully useful code for C and Fortran
Copyright (C) 2023  Recherche en Prevision Numerique

This code is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation,
version 2.1 of the License.

This code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

 bit operators, left/right masks, set bit counts, leading zero/one bit counts
 a mix of macros and statement functions

*/
#if ! defined(IN_FORTRAN_CODE) && ! defined(__GFORTRAN__)

// C interfaces and declarations

#if ! defined(RMNTOOLS_BITS)
#define RMNTOOLS_BITS

#include <stdint.h>

// STATIC may be defined as extern, to generate real entry points
#if ! defined(STATIC)
#define STATIC static
#define STATIC_DEFINED_HERE
#endif

// number of bits needed to represent range (positive number)
#define NEEDBITS(range,needed) { uint64_t rng = (range) ; needed = 1; while (rng >>= 1) needed++ ; }

// 32 and 64 bit left aligned masks (nbits may be 0)
#define LMASK32(nbits)  ((nbits) ? ((~0u )  << (32-nbits)) : 0 )
#define LMASK64(nbits)  ((nbits) ? ((~0lu)  << (64-nbits)) : 0 )

// 32 and 64 bit left aligned masks (nbits == 0) NOT supported
// (same as above but faster, assuming 0 bit case not needed)
#define LMASK32Z(nbits)  ((~0u )  << (32-nbits))
#define LMASK64Z(nbits)  ((~0lu)  << (64-nbits))

// 32 and 64 bit right aligned masks (nbits may be 32)
#define RMASK32(nbits)  (((nbits) == 32) ? (~0u ) : (~((~0u)  << nbits)))
#define RMASK64(nbits)  (((nbits) == 64) ? (~0lu) : (~((~0lu) << nbits)))

// 31 and 63 bit right aligned masks  (nbits MUST be < 32)
// (same as above but faster, assuming 32/64 bit case not needed)
#define RMASK31(nbits)  (~((~0u)  << nbits))
#define RMASK63(nbits)  (~((~0lu) << nbits))

// population count (32 bit words) (count number of bits that are set to 1)
STATIC inline uint32_t popcnt_32(uint32_t what){
  uint32_t cnt ;
#if defined(__x86_64__)
  // X86 family of processors
  __asm__ __volatile__ ("popcnt{l %1, %0| %0, %1}" : "=r"(cnt) : "r"(what) : "cc" ) ;
#else
  cnt = 0 ;
  while(what & 1){
    cnt++ ;
    what >> = 1 ;
  }
#endif
  return cnt ;
}

// population count (64 bit words) (count number of bits that are set to 1)
STATIC inline uint32_t popcnt_64(uint64_t what){
  uint64_t cnt ;
#if defined(__x86_64__)
  // X86 family of processors
  __asm__ __volatile__ ("popcnt{ %1, %0| %0, %1}" : "=r"(cnt) : "r"(what) : "cc" ) ;
#else
  cnt = 0 ;
  while(what & 1){
    cnt++ ;
    what >> = 1 ;
  }
#endif
  return cnt ;
}

// leading zeros count (32 bit word)
STATIC inline uint32_t lzcnt_32(uint32_t what){
  uint32_t cnt ;
#if defined(__x86_64__)
  // X86 family of processors
  __asm__ __volatile__ ("lzcnt{l %1, %0| %0, %1}" : "=r"(cnt) : "r"(what) : "cc" ) ;
#elif defined(__aarch64__)
  // ARM family of processors
   __asm__ __volatile__ ("clz %w[out], %w[in]" : [out]"=r"(cnt) : [in]"r"(what) ) ;
#else
   // generic code
   cnt = 32;
   if(what >> 16) { cnt-= 16 ; what >>= 16 ; } ; // bits (16-31) not 0
   if(what >>  8) { cnt-=  8 ; what >>=  8 ; } ; // bits ( 8-15) not 0
   if(what >>  4) { cnt-=  4 ; what >>=  4 ; } ; // bits ( 4- 7) not 0
   if(what >>  2) { cnt-=  2 ; what >>=  2 ; } ; // bits ( 2- 3) not 0
   if(what >>  1) { cnt-=  1 ; what >>=  1 ; } ; // bits ( 1- 1) not 0
   if(what) cnt-- ;                            // bit 0 not 0 ;
#endif
  return cnt ;
}

// leading ones count (32 bit word)
STATIC inline uint32_t lnzcnt_32(uint32_t what){
  return lzcnt_32(~what) ;
}

// leading zeros count (64 bit word)
STATIC inline uint32_t lzcnt_64(uint64_t what){
  uint64_t cnt ;
#if defined(__x86_64__)
  // X86 family of processors
  __asm__ __volatile__ ("lzcnt{ %1, %0| %0, %1}" : "=r"(cnt) : "r"(what) : "cc" ) ;
#elif defined(__aarch64__)
  // ARM family of processors
   __asm__ __volatile__ ("clz %[out], %[in]" : [out]"=r"(cnt) : [in]"r"(what) ) ;
#else
   // generic code
   cnt = 64;
   if(what >> 32) { cnt-= 32 ; what >>= 32 ; } ; // bits (32-63) not 0
   if(what >> 16) { cnt-= 16 ; what >>= 16 ; } ; // bits (16-31) not 0
   if(what >>  8) { cnt-=  8 ; what >>=  8 ; } ; // bits ( 8-15) not 0
   if(what >>  4) { cnt-=  4 ; what >>=  4 ; } ; // bits ( 4- 7) not 0
   if(what >>  2) { cnt-=  2 ; what >>=  2 ; } ; // bits ( 2- 3) not 0
   if(what >>  1) { cnt-=  1 ; what >>=  1 ; } ; // bits ( 1- 1) not 0
   if(what) cnt-- ;                            // bit 0 not 0 ;
#endif
  return cnt ;
}

// leading ones count (64 bit word)
STATIC inline uint32_t lnzcnt_64(uint64_t what){
  return lzcnt_64(~what) ;
}

#if defined(STATIC_DEFINED_HERE)
#undef STATIC
#undef STATIC_DEFINED_HERE
#endif

#endif

#else

!  some Fortran interfaces and declarations

  interface popcnt  ! generic interface
    function popcnt_32(what) result(nbits) bind(C,name='popcnt_32')
      import C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: what
      integer(C_INT32_T) :: nbits
    end function popcnt_32
    function popcnt_64(what) result(nbits) bind(C,name='popcnt_64')
      import C_INT32_T, C_INT64_T
      implicit none
      integer(C_INT64_T), intent(IN), value :: what
      integer(C_INT32_T) :: nbits
    end function popcnt_64
  end interface

  interface lzcnt  ! generic interface
    function lzcnt_32(what) result(nbits) bind(C,name='lzcnt_32')
      import C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: what
      integer(C_INT32_T) :: nbits
    end function lzcnt_32
    function lzcnt_64(what) result(nbits) bind(C,name='lzcnt_64')
      import C_INT32_T, C_INT64_T
      implicit none
      integer(C_INT64_T), intent(IN), value :: what
      integer(C_INT32_T) :: nbits
    end function lzcnt_64
  end interface

  interface lnzcnt  ! generic interface
    function lnzcnt_32(what) result(nbits) bind(C,name='lnzcnt_32')
      import C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: what
      integer(C_INT32_T) :: nbits
    end function lnzcnt_32
    function lnzcnt_64(what) result(nbits) bind(C,name='lnzcnt_64')
      import C_INT32_T, C_INT64_T
      implicit none
      integer(C_INT64_T), intent(IN), value :: what
      integer(C_INT32_T) :: nbits
    end function lnzcnt_64
  end interface

#endif
