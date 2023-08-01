program test_data_info
  use data_info_mod
  implicit none

  integer :: i, n
  integer, parameter :: NP = 16384
  integer, parameter :: NP2 = NP * 2 + 1
  real(C_FLOAT), dimension(NP2, 1), target      :: ff
  integer(C_INT32_T), dimension(NP2, 1), target :: fi
  type(limits_f) :: lf
  type(limits_i) :: li
  logical :: error
  integer(C_INT32_T), target :: ipad, imissing

  print *, "Fortran test_data_info"
  ff(NP+1,1) = 1.1
  fi(NP+1,1) = 1
  do i = 1, NP
    ff(NP+1-i,1) = ff(NP+1-i+1,1) * 1.001
    fi(NP+1-i,1) = fi(NP+1-i+1,1) + 127
    ff(NP+1+i,1) = -ff(NP+1-i ,1) * 1.001
    fi(NP+1+i,1) = -fi(NP+1-i ,1) - 1
  enddo

  ! generic call needs what looks like a rank 1 array as first argument to match dimension(*)
  lf = extrema(ff(:,1), NP2)                 ! float extrema, all optional arguments are omitted
  print 1, "lf   : maxs, mins, maxa, mina :" , lf%maxs, lf%mins, lf%maxa, lf%mina
  error = lf%mina .ne. ff(NP+1,1)
  error = error .or. lf%maxs .ne. ff(1,1)
  error = error .or. lf%mins .ne. ff(NP2,1)
  error = error .or. lf%maxa .ne. abs(ff(NP2,1))
  if(error) stop 1

  ! float extrema, one optional argument omitted
  lf = extrema_f(ff, NP2, missing = 0.0, mmask = -1, pad = 0.0)
  print 1, "lf   : maxs, mins, maxa, mina :" , lf%maxs, lf%mins, lf%maxa, lf%mina
  error = lf%mina .ne. ff(NP+1,1)
  error = error .or. lf%maxs .ne. ff(1,1)
  error = error .or. lf%mins .ne. ff(NP2,1)
  error = error .or. lf%maxa .ne. abs(ff(NP2,1))
  if(error) stop 1

  li = extrema_i(fi, NP2, unsigned=.false.)  ! signed integer extrema, 3 optional arguments are omitted
  print 2, "li(f): maxs, mins, maxa, mina :" , li%maxs, li%mins, li%maxa, li%mina
  error = li%mina .ne. fi(NP+1,1)
  error = error .or. li%maxs .ne. fi(1,1)
  error = error .or. li%mins .ne. fi(NP2,1)
  error = error .or. li%maxa .ne. abs(fi(NP2,1))
  if(error) stop 1

  li = extrema_i(fi, NP+1, unsigned=.true.)  ! unsigned integer extrema, 3 optional arguments are omitted
  print 2, "li(t): maxs, mins, maxa, mina :" , li%maxs, li%mins, li%maxa, li%mina
  error = li%mina .ne. fi(NP+1,1)
  error = error .or. li%maxs .ne. 0
  error = error .or. li%mins .ne. 1
  error = error .or. li%maxa .ne. fi(1,1)
  if(error) stop 1

  fi(3,1) = 0
  li = extrema_i(fi, NP)                     ! integer extrema, all optional arguments are omitted
  print 2, "li(l): maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "       allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 0 .or. li%allp .ne. 1 .or. li%mina .ne. 0 .or. li%min0 .ne. 128
  if(error) stop 1

  ! specific function call O.K. with dimension(*)
  li = extrema_i(fi(NP+2,1), NP-1)           ! integer extrema, all optional arguments are omitted
  print 2, "li(u): maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "       allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 1 .or. li%allp .ne. 0 .or. li%mina .ne. 129 .or. li%min0 .ne. 129
  if(error) stop 1

  ! generic call needs what looks like a rank 1 array as first argument to match dimension(*)
  li = extrema(fi(NP+2:,1), NP-1)            ! integer extrema, all optional arguments are omitted
  print 2, "li(u): maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "       allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 1 .or. li%allp .ne. 0 .or. li%mina .ne. 129 .or. li%min0 .ne. 129
  if(error) stop 1

  fi(NP2-3,1) = 0
  li = int32_extrema_missing(fi(NP+2,1), NP-1, C_NULL_PTR, -1, C_NULL_PTR)   ! explicit C function call
  print 2, "li(m): maxs, mins, mina, min0 :" , li%maxs, li%mins, li%mina, li%min0
  print 2, "       allm, allp             :" , li%allm, li%allp
  error = li%allm .ne. 1 .or. li%allp .ne. 0 .or. li%mina .ne. 0 .or. li%min0 .ne. 129
  if(error) stop 1

  imissing = 1
  ipad     = 12
  n = W32_replace_missing(C_LOC(fi(1,1)), NP2, C_LOC(imissing), 0, C_LOC(ipad))
  print 2, "missing, pad, n, fi(NP+1)     :", imissing, ipad, n, fi(NP+1,1)
  error = n .ne. NP2 .or. fi(NP+1,1) .ne. ipad
  if(error) stop 1

  print *,"SUCCESS"

1 format(A,4F15.2)
2 format(A,4I15)
end program
