#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rmn/test_helpers.h>
#include <rmn/tools_function_call.h>

Arg_callback *wrap_demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp);
AnyType call_demo_fn(Arg_list *list);
void *demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp);

// the useful function
void *demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp){
  void *r = fp ;
  fprintf(stderr,"\ni8 = %d, f = %f, d = %f, i8p = %p, fp = %p, dp = %p\n", i8, f, d, i8p, fp, dp) ;
  if(*i8p != i8) { fprintf(stderr,"ERROR: *i8p != i8\n") ; r = NULL ; }
  if(*fp  != f ) { fprintf(stderr,"ERROR: *fp != f\n")   ; r = NULL ; }
  if(*dp  != d ) { fprintf(stderr,"ERROR: *dp != d\n")   ; r = NULL ; }
  return r ;
}

// the function wrapper to generate the Arg_callback structure,
// targets call_demo_fn
// builds the argument list
Arg_callback *wrap_demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp){
  Arg_callback *c = Arg_init(call_demo_fn, 6) ;   // target function
  Arg_int8(i8,    c, "i8") ;                      // 8 bit signed integer argument
  Arg_float(f,    c, "f") ;                       // 32 bit float argument
  Arg_double(d,   c, "d") ;                       // 64 bit double argument
  Arg_int8p(i8p,  c, "i8p") ;                     // pointer to 8 bit signed integer argument
  Arg_floatp(fp,  c, "fp") ;                      // pointer to 32 bit float argument
  Arg_doublep(dp, c, "dp") ;                      // pointer to 64 bit double argument
  return c ;
}

// the wrapper function target, gets the argument list, calls the useful function
AnyType call_demo_fn(Arg_list *list){
  AnyType t ;

  if(list->numargs == 6){
    t.p = demo_fn(list->arg[0].value.i8, 
                  list->arg[1].value.f, 
                  list->arg[2].value.d, 
                  list->arg[3].value.i8p, 
                  list->arg[4].value.fp, 
                  list->arg[5].value.dp ) ;
  }else{
    fprintf(stderr,"ERROR: wrong number of arguments, expecting 6, got %d\n", list->numargs) ;
    t.p = NULL ;
  }
  return t ;
}

int main(int argc, char **argv){
  Arg_callback *c ;
  int8_t i8 = -123 ;
  float  f  = 123.45f ;
  double d  = 567.89 ;
  AnyType r ;
  float *fp = NULL ;

  start_of_test(argv[0]);
  c = wrap_demo_fn(i8, f, d, &i8, &f, &d) ;
  r = CALL_BACK(c) ;
  fp = (float *) r.p ;
  if(fp == NULL){
    fprintf(stderr,"ERROR: called function returned NULL\n") ;
    exit(1) ;
  }
  fprintf(stderr,"function result = %p, float value = %f\n", r.p, *fp) ;
  return 0 ;
}
