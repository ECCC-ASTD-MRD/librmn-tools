#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

#if (__STDC_VERSION__ >= 201112L)  // C11+
#define CT_ASSERT(e, message) _Static_assert(e, message) ;
#else
// inspired by https://www.pixelbeat.org/programming/gcc/static_assert.html
#define CT_ASSERT(e, message) extern char (*DuMmY_NaMe(void)) [sizeof(char[1 - 2*!(e)])] ;
#endif

typedef void *data_ptr ;
typedef int (* proc_ptr)() ;

// data and function pointers MUST have the same size for this to work
CT_ASSERT(sizeof(data_ptr) == sizeof(proc_ptr), "pointer sizes mismatch")

int to_int(float x){
  int result = x + .5f ;
  return result ;
}

float to_flt(int i){
  return (i * 1.0f) ;
}

data_ptr proc2data_ptr(proc_ptr fn_p){
  union{
    proc_ptr fn_p ;
    data_ptr da_p ;
  } pp ;
  pp.fn_p = fn_p ;
  return pp.da_p ;
}

proc_ptr data2proc_ptr(data_ptr da_p){
  union{
    proc_ptr fn_p ;
    data_ptr da_p ;
  } pp ;
  pp.da_p = da_p ;
  return pp.fn_p ;
}

// convert the return value of dlsym into a function pointer
proc_ptr fn_dlsym(void *handle, const char *symbol){
  void *data_ptr = dlsym(handle, symbol) ;
  return data2proc_ptr(data_ptr) ;
}

int main(int argc, char **argv){
  proc_ptr fn1 = (proc_ptr)&to_int ;
  proc_ptr fn2 = (proc_ptr)&to_flt ;

  fn1 = data2proc_ptr(proc2data_ptr((proc_ptr)&to_int)) ;
  fn2 = data2proc_ptr(proc2data_ptr((proc_ptr)&to_int)) ;

  fprintf(stderr, "syntax constructs test\n") ;
  fprintf(stderr, "address of function1 = %16.16lx\n", (uint64_t) fn1) ;
  fprintf(stderr, "address of function1 = %16.16lx\n", (uint64_t) &to_int) ;
  fprintf(stderr, "address of function1 = %16p\n", proc2data_ptr(fn1) ) ;
  // warning: ISO C forbids conversion of function pointer to object pointer type
//   fprintf(stderr, "address of function1 = %16p\n", (void *)fn1 ) ;

  fprintf(stderr, "\n") ;
  fprintf(stderr, "address of function2 = %16.16lx\n", (uint64_t) fn2) ;
  fprintf(stderr, "address of function2 = %16.16lx\n", (uint64_t) &to_flt) ;
  fprintf(stderr, "address of function2 = %16p\n", proc2data_ptr(fn2) ) ;
  return 0;
}
