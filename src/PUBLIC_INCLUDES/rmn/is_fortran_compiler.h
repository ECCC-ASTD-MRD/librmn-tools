!
!  Copyright (C) 2023  Environnement et Changement climatique Canada
!
!  This is free software; you can redistribute it and/or
!  modify it under the terms of the GNU Lesser General Public
!  License as published by the Free Software Foundation,
!  version 2.1 of the License.
!
!  This software is distributed in the hope that it will be useful,
!  but WITHOUT ANY WARRANTY; without even the implied warranty of
!  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
!  Lesser General Public License for more details.
!
!  Author:
!      M. Valin,   Recherche en Prevision Numerique, 2023
!
! attempt to automatically set IN_FORTRAN_CODE
! when a Fortran compiler is formally recognized 
! macro IN_FORTRAN_CODE will be set ot 1
!
! usage :
!     #include <rmn/is_fortran_compiler.h>
!

#if ! defined(IN_FORTRAN_CODE)

#if defined(__GFORTRAN__)
! GNU gfortran
#define IN_FORTRAN_CODE 1

#elif defined(__INTEL_LLVM_COMPILER)
! macro also set for the C compiler

#elif defined(__INTEL_COMPILER)
! macro also set for the C compiler

#elif defined(__PGIF90__)
! Nvidia/PGI compiler
#define IN_FORTRAN_CODE 1

#elif defined(__FLANG)
! aocc flang compiler
#define IN_FORTRAN_CODE 1

#elif defined(__flang__)
! llvm flang-new (f18) compiler
#define IN_FORTRAN_CODE 1

#endif

#endif
