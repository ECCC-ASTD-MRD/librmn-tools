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

// create new  Arg_list structure
// maxargs : maximum allowable number of arguments
// return  : address of allocated structure
Arg_list *Arg_list_init(int maxargs){
  size_t the_size = sizeof(Arg_list) + maxargs * sizeof(Argument) ;
  Arg_list *temp = (Arg_list *) malloc(the_size) ;

  if(temp != NULL){
    temp->maxargs = maxargs ;
    temp->numargs = 0 ;        // no actual arguments yet
    temp->result.name = 0 ;    // invalid name
    temp->result.resv = 0 ;
    temp->result.stat = 0 ;    // no error
    temp->result.kind = 0 ;    // invalid kind
  }
  return temp ;
}

// Arg_list *Arg_list_address(Arg_fn_list *c)     // get address of argument list
//   { return &(c->s) ; }

// create new  Arg_fn_list structure
// fn      : address of function to be called eventually
// maxargs : maximum allowable number of arguments
// return  : address of allocated structure
Arg_fn_list *Arg_init(Arg_fn fn, int maxargs){
  size_t the_size = sizeof(Arg_fn_list) + maxargs * sizeof(Argument) ;
  Arg_fn_list *temp = (Arg_fn_list *) malloc(the_size) ;

  if(temp != NULL){
    temp->fn = fn ;
    temp->s.maxargs = maxargs ;
    temp->s.numargs = 0 ;        // no actual arguments yet
    temp->s.result.name = 0 ;    // invalid name
    temp->s.result.resv = 0 ;
    temp->s.result.stat = 0 ;    // no error
    temp->s.result.kind = 0 ;    // invalid kind
  }
  return temp ;
}

// hash argument name
// up to 8 characters are kept, 7 bits per character
// name   : name of argument
// return : 56 bit hash
static int64_t hash_name(char *name){
  int i ;
  int64_t hash = 0 ;
  for(i=0 ; i<8 && name[i] != '\0' ; i++){
    hash = (hash <<7) + name[i] ;
  }
  for( ; i<8 ; i++) hash <<= 7 ;    // left align the 56 bit hash
  return hash ;
}
int64_t Arg_name_hash(char *name){
  return hash_name(name) ;
}

int Arg_name_hash_index(Arg_list *s, int64_t hash, uint32_t kind){
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name && kind == s->arg[i].kind) return i ;
  }
  return -1 ; // not found
}

// get position in argument list using name only
int Arg_name_pos(Arg_list *s, char *name){
  int64_t hash = hash_name(name) ;
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name) return i ;
  }
  return -1 ; // not found
}

// get position in argument list using name and kind
int Arg_name_index(Arg_list *s, char *name, uint32_t kind){
  int64_t hash = hash_name(name) ;
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name && kind == s->arg[i].kind) return i ;
  }
  return -1 ; // not found
}

static AnyType Arg_null = { .u64 = 0ul } ;

// get value from argument list using name and kind
AnyType Arg_value(Arg_list *s, char *name, uint32_t kind){
  int64_t hash = hash_name(name) ;
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name && kind == s->arg[i].kind){
      s->arg[i].stat = 0 ;
      s->result.stat = 0 ;
      return s->arg[i].value ;
    }
  }
  s->result.stat = 1 ;   // set error flags
  return Arg_null ;      // not found, "nullest" return value
}

// translate name hash to character string
// hash   : from hash_name()
// name   : character array that can receive at least 9 characters
void Arg_name(int64_t hash, unsigned char *name){
  int i ;
  for(i=0 ; i<8 ; i++){
    name[i] = (hash >> 49) & 0x7F ;
    hash <<= 7 ;
  }
  name[9] = '\0' ;   // make sure there is a null at then end of the name string
}

