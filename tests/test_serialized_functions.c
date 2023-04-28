#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rmn/test_helpers.h>
#include <rmn/serialized_functions.h>

Arg_fn_list *serialize_demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp);
AnyType call_demo_fn(Arg_list *list);
AnyType call_demo_fn2(Arg_list *list);
float *demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp);
float *demo_fn2(int8_t i8, int8_t *i8p, float  f, float *fp, double d, double *dp);

#if 1
// the useful function
float *demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp){
  void *r = fp ;
  fprintf(stderr,"\ni8 = %d, f = %f, d = %f, i8p = %p, fp = %p, dp = %p\n", i8, f, d, i8p, fp, dp) ;
  if(*i8p != i8) { fprintf(stderr,"ERROR: *i8p != i8\n") ; r = NULL ; }
  if(*fp  != f ) { fprintf(stderr,"ERROR: *fp != f\n")   ; r = NULL ; }
  if(*dp  != d ) { fprintf(stderr,"ERROR: *dp != d\n")   ; r = NULL ; }
  return r ;
}

// another function
float *demo_fn2(int8_t i8, int8_t *i8p, float  f, float *fp, double d, double *dp){
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
Arg_fn_list *serialize_demo_fn(int8_t i8, float  f, double d, int8_t *i8p, float *fp, double *dp){
  Arg_fn_list *c = Arg_init(call_demo_fn, 6) ;    // set target function and initialize structure
  Arg_list *s = Arg_list_address(c) ;             // address of argument list

  fprintf(stderr, "IN serialize_demo_fn, building argument list\n") ;
  Arg_int8(i8,    s, "arg_i8") ;                  // 8 bit signed integer argument
  Arg_float(f,    s, "arg_f") ;                   // 32 bit float argument
  Arg_double(d,   s, "arg_d") ;                   // 64 bit double argument
  Arg_int8p(i8p,  s, "arg_i8p") ;                 // pointer to 8 bit signed integer argument
  Arg_floatp(fp,  s, "arg_fp") ;                  // pointer to 32 bit float argument
  Arg_doublep(dp, s, "arg_dp") ;                  // pointer to 64 bit double argument
  Arg_result(Arg_fp, Arg_list_address(c)) ;       // deliberately using Arg_list_address() as parameter
  Arg_list_dump(s) ;
  return c ;
}

AnyType call_demo_fn2(Arg_list *list){
  AnyType t ;
  int i ;
  // expected argument types
  static uint32_t expected_kinds[]     = {Arg_i8, Arg_i8p, Arg_f, Arg_fp, Arg_d, Arg_dp} ;
  static uint32_t expected_kinds_bad[] = {Arg_i8, Arg_i8p, Arg_f, Arg_p,  Arg_d, Arg_p} ;
  static char *expected_names[] =     {"arg_i8", "arg_i8p", "arg_f", "arg_fp", "arg_d", "arg_dp"} ;
  static char *expected_names_bad[] = {"ARG_I8", "arg_i8p", "arg_f", "arg_fp", "arg_d", "arg_dp"} ;
  int ix[6] ;

  // arguments 1, 4 and 6 expected to have bad name/type combinations
  fprintf(stderr, "IN call_demo_fn2, expecting arguments 1, 4, 6 to be bad\n");
  for(i=0 ; i<6 ; i++){
    ix[i] = Arg_name_index(list, expected_names_bad[i], expected_kinds_bad[i]) ;
    if(ix[i] < 0) {
      int ixn, kind = 0 ;
      ixn = Arg_name_pos(list, expected_names_bad[i]) ;  // find name in argument list
      if(ixn > 0) kind = list->arg[ixn].kind ;       // if valid, get assocoated type
      fprintf(stderr, "ERROR: argument %d, name = %s, expecting kind = %s, got %s\n",
              i+1, expected_names_bad[i], Arg_kind[expected_kinds_bad[i]], Arg_kind[kind]) ;
    }
  }
  fprintf(stderr, "\nIN call_demo_fn2, get some argument values (with intentional errors)\n");
  int8_t i8 = Arg_value(list, "arg_i8", Arg_i8).i8 ;
  fprintf(stderr, "arg_i8(%s) = %d, error = %d\n", Arg_kind[Arg_i8], i8, list->result.stat) ;
  int8_t *i8p = Arg_value(list, "arg_i8p", Arg_i8p).i8p ;
  fprintf(stderr, "arg_i8p(%s) = %d, error = %d\n", Arg_kind[Arg_i8p], *i8p, list->result.stat) ;
  float *fp = Arg_value(list, "arg_fp", Arg_fp).fp ;
  fprintf(stderr, "arg_fp(%s) = %.2f, error = %d\n", Arg_kind[Arg_fp], *fp, list->result.stat) ;
  double *dp = Arg_value(list, "arg_dp", Arg_dp).dp ;
  fprintf(stderr, "arg_dp(%s) = %.2f, error = %d\n", Arg_kind[Arg_dp], *dp, list->result.stat) ;
  i8 = Arg_value(list, "arg_i8", Arg_i16).i8 ;
  fprintf(stderr, "arg_i8(%s) = %d, error = %d (expected error)\n", Arg_kind[Arg_i16], i8, list->result.stat) ;
  fprintf(stderr, "\nIN call_demo_fn2, reorder index =");
  for(i=0 ; i<6 ; i++){    // reorder argument list
    ix[i] = Arg_name_index(list, expected_names[i], expected_kinds[i]) ;
    fprintf(stderr, " %d", ix[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "\nIN call_demo_fn2, calling demo_fn2\n") ;
  t.fp = demo_fn2(list->arg[ix[0]].value.i8, 
                  list->arg[ix[1]].value.i8p, 
                  list->arg[ix[2]].value.f, 
                  list->arg[ix[3]].value.fp, 
                  list->arg[ix[4]].value.d, 
                  list->arg[ix[5]].value.dp ) ;
  return t ;
}
// the wrapper function target
// - gets the argument list
// - checks for proper number of arguments
// - calls the useful function with arguments from serialized argument list
AnyType call_demo_fn(Arg_list *list){
  AnyType t ;
  int i ;
  // expected argument types
  static uint32_t expected_kinds[] = {Arg_i8, Arg_f, Arg_d, Arg_i8p, Arg_fp, Arg_dp} ;
  static char *expected_names[] = {"arg_i8", "arg_f", "arg_d", "arg_i8p", "arg_fp", "arg_dp"} ;
// fprintf(stderr,"DEBUG : callee list = %16p, maxargs = %d, numargs = %d\n", list, list->maxargs, list->numargs);
  fprintf(stderr, "IN call_demo_fn\n");
  if(list->result.kind == 0) {
    fprintf(stderr,"INFO: result type not specified, assuming generic pointer\n") ;
    list->result.kind = Arg_p ;
  }else{
    if(list->result.kind  != Arg_fp){
      fprintf(stderr,"WARNING: expecting result type = %d, got %d\n", Arg_p, list->result.kind) ;
      t.fp = NULL ;
      return t ;
    }
  }
  if(list->numargs == 6){
    if(0 == Arg_names_check(list, expected_names, 0)) fprintf(stderr,"Arg_names_check O.K.\n") ;
    if(0 == Arg_types_check(list, expected_kinds, 0)) fprintf(stderr,"Arg_types_check O.K.\n") ;
    for(i=0 ; i<list->numargs ; i++){
    }
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
#endif
int main(int argc, char **argv){
  Arg_fn_list *c ;
  Arg_list *l ;
  int8_t i8 = -123 ;
  float  f  = 123.45f ;
  double d  = 567.89 ;
  AnyType r ;
  float *fp = NULL ;
  int i ;

  start_of_test(argv[0]);
  c = serialize_demo_fn(i8, f, d, &i8, &f, &d) ;
  l = Arg_list_address(c) ;
//   fprintf(stderr,"DEBUG: dumping argument list\n") ;
//   Arg_list_dump(l) ;
// fprintf(stderr,"DEBUG : c->s = %16p, maxargs = %d(%p), numargs = %d(%p), result (%p), arg (%p)\n", 
//         &(c->s), c->s.maxargs, &(c->s.maxargs), c->s.numargs, &(c->s.numargs), &(c->s.result), &(c->s.arg[0])) ;
// fprintf(stderr,"DEBUG : list = %16p, maxargs = %d, numargs = %d\n", l, l->maxargs, l->numargs) ;
// fprintf(stderr,"DEBUG: caller list address = %p\n", l);
  fprintf(stderr,"==================================================================\n");
  fprintf(stderr,"using SERIALIZED_FUNCTION() to call call_demo_fn\n") ;
  r = SERIALIZED_FUNCTION(c) ;
  fp = (float *) r.p ;
  if(fp == NULL){
    fprintf(stderr,"ERROR: called function returned NULL\n") ;
    exit(1) ;
  }
  fprintf(stderr,"function result = %p, float value = %f\n", r.fp, *fp) ;
  fprintf(stderr,"==================================================================\n");
  fprintf(stderr,"direct call to call_demo_fn reusing previous argument list\n") ;
  l = Arg_list_address(c) ;
  r = call_demo_fn(l) ;
  fp = (float *) r.p ;
  fprintf(stderr,"function result = %p, float value = %f\n", r.fp, *fp) ;
  fprintf(stderr,"==================================================================\n");
  r = call_demo_fn2(l) ;
  fp = (float *) r.p ;
  fprintf(stderr,"function2 result = %p, float value = %f\n", r.fp, *fp) ;
  fprintf(stderr,"==================================================================\n");
  fprintf(stderr,"kind and name table\n") ;
  for(i=0 ; i<=Arg_void ; i++){
    fprintf(stderr, "kind = %2d, name = %4s\n", i, Arg_kind[i]) ;
  }
  return 0 ;
}
