! Copyright (C) 2022  Environnement et Changement climatique Canada
! 
! This is free software; you can redistribute it and/or
! modify it under the terms of the GNU Lesser General Public
! License as published by the Free Software Foundation,
! version 2.1 of the License.
! 
! This software is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
! Lesser General Public License for more details.
! 
! Author:
!     M. Valin,   Environnement et Changement climatique Canada, 2022
!
program test_bits
  use ISO_C_BINDING
  implicit none
! next define not necessary for GNU/aocc/LLVM/PGI Fortran compilers, necessary for Intel
#define IN_FORTRAN_CODE
#include <rmn/bits.h>
  integer(C_INT32_T) :: i32
  integer(C_INT64_T) :: i64
  real(C_FLOAT) :: r32
  real(C_DOUBLE) :: r64

  call start_of_test("Fortran extra bit operators"//achar(0))
  i32 = 31
  i64 = 63
  r32 = 1.5
  r64 = 1.5
  print 3,'i32/i64, r32/r64 =', i32, i64, r32, r64
  print 1,'i32 =', i32, ', popcnt(i32) =', popcnt(i32), ', lzcnt(i32) =', lzcnt(i32), ', lnzcnt(i32) =', lnzcnt(i32)
  print 2,'i64 =', i64, ', popcnt(i64) =', popcnt(i64), ', lzcnt(i64) =', lzcnt(i64), ', lnzcnt(i64) =', lnzcnt(i64)
  print 1,'r32 =', r32, ', popcnt(r32) =', popcnt(transfer(r32,i32)), ', lzcnt(r32) =', lzcnt(transfer(r32,i32)), ', lnzcnt(r32) =', lnzcnt(transfer(r32,i32))
  print 2,'r64 =', r64, ', popcnt(r64) =', popcnt(transfer(r64,i64)), ', lzcnt(r64) =', lzcnt(transfer(r64,i64)), ', lnzcnt(r64) =', lnzcnt(transfer(r64,i64))
  i32 = -i32
  i64 = -i64
  r32 = -r32
  r64 = -r64
  print 3,'i32/i64, r32/r64 =', i32, i64, r32, r64
  print 1,'i32 =', i32, ', popcnt(i32) =', popcnt(i32), ', lzcnt(i32) =', lzcnt(i32), ', lnzcnt(i32) =', lnzcnt(i32)
  print 2,'i64 =', i64, ', popcnt(i64) =', popcnt(i64), ', lzcnt(i64) =', lzcnt(i64), ', lnzcnt(i64) =', lnzcnt(i64)
  print 1,'r32 =', r32, ', popcnt(r32) =', popcnt(transfer(r32,i32)), ', lzcnt(r32) =', lzcnt(transfer(r32,i32)), ', lnzcnt(r32) =', lnzcnt(transfer(r32,i32))
  print 2,'r64 =', r64, ', popcnt(r64) =', popcnt(transfer(r64,i64)), ', lzcnt(r64) =', lzcnt(transfer(r64,i64)), ', lnzcnt(r64) =', lnzcnt(transfer(r64,i64))
1 format(A,Z16.8,A,i5,A,I5,A,I5)
2 format(A,Z16.16,A,i5,A,I5,A,I5)
3 format(A,I5,I5,F5.1,F5.1)
end program
