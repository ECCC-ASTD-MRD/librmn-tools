module analyze_itf
  use ISO_C_BINDING
  implicit none
  interface
    subroutine Analyze_NxN(f, ni, nj, varname) bind(C, name='Analyze_NxN')
      import :: C_FLOAT, C_INT32_T, C_CHAR
      implicit none
      integer(C_INT32_T), intent(IN), value :: ni, nj
      real(C_FLOAT), dimension(ni,nj), intent(IN) :: f
      character(C_CHAR), dimension(4) :: varname
    end subroutine
  end interface
end module

program fstd_to_raw
  use ISO_C_BINDING
  use analyze_itf
  implicit none
  integer, external :: fnom, fstouv, fstnbr, fstinf, fstsui
  integer :: iun, status, nrec, key, nk, irec, ilev, ilen
  integer, target :: ni, nj, ninj
  integer :: date,deet,npas,nbits,datyp,ip1,ip2,ip3,ig1,ig2,ig3,ig4
  integer :: swa,lng,dltf,ubc,extra1,extra2,extra3
  character(len=1) :: grtyp
  character(len=2) :: typvar
  character(len=4) :: nomvar
  character(len=12) :: etiket
  real, dimension(:), pointer :: p=>NULL()
  integer :: sizep
  character (len=128) :: filename, varname
  integer c1, c2, c0

  c0 = command_argument_count()
  if(c0 < 2 .or. c0 > 4) then
    call get_command_argument(0,filename,ilen,status)
    print *,'usage : '//trim(filename)//' standard_file variable_name'
    stop
  endif
  write(0,*)'======= analyzing RPN standard file contents ======='
  iun=0
  call get_command_argument(1,filename,ilen,status)
  if(status .ne. 0) stop
  c2 = len(trim(filename))   ! eliminate first 11 chars of input file name (postfix to new file name)
  c1 = c2-11
  call get_command_argument(2,varname,ilen,status)
  if(status .ne. 0) stop

  status = fnom(iun,trim(filename),'RND+STD+R/O+OLD',0) ! existing std file opened in read-only mode
  if(c0 > 3) then   ! reuse file name for postfix to new file name
    call get_command_argument(4,filename,ilen,status)
    filename = trim(filename)   ! explicit postfix
    c1 = 1
    c2 = len(trim(filename))
    if(status .ne. 0) stop
  endif
  call fstopi("MSGLVL",0,0)
  if(status < 0) goto 999
  status = fstouv(iun,'RND')
  if(status < 0) goto 999    ! error opening source file
  nrec = fstnbr(iun)
  irec = 0
  ilev = 0
  write(0,*)nrec,' records found, unit =',iun
  sizep = 0;

  key = fstinf(iun,ni,nj,nk,-1,'            ',-1,-1,-1,'  ','    ') ! select any record
  do while(key >= 0)
    if(ni>10 .and. nj>10) then
      if(ni*nj*nk > sizep) then
        if(associated(p)) deallocate(p)
        sizep = ni*nj*nk
        allocate(p(sizep))
      endif
      call fstprm(key,date,deet,npas,ni,nj,nk,nbits,datyp,ip1,ip2,ip3,  &
                  typvar,nomvar,etiket,grtyp,ig1,ig2,ig3,ig4,           &
                  swa,lng,dltf,ubc,extra1,extra2,extra3)
      if(nomvar(1:4) == varname(1:4) .or. varname(1:1) == '+') then  ! select desired variable name
        call fstluk(p,key,ni,nj,nk)
!         write(0,*)'processing '//nomvar(1:4), ni, 'x', nj
        call Analyze_NxN(p, ni, nj, nomvar//char(0))
        irec = irec + 1
      else
        write(0,*)'ignoring '//nomvar(1:4), ni, 'x', nj
      endif    ! select desired variable name
    else
      write(0,*)'skipping ni =',ni,', nj =',nj
    endif      ! ni>10 .and. nj>10
    key = fstsui(iun,ni,nj,nk)
  enddo        ! while(key >= 0)
  write(0,*)'number of records processed:',irec,' out of',nrec
  if(iun .ne. 0) call fstfrm(iun)       ! close input file
  stop
999 continue
  write(0,*)"=== ERROR opening file '" // trim(filename) // "' ==="
  stop
end program
