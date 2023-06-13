program test_compress
  use ISO_C_BINDING
  implicit none
  integer, external :: fnom, fstouv, fstnbr, fstinf, fstsui
  integer :: iun, status, nrec, key, nk, irec, ilev, ilen
  integer, target :: ni, nj, ninj
  integer :: date,deet,npas,nbits,datyp,ip1,ip2,ip3,ig1,ig2,ig3,ig4
  integer :: swa,lng,dltf,ubc,extra1,extra2,extra3
  character(len=2) :: typvar
  character(len=4) :: nomvar, oldnam
  character(len=12) :: etiket
  character(len=1) :: grtyp
  real, dimension(:), pointer :: p=>NULL()
  integer :: sizep
  integer, dimension(:,:), pointer :: iz=>NULL()
  character (len=128) :: filename, varname, str_ip
  character (len=128) :: outfile
  integer :: iunout
  integer :: i, j, the_kind, ii
  real :: p1
  interface
    function c_creat(name, mode) result(fd) bind(C,name='creat')
      import :: C_CHAR, C_INT
      implicit none
      character(C_CHAR), dimension(*), intent(IN) :: name
      integer(C_INT), intent(IN), value :: mode
      integer(C_INT) :: fd
    end function
    function c_write(fd, buf, cnt) result(nc) bind(C, name='write')
      import :: C_SIZE_T, C_PTR, C_INT
      implicit none
      integer(C_INT), intent(IN), value :: fd
      type(C_PTR), intent(IN), value :: buf
      integer(C_SIZE_T), intent(IN), value :: cnt
      integer(C_SIZE_T) :: nc
    end function
    function c_read(fd, buf, cnt) result(nc) bind(C, name='read')
      import :: C_SIZE_T, C_PTR, C_INT
      implicit none
      integer(C_INT), intent(IN), value :: fd
      type(C_PTR), intent(IN), value :: buf
      integer(C_SIZE_T), intent(IN), value :: cnt
      integer(C_SIZE_T) :: nc
    end function
    function c_close(fd) result(status) bind(C,name='close')
      import :: C_INT
      implicit none
      integer(C_INT), intent(IN), value :: fd
      integer(C_INT) :: status
    end function
  end interface
  integer, dimension(:,:), pointer :: bits0
  integer :: fd, fdstatus, fdmode, ipkind
  real :: ipvalue
  integer(C_SIZE_T) :: nc
  character(len=128) :: c_fname, ipstring
  integer c1, c2

  write(0,*)'======= converting RPN standard file to raw file ======='
  iun=0
  iunout = 0
  call get_command_argument(1,filename,ilen,status)
  if(status .ne. 0) stop
  c2 = len(trim(filename))
  c1 = c2-11
  call get_command_argument(2,varname,ilen,status)
  if(status .ne. 0) stop

  status = fnom(iun,trim(filename),'RND+STD+R/O+OLD',0)
  call fstopi("MSGLVL",6,0)
  if(status < 0) goto 999
  status = fstouv(iun,'RND')
  if(status < 0) goto 999
  nrec = fstnbr(iun)
  irec = 0
  ilev = 0
  oldnam='    '
  write(0,*)nrec,' records found, unit =',iun
  sizep = 0;

  key = fstinf(iun,ni,nj,nk,-1,'            ',-1,-1,-1,'  ','    ')
  do while(key >= 0)
    if(ni>10 .and. nj>10) then
      if(ni*nj*nk > sizep) then
        if(associated(p)) deallocate(p)
        sizep = ni*nj*nk
        allocate(p(sizep))
      endif
      call fstprm(key,date,deet,npas,ni,nj,nk,nbits,datyp,ip1,ip2,ip3,  &
                  typvar,nomvar,etiket,grtyp,ig1,ig2,ig3,ig4, &
                  swa,lng,dltf,ubc,extra1,extra2,extra3)
!       varname(1:2) = nomvar(1:2)
      if(nomvar(1:2) == varname(1:2) .or. varname(1:1) == '+') then
        irec = irec + 1
        call fstluk(p,key,ni,nj,nk)
! ==========================================================================
        call CONVIP_plus( ip1, ipvalue, ipkind, -1, ipstring, .true. )  ! convert ip1
        ii = 1
        do i = 1, 20
          if(ipstring(i:i) == ' ') then
            ipstring(i:i) = '_'
!             exit
          endif
        enddo
        do i = 20, 1, -1
          if(ipstring(i:i) == '_') then
            ipstring(i:i) = ' '
          else
            exit
          endif
        enddo
        write(c_fname,1111)'RAW/',trim(nomvar),'_'//trim(ipstring)//filename(c1:c2)   ! create file name
1111 format(A,A,A,I3.3)
        fdmode = INT(O'777')
        fd = c_creat(trim(c_fname)//achar(0), fdmode) ! create raw file
        if(fd > 0) then
          ninj = 2         ! 2D data
          nc = 4
          nc = c_write(fd, C_LOC(ninj), nc)
          nc = 4
          nc = c_write(fd, C_LOC(ni), nc)
          nc = 4
          nc = c_write(fd, C_LOC(nj), nc)
          nc = 4 * ni * nj
          nc = c_write(fd, C_LOC(p(1)), nc)
          print *,'INFO, wrote',nc+16,' bytes into ',trim(c_fname)
          ninj = ni * nj ;
          nc = 4
          nc = c_write(fd, C_LOC(ninj), nc)
          fdstatus = c_close(fd)
        else
          print *,'ERROR creating '//trim(c_fname)
        endif
!         if(irec == 25) exit
      endif
    endif
    key = fstsui(iun,ni,nj,nk)
  enddo
  write(6,*)'number of records processed:',irec,' out of',nrec
888 continue
  if(iun .ne. 0) call fstfrm(iun)
  stop
999 continue
  write(0,*)'=== ERROR opening files ==='
  stop
end program
