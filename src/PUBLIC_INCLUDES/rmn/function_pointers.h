//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
//  M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <rmn/ct_assert.h>

typedef void *data_ptr ;
typedef int (*proc_ptr)() ;

// data and function pointers MUST have the same size for this to work
CT_ASSERT(sizeof(data_ptr) == sizeof(proc_ptr), "pointer sizes mismatch")

// convert function pointer to data object pointer
static inline data_ptr proc2data_ptr(proc_ptr fn_p){
  union{
    proc_ptr fn_p ;
    data_ptr da_p ;
  } pp ;
  pp.fn_p = fn_p ;
  return pp.da_p ;
}

// convert data object pointer to function pointer
static inline proc_ptr data2proc_ptr(data_ptr da_p){
  union{
    proc_ptr fn_p ;
    data_ptr da_p ;
  } pp ;
  pp.da_p = da_p ;
  return pp.fn_p ;
}
