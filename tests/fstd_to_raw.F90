program fstd_to_raw
  use ISO_C_BINDING
  implicit none
#define IN_FORTRAN_CODE
#include <rmn/c_record_io.h>
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
  character (len=128) :: filename, varname, str_ip, dirname
  character (len=128) :: outfile
  integer :: iunout
  integer :: i, j, the_kind, ii
  real :: p1
  integer, dimension(:,:), pointer :: bits0
  integer :: fd, fdstatus, fdmode, ipkind
  real :: ipvalue
  integer(C_SIZE_T) :: nc
  character(len=128) :: c_fname, ipstring, tmpstring
  integer c1, c2, c0, ndata

  c0 = command_argument_count()
  if(c0 < 2 .or. c0 > 4) then
    call get_command_argument(0,filename,ilen,status)
    print *,'usage : '//trim(filename)//' standard_file variable_name [ dir_name [ out_file_postfix ] ]'
    stop
  endif
  write(0,*)'======= converting RPN standard file to raw file ======='
  iun=0
  iunout = 0
  call get_command_argument(1,filename,ilen,status)
  if(status .ne. 0) stop
  c2 = len(trim(filename))   ! eliminate first 11 chars of input file name (postfix to new file name)
  c1 = c2-11
  call get_command_argument(2,varname,ilen,status)
  if(status .ne. 0) stop
  dirname = 'RAW'
  if(c0 > 2) then
    call get_command_argument(3,dirname,ilen,status)
    if(status .ne. 0) stop
  endif

  status = fnom(iun,trim(filename),'RND+STD+R/O+OLD',0) ! existing std file opened in read-only mode
  if(c0 > 3) then   ! reuse file name for postfix to new file name
    call get_command_argument(4,filename,ilen,status)
    filename = trim(filename)   ! explicit postfix
    c1 = 1
    c2 = len(trim(filename))
    if(status .ne. 0) stop
  endif
  call fstopi("MSGLVL",6,0)
  if(status < 0) goto 999
  status = fstouv(iun,'RND')
  if(status < 0) goto 999    ! error opening source file
  nrec = fstnbr(iun)
  irec = 0
  ilev = 0
  oldnam='    '
  write(0,*)nrec,' records found, unit =',iun
  sizep = 0;

  fd = 0    ! will not close output file after write
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
        irec = irec + 1
        call fstluk(p,key,ni,nj,nk)
!       ==========================================================================
!       make clean string from ip1 (no spaces, no starting underscore, no consecutive underscores)
        call CONVIP_plus( ip1, ipvalue, ipkind, -1, tmpstring, .true. )  ! convert ip1
        i = 1
        j = 0
        ipstring = ' '
        do while(i < 21)
          do while(tmpstring(i:i) == ' ' .and. i < 21)     ! suppress sequence of spaces
            i = i + 1
          enddo
          if(i > 20) exit
          do while(tmpstring(i:i) .ne. ' ' .and. i < 21)   ! copy sequence of non space characters
            j = j + 1
            ipstring(j:j) = tmpstring(i:i)
            i = i + 1
          enddo
          j = j + 1
          ipstring(j:j) = '_'   ! add '_' after sequence of non space characters
        enddo
! ==========================================================================
        write(c_fname,1111)trim(dirname)//'/',trim(nomvar),'_'//trim(ipstring)//filename(c1:c2)   ! create file name
        ndata = write_32bit_data_record_named(trim(c_fname)//achar(0), fd, [ni, nj], 2, C_LOC(p(1)), nomvar//achar(0))
        if(fd < 0) goto 999   ! error opening output file
        print 2,'INFO, wrote',nc+16," bytes into '",trim(c_fname), "', fd =", fd,    &
                ", avg(p) = ",sum(p)/sizep, ' , avg(|p|) = ', sum(abs(p))/sizep,     &
                ', min/max = ', minval(p), maxval(p)
      endif    ! select desired variable name
    endif      ! ni>10 .and. nj>10
    key = fstsui(iun,ni,nj,nk)
  enddo        ! while(key >= 0)
  write(6,*)'number of records processed:',irec,' out of',nrec
  fd = -fd
  fdstatus = write_32bit_data_record_named(""//achar(0), fd, [0], 0, C_NULL_PTR, ""//achar(0)) ! dummy call to close output file
  if(iun .ne. 0) call fstfrm(iun)       ! close input file
  stop
999 continue
  write(0,*)'=== ERROR opening files ==='
  stop
2    format(A,I10,A,A,A,I3,A,G12.4,A,G12.4,A,2G12.4)
1111 format(A,A,A)
end program
