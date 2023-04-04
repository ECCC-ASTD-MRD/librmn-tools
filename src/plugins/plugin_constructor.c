//
//  Copyright (C) 2023  Environnement Canada
// 
//  This is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation,
//  version 2.1 of the License.
// 
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
#include <stdio.h>

// C constructor/destructor pair
// calls fortran_constructor/fortran_destructor if they exist

int fortran_constructor(void) __attribute__((weak)) ;
int fortran_destructor(void) __attribute__((weak)) ;

void __attribute__ ((constructor)) FortranConstructor(void) {
   if( fortran_constructor ){
     printf("fortran_constructor found at [%p]\n", &fortran_constructor) ;
     fortran_constructor() ;   // call user supplied Fortran constructor
   }else{
     printf("INFO: fortran_constructor NOT FOUND\n") ;
   }
}

void __attribute__ ((destructor)) FortranDestructor(void) {
   if( fortran_destructor ){
     printf("fortran_destructor found at [%p]\n", &fortran_destructor) ;
     fortran_destructor() ;   // call user supplied Fortran destructor
   }else{
     printf("INFO:fortran_destructor NOT FOUND\n") ;
   }
}
