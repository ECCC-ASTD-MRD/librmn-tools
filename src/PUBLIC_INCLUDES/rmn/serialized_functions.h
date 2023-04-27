
#if ! defined(SERIALIZED_FUNCTION)

#include <rmn/tools_types.h>

// call the function targeted by Arg_fn_list structure c
#define SERIALIZED_FUNCTION(c) c->fn(&(c->s))

// start enum at value 1, 0 is used as invalid/uninitialized
typedef enum{
  Arg_u8 = 1 , // 8/16/32/64 unsigned integer
  Arg_u16  ,
  Arg_u32  ,
  Arg_u64  ,
  Arg_i8   ,   // 8/16/32/64 signed integer
  Arg_i16  ,
  Arg_i32  ,
  Arg_i64  ,
  Arg_f    ,   // 32 bit float
  Arg_d    ,   // 64 bit float
  Arg_p    ,   // generic memory pointer
  Arg_u8p ,    // pointer to 8 bit unsigned integer
  Arg_i8p ,    // pointer to 8 bit signed integer
  Arg_u16p ,   // pointer to 16 bit unsigned integer
  Arg_i16p ,   // pointer to 16 bit signed integer
  Arg_u32p ,   // pointer to 32 bit unsigned integer
  Arg_i32p ,   // pointer to 32 bit signed integer
  Arg_u64p ,   // pointer to 64 bit unsigned integer
  Arg_i64p ,   // pointer to 64 bit signed integer
  Arg_fp   ,   // pointer to 32 bit float
  Arg_dp   ,   // pointer to 64 bit double
  Arg_void     // no return value
} ArgType ;

static char *Arg_kind[] = {
  "INVALID" ,
  "u8"   ,
  "u16"  ,
  "u32"  ,
  "u64"  ,
  "i8"   ,
  "i16"  ,
  "i32"  ,
  "i64"  ,
  "f"    ,
  "d"    ,
  "p"    ,
  "u8p"  ,
  "i8p"  ,
  "u16p" ,
  "i16p" ,
  "u32p" ,
  "i32p" ,
  "u64p" ,
  "i64p" ,
  "fp"   ,
  "dp"   ,
  "void"
} ;

typedef struct{     // argument list element
  uint64_t name:56, // 8 x 7bit ASCII name
           kind:8 ; // type code (from ArgType)
  AnyType value ;   // argument value
} Argument ;

typedef struct{     // serialized argument list structure
  int maxargs ;     // max number of arguments permitted
  int numargs:24 ,  // actual number of arguments
      result:8 ;    // return value type
  Argument arg[] ;
} Arg_list ;

typedef AnyType (*Arg_fn)(Arg_list *) ;  // pointer to function with serialized argument list

typedef struct{    // serialized argument list control structure
  Arg_fn    fn ;   // function to be called, returns AnyType, takes pointer to Arg_list as only argument ;
  Arg_list s  ;    // argument list
} Arg_fn_list ;

Arg_fn_list *Arg_init(Arg_fn fn, int maxargs);            // initialize Arg_fn_list structure
void Arg_name(int64_t hash, unsigned char *name);         // get name string from hash
void Arg_list_dump(Arg_list *s);                          // dump argument names and types
static inline Arg_list *Arg_list_address(Arg_fn_list *c)  // get address of argument list
  { return &(c->s) ; }
static inline void Arg_result(ArgType kind, Arg_list *s)   // set result type in argument list
  { s->result = kind ; }
int Arg_uint8(uint8_t v, Arg_list *s, char *name);     // add unsigned 8 bit integer argument
int Arg_int8(int8_t v, Arg_list *s, char *name);       // add signed 8 bit integer argument
int Arg_uint16(uint16_t v, Arg_list *s, char *name);   // add unsigned 16 bit integer argument
int Arg_int16(int16_t v, Arg_list *s, char *name);     // add signed 16 bit integer argument
int Arg_uint32(uint32_t v, Arg_list *s, char *name);   // add unsigned 32 bit integer argument
int Arg_int32(int32_t v, Arg_list *s, char *name);     // add signed 32 bit integer argument
int Arg_uint64(uint64_t v, Arg_list *s, char *name);   // add unsigned 64 bit integer argument
int Arg_int64(int64_t v, Arg_list *s, char *name);     // add signed 64 bit integer argument
int Arg_float(float v, Arg_list *s, char *name);       // add 32 bit float argument
int Arg_double(double v, Arg_list *s, char *name);     // add 64 bit double argument
int Arg_ptr(void *v, Arg_list *s, char *name);         // add pointer argument
int Arg_uint8p(uint8_t *v, Arg_list *s, char *name);   // add pointer to unsigned 8 bit integer argument
int Arg_int8p(int8_t *v, Arg_list *s, char *name);     // add pointer to signed 8 bit integer argument
int Arg_uint16p(uint16_t *v, Arg_list *s, char *name); // add pointer to unsigned 32 bit integer argument
int Arg_int16p(int16_t *v, Arg_list *s, char *name);   // add pointer to signed 32 bit integer argument
int Arg_uint32p(uint32_t *v, Arg_list *s, char *name); // add pointer to unsigned 32 bit integer argument
int Arg_int32p(int32_t *v, Arg_list *s, char *name);   // add pointer to signed 32 bit integer argument
int Arg_uint64p(uint64_t *v, Arg_list *s, char *name); // add pointer to unsigned 64 bit integer argument
int Arg_int64p(int64_t *v, Arg_list *s, char *name);   // add pointer to signed 64 bit integer argument
int Arg_floatp(float *v, Arg_list *s, char *name);     // add pointer to 32 bit float argument
int Arg_doublep(double *v, Arg_list *s, char *name);   // add pointer to 64 bit double argument

#endif