// check that argument names in s are consistent with expected name in names
// Arg_list [IN] : pointer to argument list struct
// names    [IN] : list of expected argument names
// ncheck   [IN] : number of positions to check (0 means s->numargs)
// return        : 0 if no error, 1 otherwise
int Arg_names_check(Arg_list *s, char **names, int ncheck){
  int i, errors = 0 ;
  unsigned char name[9] ;
  ncheck = (ncheck == 0) ? s->numargs : ncheck ;
  for(i=0 ; i<ncheck ; i++){
    if(hash_name(names[i]) != s->arg[i].name) {
      Arg_name(s->arg[i].name, name) ;
      fprintf(stderr,"argument %8s found at position %d instead of %s\n", name, i, names[i]) ;
      errors++ ;
    }
  }
  return (errors > 0) ? 1 : 0 ;
}

// check that argument kinds in s are consistent with expected kinds
// Arg_list [IN] : pointer to argument list struct
// names    [IN] : list of expected argument types
// ncheck   [IN] : number of positions to check (0 means s->numargs)
// return        : 0 if no error, 1 otherwise
int Arg_types_check(Arg_list *s, uint32_t *kind, int ncheck){
  int i, errors = 0 ;
  ncheck = (ncheck == 0) ? s->numargs : ncheck ;
  for(i=0 ; i<ncheck ; i++){
    if(kind[i] != s->arg[i].kind){
      fprintf(stderr,"argument %2d, expecting %s, got %s\n", i+1, Arg_kind[kind[i]], Arg_kind[s->arg[i].kind]) ;
      errors++ ;
    }
  }
  return (errors > 0) ? 1 : 0 ;
}

// dump names and types of arguments and result from Arg_list
void Arg_list_dump(Arg_list *s){
  unsigned char name[9] ;
  int i ;
  fprintf(stderr, "maxargs = %d, numargs = %d\n", s->maxargs, s->numargs) ;
  for(i=0 ; i<s->numargs ; i++){
    Arg_name(s->arg[i].name, name) ;
    fprintf(stderr, "argument %2d : name = %9s, kind = %4s\n", i+1, name, Arg_kind[s->arg[i].kind]) ;
  }
  fprintf(stderr, "function result kind = %s\n", Arg_kind[s->result.kind]) ;
}

// add argument to Arg_list
// v           [IN] : uint8_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint8(uint8_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u8 ;
  s->arg[numargs].value.u8  = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : int8_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int8(int8_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i8 ;
  s->arg[numargs].value.i8  = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : uint16_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint16(uint16_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u16 ;
  s->arg[numargs].value.u16 = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : int16_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int16(int16_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i16 ;
  s->arg[numargs].value.i16 = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : uint32_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint32(uint32_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u32 ;
  s->arg[numargs].value.u32 = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : int32_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int32(int32_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i32 ;
  s->arg[numargs].value.i32 = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : uint64_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint64(uint64_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u64 ;
  s->arg[numargs].value.u64 = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : int64_t value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int64(int64_t v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i64 ;
  s->arg[numargs].value.i64 = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : float value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_float(float v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_f ;
  s->arg[numargs].value.f = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : double value
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_double(double v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_d ;
  s->arg[numargs].value.d = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_ptr(void *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_p ;
  s->arg[numargs].value.p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to uint8_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint8p(uint8_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u8p ;
  s->arg[numargs].value.u8p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to int8_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int8p(int8_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i8p ;
  s->arg[numargs].value.i8p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to uint16_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint16p(uint16_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u16p ;
  s->arg[numargs].value.u16p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to int16_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int16p(int16_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i16p ;
  s->arg[numargs].value.i16p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to uint32_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint32p(uint32_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u32p ;
  s->arg[numargs].value.u32p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to int32_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int32p(int32_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i32p ;
  s->arg[numargs].value.i32p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to uint64_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint64p(uint64_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_u64p ;
  s->arg[numargs].value.u64p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to int64_t
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int64p(int64_t *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_i64p ;
  s->arg[numargs].value.i64p = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to float
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_floatp(float *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_fp ;
  s->arg[numargs].value.fp = v ;
  s->numargs++ ;
  return s->numargs ;
}

// add argument to Arg_list
// v           [IN] : pointer to double
// Arg_list [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_doublep(double *v, Arg_list *s, char *name){
  int numargs = s->numargs ;

  if(numargs >= s->maxargs) return -1 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = Arg_dp ;
  s->arg[numargs].value.dp = v ;
  s->numargs++ ;
  return s->numargs ;
}
