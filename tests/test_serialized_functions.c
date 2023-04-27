#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rmn/test_helpers.h>
#include <rmn/serialized_functions.h>

Arg_fn_list *wrap_demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp);
AnyType call_demo_fn(Arg_list *list);
float *demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp);

// the useful function
float *demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp){
  void *r = fp ;
  fprintf(stderr,"\ni8 = %d, f = %f, d = %f, i8p = %p, fp = %p, dp = %p\n", i8, f, d, i8p, fp, dp) ;
  if(*i8p != i8) { fprintf(stderr,"ERROR: *i8p != i8\n") ; r = NULL ; }
  if(*fp  != f ) { fprintf(stderr,"ERROR: *fp != f\n")   ; r = NULL ; }
  if(*dp  != d ) { fprintf(stderr,"ERROR: *dp != d\n")   ; r = NULL ; }
  return r ;
}

// the function wrapper to generate the Arg_fn_list structure,
// targets call_demo_fn
// builds the argument list
Arg_fn_list *wrap_demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp){
  Arg_fn_list *c = Arg_init(call_demo_fn, 6) ;    // set target function and initialize structure
  Arg_list *s = Arg_list_address(c) ;             // address of argument list
  Arg_int8(i8,    s, "i8") ;                      // 8 bit signed integer argument
  Arg_float(f,    s, "f") ;                       // 32 bit float argument
  Arg_double(d,   s, "d") ;                       // 64 bit double argument
  Arg_int8p(i8p,  s, "i8p") ;                     // pointer to 8 bit signed integer argument
  Arg_floatp(fp,  s, "fp") ;                      // pointer to 32 bit float argument
  Arg_doublep(dp, s, "dp") ;                      // pointer to 64 bit double argument
  Arg_result(Arg_fp, Arg_list_address(c)) ;
  return c ;
}

// the wrapper function target, gets the argument list, calls the useful function
AnyType call_demo_fn(Arg_list *list){
  AnyType t ;

  if(list->result == 0) {
    fprintf(stderr,"INFO: result type not specified, assuming generic pointer") ;
  }else{
    if(list->result  != Arg_fp){
      fprintf(stderr,"WARNING: expecting result type = %d, got %d\n", Arg_p, list->result) ;
      t.fp = NULL ;
      return t ;
    }
  }
  if(list->numargs == 6){
    t.fp = demo_fn(list->arg[0].value.i8, 
                   list->arg[1].value.f, 
                   list->arg[2].value.d, 
                   list->arg[3].value.i8p, 
                   list->arg[4].value.fp, 
                   list->arg[5].value.dp ) ;
  }else{
    fprintf(stderr,"ERROR: wrong number of arguments, expecting 6, got %d\n", list->numargs) ;
    t.fp = NULL ;
  }
  return t ;
}

int main(int argc, char **argv){
  Arg_fn_list *c ;
  Arg_list *l ;
  int8_t i8 = -123 ;
  float  f  = 123.45f ;
  double d  = 567.89 ;
  AnyType r ;
  float *fp = NULL ;

  start_of_test(argv[0]);
  c = wrap_demo_fn(i8, f, d, &i8, &f, &d) ;
  r = CALL_SERIALIZED(c) ;
  fp = (float *) r.p ;
  if(fp == NULL){
    fprintf(stderr,"ERROR: called function returned NULL\n") ;
    exit(1) ;
  }
  fprintf(stderr,"function result = %p, float value = %f\n", r.fp, *fp) ;
  fprintf(stderr,"==================================================================\n");
  l = &(c->s) ;
  r = call_demo_fn(l) ;
  fp = (float *) r.p ;
  fprintf(stderr,"function result = %p, float value = %f\n", r.fp, *fp) ;
  return 0 ;
}
