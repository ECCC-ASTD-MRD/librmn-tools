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

// create new  Arg_list structure (argument list only)
// maxargs : maximum allowable number of arguments
// return  : address of allocated structure
Arg_list *Arg_list_init(int maxargs){
  size_t the_size = sizeof(Arg_list) + maxargs * sizeof(Argument) ;
  Arg_list *temp = (Arg_list *) malloc(the_size) ;

  if(temp != NULL){
    temp->maxargs = maxargs ;
    temp->numargs = 0 ;        // no actual arguments yet
    temp->result.name = 0 ;    // invalid name
    temp->result.undf = 1 ;    // not defined
    temp->result.stat = 0 ;    // no error
    temp->result.kind = 0 ;    // invalid kind
  }
  return temp ;
}

// create new  Arg_fn_list structure (function address + argument list)
// fn      : address of function to be called eventually
// maxargs : maximum allowable number of arguments
// return  : address of allocated structure
Arg_fn_list *Arg_init(Arg_fn fn, int maxargs){
  size_t the_size = sizeof(Arg_fn_list) + maxargs * sizeof(Argument) ;
  Arg_fn_list *temp = (Arg_fn_list *) malloc(the_size) ;
  int i ;

  if(temp != NULL){
    temp->fn = fn ;
    temp->s.maxargs = maxargs ;
    temp->s.numargs = 0 ;        // no actual arguments yet
    temp->s.result.name = 0 ;    // invalid name
    temp->s.result.stat = 0 ;    // no error
    temp->s.result.undf = 1 ;    // not defined
    temp->s.result.kind = 0 ;    // invalid kind
    for(i=0 ; i<maxargs ; i++) temp->s.arg[i].undf = 1 ;  // set all arguments to undefined
  }
  return temp ;
}

// hash argument name (8 characters -> 56 bit integer)
// up to 8 characters are kept, 7 bits per character
// name   : character array up to 8 characters (name of argument)
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
// public version of hash_name
int64_t Arg_name_hash(char *name){
  return hash_name(name) ;
}

// find argument index in argument list using argument name hash and kind
// s    [IN] : pointer to argument list struct
// hash [IN] : from hash_name()
// kind [IN] : expected argument type
int Arg_name_hash_index(Arg_list *s, int64_t hash, uint32_t kind){
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name && kind == s->arg[i].kind) return i ;
  }
  return -1 ; // not found
}

// find argument index in argument list using argument name only
// s    [IN] : pointer to argument list struct
// name [IN] : character array up to 8 characters (name of argument)
int Arg_name_pos(Arg_list *s, char *name){
  int64_t hash = hash_name(name) ;
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name) return i ;
  }
  return -1 ; // not found
}

// find argument index in argument list using argument name and kind
// s    [IN] : pointer to argument list struct
// name [IN] : character array up to 8 characters (name of argument)
// kind [IN] : expected argument type
int Arg_name_index(Arg_list *s, char *name, uint32_t kind){
  int64_t hash = hash_name(name) ;
  int i ;
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name && kind == s->arg[i].kind) return i ;
  }
  return -1 ; // not found
}

static AnyType Arg_null = { .u64 = 0ul } ;

// get value from argument list using name and kind (union is returned)
// s    [IN] : pointer to argument list struct
// name [IN] : character array up to 8 characters (name of argument)
// N.B. it is up to the caller to get the desired field from the returned union
AnyType Arg_value(Arg_list *s, char *name, uint32_t kind){
  int64_t hash = hash_name(name) ;
  int i ;
  if(name == NULL){
    s->result.stat = s->result.undf ;      // status = O.K. if defined
    return s->result.value ;               // function result (if defined)
  }
  for(i=0 ; i<s->numargs ; i++){
    if(hash == s->arg[i].name && kind == s->arg[i].kind){
      s->result.stat = s->arg[i].undf ;     // status = O.K. if defined
      return s->arg[i].value ;
    }
  }
  s->result.stat = 1 ;   // set error flag
  return Arg_null ;      // not found, "nullest" return value
}

// translate name hash into character string (9 characters, null terminated)
// hash  [IN] : from hash_name()
// name [OUT] : character array that can receive at least 9 characters
void Arg_name(int64_t hash, unsigned char *name){
  int i ;
  for(i=0 ; i<8 ; i++){
    name[i] = (hash >> 49) & 0x7F ;
    hash <<= 7 ;
  }
  name[9] = '\0' ;   // make sure there is a null at then end of the name string
}

// check that argument names in struct s are consistent with expected name in names[]
// s        [IN] : pointer to argument list struct
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
      fprintf(stderr,"ERROR: argument %8s found at position %d instead of %s\n", name, i, names[i]) ;
      errors++ ;
    }
  }
  return (errors > 0) ? 1 : 0 ;
}

// check that argument kinds in struct s are consistent with expected kind in kinds[]
// s        [IN] : pointer to argument list struct
// kinds    [IN] : list of expected argument types
// ncheck   [IN] : number of positions to check (0 means s->numargs)
// return        : 0 if no error, 1 otherwise
int Arg_types_check(Arg_list *s, uint32_t *kinds, int ncheck){
  int i, errors = 0 ;
  ncheck = (ncheck == 0) ? s->numargs : ncheck ;
  for(i=0 ; i<ncheck ; i++){
    if(kinds[i] != s->arg[i].kind){
      fprintf(stderr,"ERROR: argument %2d, expecting %s, got %s\n", i+1, Arg_kind[kinds[i]], Arg_kind[s->arg[i].kind]) ;
      errors++ ;
    }
  }
  return (errors > 0) ? 1 : 0 ;
}

