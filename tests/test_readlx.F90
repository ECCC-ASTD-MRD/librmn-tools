#define NEW_READLX_
#if defined(NEW_READLX)
#define qlxins qlxins_2
#define READLX READLX_2
#define qlxinx qlxinx_2
#endif
/* RMNLIB - Library of useful routines for C and FORTRAN programming
*  Copyright (C) 1975-2001  Division de Recherche en Prevision Numerique
*                           Environnement Canada
* 
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation,
*  version 2.1 of the License.
* 
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
* 
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the
*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
*  Boston, MA 02111-1307, USA.
*/
module yyy
      integer, save :: NTAB1,NTAB2,NTAB3,NSUB1,NSUB2,NSUB3,NNN
end module

PROGRAM YOYO
  use ISO_C_BINDING
  use yyy
  implicit none
  integer, dimension(10) :: TAB1,TAB2,TAB3
  integer :: dummy, INDICE
  interface
    subroutine program_exit(code) bind(C,name='exit')
      import :: C_INT32_T
      integer(C_INT32_T), intent(IN), value :: code
    end subroutine
  end interface

  EXTERNAL SUB1,SUB2, YOUPI
  integer :: KND, KRR, i
  character(len=4096) :: input_file

  if(command_argument_count() < 1) then
    print *,'ERROR: at least one command line argument is needed'
    call program_exit(1)
  endif
  call GET_COMMAND_ARGUMENT(1 , input_file)

  CALL qlxins(TAB1(1), 'TAB1', NTAB1, 9, 1)      ! up to 9 values, writable
  CALL qlxins(INDICE,  'IND' , NNN,   1, 1)      ! up to 1 value, writable
  CALL qlxins(TAB2(1), 'TAB2', NTAB2, 4, 1)      ! up to 4 values, writable
  CALL qlxins(TAB3(1), 'TAB3', NTAB3, 7, 1)      ! up to 7 values, writable
  CALL qlxins(22,   'CONST22', dummy, 1, 0)      ! up to 1 value, constant
  CALL qlxins(55,   'CONST55', dummy, 1, 0)      ! up to 1 value, constant

  CALL qlxinx(SUB1,  'SUB1', NSUB1, 0104, 2)    ! 1 to 4 argments, callable
  CALL qlxinx(SUB2,  'SUB2', NSUB2, 0305, 2)    ! 3 to 5 argments, callable
  CALL qlxinx(YOUPI, 'SUB3', NSUB3, 0000, 2)    ! NO argument, callable
!
  write(*,77) LOC(sub1),LOC(sub2)
77  format(' *** debug sub1 = ',z16.16,' sub2 = ',z16.16)
  PRINT *,' Avant READLX - input=inp_readlx'
!   IER = FNOM(5,'INP_READLX','SEQ',0)
  open(5, file=trim(input_file), form='FORMATTED')
  CALL READLX(5,KND,KRR)
  print '(A,I3,A,10A4)', 'NTAB2 =', NTAB2, ' TAB2 = ',(tab2(i), i=1,NTAB2)
  PRINT *,' APRES READLX - KND,KRR ',KND,KRR
!     CALL READLX(5,KND,KRR)
!     PRINT *,' APRES READLX - KND,KRR ',KND,KRR

!     PRINT *,' NTAB1,NTAB2MNTAB3 -',NTAB1,NTAB2,NTAB3
!     WRITE(6,'(3X,4Z20)')TAB1
!     WRITE(6,'(3X,4Z20)')TAB2
!     WRITE(6,'(3X,4Z20)')TAB3
      STOP
END
SUBROUTINE SUB1(A,B,C,D)
  use yyy
  INTEGER A(*),B(*),C(*),D(*),ARGDIMS,ARGDOPE,ND
  INTEGER LISTE(5)
  PRINT *,' PASSE PAR SUB1'
  PRINT *,' NB D ARGUMENTS =',NSUB1
   GOTO(1,2,3,4)NSUB1
4  PRINT 101,D(1),D(1),LOC(D),(D(I),I=1,ARGDIMS(4))
   ND = ARGDOPE(4,LISTE,5)
   PRINT 102,ND,LISTE
3  PRINT 101,C(1),C(1),LOC(C),(C(I),I=1,ARGDIMS(3))
   ND = ARGDOPE(3,LISTE,5)
   PRINT 102,ND,LISTE
2  PRINT 101,B(1),B(1),LOC(B),(B(I),I=1,ARGDIMS(2))
   ND = ARGDOPE(2,LISTE,5)
   PRINT 102,ND,LISTE
1  PRINT 101,A(1),A(1),LOC(A),(A(I),I=1,ARGDIMS(1))
   ND = ARGDOPE(1,LISTE,5)
   PRINT 102,ND,LISTE
   print *,' =========================='
101 FORMAT(3X,Z10.8,I10,Z20.16,4Z10.8)
102 FORMAT(1X,' ND=',I2,' LISTE=',5Z12.8)
END
SUBROUTINE SUB2(A,B,C,D,E)
  use yyy
  INTEGER A,B,C,D,E
  PRINT *,' PASSE PAR SUB2'
  PRINT *,' NB D ARGUMENTS =',NSUB2
  GOTO(1,2,3,4,5)NSUB2
5 PRINT 9,E,LOC(E)
4 PRINT 9,D,LOC(D)
3 PRINT 9,C,LOC(C)
2 PRINT 9,B,LOC(B)
1 PRINT 9,A,LOC(A)
9 format(I10,2x,Z16.16)
      RETURN
END
SUBROUTINE YOUPI
  PRINT *,' <><><> Y O U P I <><><>'
  RETURN
END
