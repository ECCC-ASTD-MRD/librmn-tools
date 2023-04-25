#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rmn/test_helpers.h>
#include <rmn/tools_function_call.h>

AnyType demo_fn(Arg_stack *stack){
  AnyType t ;
  float *p = NULL ;

fprintf(stderr,"entering demo_fn\n") ;
fprintf(stderr,"number of actual arguments = %d (max = %d)\n", stack->numargs, stack->maxargs) ;
fprintf(stderr,"argument 1 = %d, kind = %d\n", stack->arg[0].value.i8, stack->arg[0].kind) ;
fprintf(stderr,"argument 2 = %f, kind = %d\n", stack->arg[1].value.f, stack->arg[1].kind) ;
fprintf(stderr,"argument 3 = %f, kind = %d\n", stack->arg[2].value.d, stack->arg[2].kind) ;
  p = stack->arg[3].value.p ;
fprintf(stderr,"argument 4 = %p, value = %f\n", p, *p) ;
  t.p = (void *) p ;
  return t ;
}

Arg_callback *call_demo_fn(){
  Arg_callback *c = Arg_init(demo_fn, 10) ;
  int8_t i8 = -123 ;
  float  f  = 123.45f ;
  double d  = 567.89 ;
  void *p   = (void *) &f ;

  Arg_int8(i8,  c, "tagada") ;
  fprintf(stderr,"argument 1 set to %d\n", i8) ;
  Arg_float(f,  c, "shimboum") ;
  fprintf(stderr,"argument 2 set to %f\n", f) ;
  Arg_double(d, c, "tsoin") ;
  fprintf(stderr,"argument 3 set to %f\n", d) ;
  Arg_ptr(p,    c, "p_to_f") ;
  fprintf(stderr,"argument 4 (pointer to argument 2) set to %p\n", p) ;
  return c ;
}

int main(int argc, char **argv){
  Arg_callback *c ;
  AnyType r ;
  float *fp = NULL ;

  start_of_test(argv[0]);
  c = call_demo_fn() ;
  r = c->fn(&(c->s)) ;
  fp = (float *) r.p ;
  fprintf(stderr,"function result = %p, float value = %f\n", r.p, *fp) ;
  return 0 ;
}