// dump names and types of arguments and result from Arg_list
// s        [IN] : pointer to argument list struct
void Arg_list_dump(Arg_list *s){
  unsigned char name[9] ;
  int i ;
  fprintf(stderr, "max number of arguments = %d, actual number of arguments = %d\n", s->maxargs, s->numargs) ;
  for(i=0 ; i<s->numargs ; i++){
    Arg_name(s->arg[i].name, name) ;
    fprintf(stderr, "argument %2d : name = %9s, kind = %4s\n", i+1, name, Arg_kind[s->arg[i].kind]) ;
  }
  fprintf(stderr, "function result kind = %s\n", Arg_kind[s->result.kind]) ;
}

// find name/kind in argument table, add entry if not found
// s     [INOUT] : pointer to argument list struct
// name     [IN] : expected argument name, up to 8 characters
// kind     [IN] : expected argument type
// return (insertion) position (-1 in case of error)
// if it is a new argument, add name/kind/flags to the argument list
// the actual value will be set by the calling function
// this function is used by all the Arg_xxx add argument functions
int Arg_find_pos( Arg_list *s, char *name, uint32_t kind){
  int i ;
  int64_t hash = hash_name(name) ;
  int numargs = s->numargs ;
  for(i=0 ; i<numargs ; i++){
    if(s->arg[i].name == hash){         // name found
      if(s->arg[i].kind == kind){
// fprintf(stderr, "DEBUG: found argument %s(%s) as argument %d\n", name, Arg_kind[kind], i+1) ;
        s->arg[i].undf = 0 ;            // flags as if it were a new argument
        s->arg[numargs].stat = 0 ;
        return i ;                      // expected type
      }else{
        return -1 ;                     // wrong type, error
      }
    }
  }
  if(numargs >= s->maxargs) return -1 ; // argument table is full
// fprintf(stderr, "DEBUG: adding argument %s(%s) as argument %d\n", name, Arg_kind[kind], numargs+1) ;
  s->arg[numargs].undf      = 0 ;
  s->arg[numargs].stat      = 0 ;
  s->arg[numargs].name      = hash_name(name) ;
  s->arg[numargs].kind      = kind ;
  s->numargs++ ;                        // bump number of arguments
  return numargs ;                      // position for insertion
}

// add uint8_t argument to Arg_list, return number of arguments
// v           [IN] : uint8_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint8(uint8_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u8) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u8  = v ;
  return s->numargs ;
}

// add int8_t argument to Arg_list, return number of arguments
// v           [IN] : int8_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int8(int8_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i8) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i8  = v ;
  return s->numargs ;
}

// add uint16_t argument to Arg_list, return number of arguments
// v           [IN] : uint16_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint16(uint16_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u16) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u16  = v ;
  return s->numargs ;
}

// add int16_t argument to Arg_list, return number of arguments
// v           [IN] : int16_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int16(int16_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i16) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i16  = v ;
  return s->numargs ;
}

// add uint32_t argument to Arg_list, return number of arguments
// v           [IN] : uint32_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint32(uint32_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u32) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u32  = v ;
  return s->numargs ;
}

// add int32_t argument to Arg_list, return number of arguments
// v           [IN] : int32_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int32(int32_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i32) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i32  = v ;
  return s->numargs ;
}

// add uint64_t argument to Arg_list, return number of arguments
// v           [IN] : uint64_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint64(uint64_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u64) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u64  = v ;
  return s->numargs ;
}

// add int64_t argument to Arg_list, return number of arguments
// v           [IN] : int64_t value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int64(int64_t v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i64) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i64  = v ;
  return s->numargs ;
}

// add float argument to Arg_list, return number of arguments
// v           [IN] : float value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_float(float v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_f) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.f  = v ;
  return s->numargs ;
}

// add double argument to Arg_list, return number of arguments
// v           [IN] : double value
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_double(double v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_d) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.d  = v ;
  return s->numargs ;
}

// add void * argument to Arg_list, return number of arguments
// v           [IN] : pointer
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_ptr(void *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.p  = v ;
  return s->numargs ;
}

// add uint8_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to uint8_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint8p(uint8_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u8p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u8p  = v ;
  return s->numargs ;
}

// add int8_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to int8_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int8p(int8_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i8p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i8p  = v ;
  return s->numargs ;
}

// add uint16_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to uint16_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint16p(uint16_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u16p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u16p  = v ;
  return s->numargs ;
}

// add int16_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to int16_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int16p(int16_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i16p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i16p  = v ;
  return s->numargs ;
}

// add uint32_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to uint32_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint32p(uint32_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u32p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u32p  = v ;
  return s->numargs ;
}

// add int32_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to int32_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int32p(int32_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i32p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i32p  = v ;
  return s->numargs ;
}

// add uint64_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to uint64_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_uint64p(uint64_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_u64p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.u64p  = v ;
  return s->numargs ;
}

// add int64_t * argument to Arg_list, return number of arguments
// v           [IN] : pointer to int64_t
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_int64p(int64_t *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_i64p) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.i64p  = v ;
  return s->numargs ;
}

// add float * argument to Arg_list, return number of arguments
// v           [IN] : pointer to float
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_floatp(float *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_fp) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.fp  = v ;
  return s->numargs ;
}

// add double * argument to Arg_list, return number of arguments
// v           [IN] : pointer to double
// s        [INOUT] : pointer to argument list struct
// name        [IN] : argument name, up to 8 characters
int Arg_doublep(double *v, Arg_list *s, char *name){
  int numargs = Arg_find_pos(s, name, Arg_dp) ;
  if(numargs == -1) return -1 ;
  s->arg[numargs].value.dp  = v ;
  return s->numargs ;
}
