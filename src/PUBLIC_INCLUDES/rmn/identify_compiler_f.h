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
! attempt to identify the Fortran compiler used to compile code including this file

#if defined(__x86_64__) || defined(__LP64__)
  integer, parameter :: ADDRESS_SIZE = 64   ! 64 bit addresses

#elif defined(__i386__)
  integer, parameter :: ADDRESS_SIZE = 32   ! 32 bit addresses

#else
  integer, parameter :: ADDRESS_SIZE = -1   ! unknown

#endif

#if defined(__GFORTRAN__)
  character(len=*), parameter :: FORTRAN_COMPILER = 'GNU'            ! gfortran

#elif defined(__INTEL_LLVM_COMPILER)
    character(len=*), parameter :: FORTRAN_COMPILER = 'INTEL_IFX'    ! ifx

#elif defined(__INTEL_COMPILER)
    character(len=*), parameter :: FORTRAN_COMPILER = 'INTEL_IFORT'  ! ifort

#elif defined(__PGIF90__)
  character(len=*), parameter :: FORTRAN_COMPILER = 'PGI/Nvidia'     ! pgfortran/nvfortran

#elif defined(__FLANG)
  character(len=*), parameter :: FORTRAN_COMPILER = 'AOCC'           ! aocc flang

#elif defined(__flang__)
  character(len=*), parameter :: FORTRAN_COMPILER = 'LLVM'           ! llvm flang

#else
  character(len=*), parameter :: FORTRAN_COMPILER = 'UNKNOWN'        ! unknown

#endif
