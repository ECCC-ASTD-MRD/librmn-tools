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
#include <rmn/tools_types.h>

Arg_callback *Arg_init(Arg_fn fn, int maxargs){
  size_t the_size = sizeof(Arg_callback) + maxargs * sizeof(Argument) ;
  Arg_callback *temp = (Arg_callback *) malloc(the_size) ;

  temp->fn = fn ;
  temp->s.maxargs = maxargs ;
  temp->s.numargs = 0 ;
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

int Arg_uint8(uint8_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u8 ;
  s->arg[numargs].value.u8  = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int8(int8_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i8 ;
  s->arg[numargs].value.i8  = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint16(uint16_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u16 ;
  s->arg[numargs].value.u16 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int16(int16_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i16 ;
  s->arg[numargs].value.i16 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint32(uint32_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u32 ;
  s->arg[numargs].value.u32 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int32(int32_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i32 ;
  s->arg[numargs].value.i32 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_uint64(uint64_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u64 ;
  s->arg[numargs].value.u64 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_int64(int64_t v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i64 ;
  s->arg[numargs].value.i64 = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_float(float v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_f ;
  s->arg[numargs].value.f = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_double(double v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_d ;
  s->arg[numargs].value.d = v ;
  s->numargs++ ;
  return s->numargs ;
}

int Arg_ptr(void *v, Arg_callback *c, char *name){
  Arg_stack *s = &(c->s) ;
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_p ;
  s->arg[numargs].value.p = v ;
  s->numargs++ ;
  return s->numargs ;
}
