program test_data_info
  use ISO_C_BINDING
  implicit none
#define IN_FORTRAN_CODE
#include <rmn/data_info.h>

  integer :: i
  integer, parameter :: NP = 16384
  integer, parameter :: NP2 = NP * 2 + 1
  real(C_FLOAT), dimension(NP2) :: ff
  integer(C_INT32_T), dimension(NP2) :: fi
  type(limits_f) :: lf
  type(limits_i) :: li
  logical :: error

  print *, "Fortran test_data_info"
  ff(NP+1) = 1.1
  fi(NP+1) = 1
  do i = 1, NP
    ff(NP+1-i) = ff(NP+1-i+1) * 1.001
    fi(NP+1-i) = fi(NP+1-i+1) + 127
    ff(NP+1+i) = -ff(NP+1-i) * 1.001
    fi(NP+1+i) = -fi(NP+1-i) - 1
  enddo

  lf = IEEE32_extrema(ff, NP2)
  print 1, "maxs, mins, maxa, mina :" , lf%maxs, lf%mins, lf%maxa, lf%mina
  error = lf%mina .ne. ff(NP+1)
  error = error .or. lf%maxs .ne. ff(1)
  error = error .or. lf%mins .ne. ff(NP2)
  error = error .or. lf%maxa .ne. abs(ff(NP2))
  if(error) stop 1

  li = int32_extrema(fi, NP2)
  print 2, "maxs, mins, maxa, mina :" , li%maxs, li%mins, li%maxa, li%mina
  error = li%mina .ne. fi(NP+1)
  error = error .or. li%maxs .ne. fi(1)
  error = error .or. li%mins .ne. fi(NP2)
  error = error .or. li%maxa .ne. abs(fi(NP2))
  if(error) stop 1

  fi(3) = 0
  li = int32_extrema(fi, NP)
  print 2, "maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 0 .or. li%allp .ne. 1 .or. li%mina .ne. 0 .or. li%min0 .ne. 128
  if(error) stop 1

  li = int32_extrema(fi(NP+2), NP-1)
  print 2, "maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 1 .or. li%allp .ne. 0 .or. li%mina .ne. 129 .or. li%min0 .ne. 129
  if(error) stop 1

  fi(NP2-3) = 0
  li = int32_extrema(fi(NP+2), NP-1)
  print 2, "maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 0 .or. li%allp .ne. 0 .or. li%mina .ne. 0 .or. li%min0 .ne. 129
  if(error) stop 1

  print *,"SUCCESS"

1 format(A,4F15.2)
2 format(A,4I15)
end program
