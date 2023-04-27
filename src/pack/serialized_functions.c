// Hopefully useful code for C (dyanmic function arguments)
// Copyright (C) 2022  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rmn/serialized_functions.h>

Arg_list *Arg_list_init(int maxargs){
  size_t the_size = sizeof(Arg_list) + maxargs * sizeof(Argument) ;
  Arg_list *temp = (Arg_list *) malloc(the_size) ;
  temp->maxargs = maxargs ;
  temp->numargs = 0 ;        // no actual arguments yet
  temp->result = 0 ;         // invalid kind
  return temp ;
}

Arg_fn_list *Arg_init(Arg_fn fn, int maxargs){
  size_t the_size = sizeof(Arg_fn_list) + maxargs * sizeof(Argument) ;
  Arg_fn_list *temp = (Arg_fn_list *) malloc(the_size) ;

  temp->fn = fn ;
  temp->s.maxargs = maxargs ;
  temp->s.numargs = 0 ;        // no actual arguments yet
  temp->s.result = 0 ;         // invalid kind
  return temp ;
}

static int hash_name(char *name){
  int i ;
  int hash = 0 ;
  for(i=0 ; i<8 && name[i] != '\0' ; i++){
    hash = (hash <<7) + name[i] ;
  }
  return hash ;
}

int Arg_uint8(uint8_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u8 ;
  s->arg[numargs].value.u8  = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int8(int8_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i8 ;
  s->arg[numargs].value.i8  = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint16(uint16_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u16 ;
  s->arg[numargs].value.u16 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int16(int16_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i16 ;
  s->arg[numargs].value.i16 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint32(uint32_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u32 ;
  s->arg[numargs].value.u32 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int32(int32_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i32 ;
  s->arg[numargs].value.i32 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint64(uint64_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u64 ;
  s->arg[numargs].value.u64 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int64(int64_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i64 ;
  s->arg[numargs].value.i64 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_float(float v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_f ;
  s->arg[numargs].value.f = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_double(double v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_d ;
  s->arg[numargs].value.d = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_ptr(void *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_p ;
  s->arg[numargs].value.p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint8p(uint8_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u8p ;
  s->arg[numargs].value.u8p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int8p(int8_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i8p ;
  s->arg[numargs].value.i8p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint16p(uint16_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u16p ;
  s->arg[numargs].value.u16p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int16p(int16_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i16p ;
  s->arg[numargs].value.i16p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint32p(uint32_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u32p ;
  s->arg[numargs].value.u32p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int32p(int32_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i32p ;
  s->arg[numargs].value.i32p = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_floatp(float *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_fp ;
  s->arg[numargs].value.fp = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_doublep(double *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_dp ;
  s->arg[numargs].value.dp = v ;
  s->numargs++ ;
  return s->numargs ;
}
