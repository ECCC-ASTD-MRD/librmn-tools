
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

// the stat field may be used as a Read-Only lock for the arguments (status  for the result)
typedef struct{     // argument list element
  uint64_t name:56, // 8 x 7bit ASCII name
           undf:1 , // 1 undefined, 0 defined
           stat:1 , // status (0 = O.K., 1 = FAIL) (only used for result)
           kind:6 ; // type code (from ArgType)
  AnyType value ;   // argument value
} Argument ;

typedef struct{     // serialized argument list structure
  int32_t maxargs ; // max number of arguments permitted
  int32_t numargs ; // actual number of arguments
  Argument result ; // function result (also addressed as arg[-1])
  Argument arg[] ;  // the arguments (the first numargs entries are defined)
} Arg_list ;

// TO DO :
// add an inline function to copy an argument list into another one (maybe allocating the copy)

typedef AnyType (*Arg_fn)(Arg_list *) ;  // pointer to function with serialized argument list

typedef struct{    // serialized argument list control structure
  Arg_fn    fn ;   // function to be called, returns AnyType, takes pointer to Arg_list as only argument ;
  Arg_list s  ;    // argument list
} Arg_fn_list ;

Arg_fn_list *Arg_init(Arg_fn fn, int maxargs);               // create and initialize Arg_fn_list structure

// helper functions
AnyType Arg_value(Arg_list *s, char *name, uint32_t kind);   // get value from argument list using name and kind
int64_t Arg_name_hash(char *name);                           // get hash associated with name string
int Arg_name_hash_index(Arg_list *s, int64_t hash, uint32_t kind);   // get argument position in argument list using name hash
int Arg_name_index(Arg_list *s, char *name, uint32_t kind);          // get argument position in argument list using name
int Arg_name_pos(Arg_list *s, char *name);                   // get position of name in argument list
void Arg_name(int64_t hash, unsigned char *name);            // get name string from hash
int Arg_names_check(Arg_list *s, char **names, int ncheck);  // check argument names against expected names
int Arg_types_check(Arg_list *s, uint32_t *kind, int ncheck);     // check argument types against expected types
void Arg_list_dump(Arg_list *s);                             // dump argument names and types

// inline helpers
static inline Arg_list *Arg_list_address(Arg_fn_list *c)     // get address of argument list from Arg_fn_list
  { return &(c->s) ; }
static inline void Arg_result(ArgType kind, Arg_list *s)     // set result type in argument list
  { s->result.kind = kind ; }

// add arguments to argument list
int Arg_find_pos( Arg_list *s, char *name, uint32_t kind);   // find name/kind in argument list, add to list if not found

int Arg_uint8(uint8_t v, Arg_list *s, char *name);           // add unsigned 8 bit integer argument
int Arg_int8(int8_t v, Arg_list *s, char *name);             // add signed 8 bit integer argument
int Arg_uint16(uint16_t v, Arg_list *s, char *name);         // add unsigned 16 bit integer argument
int Arg_int16(int16_t v, Arg_list *s, char *name);           // add signed 16 bit integer argument
int Arg_uint32(uint32_t v, Arg_list *s, char *name);         // add unsigned 32 bit integer argument
int Arg_int32(int32_t v, Arg_list *s, char *name);           // add signed 32 bit integer argument
int Arg_uint64(uint64_t v, Arg_list *s, char *name);         // add unsigned 64 bit integer argument
int Arg_int64(int64_t v, Arg_list *s, char *name);           // add signed 64 bit integer argument
int Arg_float(float v, Arg_list *s, char *name);             // add 32 bit float argument
int Arg_double(double v, Arg_list *s, char *name);           // add 64 bit double argument
int Arg_ptr(void *v, Arg_list *s, char *name);               // add pointer argument
int Arg_uint8p(uint8_t *v, Arg_list *s, char *name);         // add pointer to unsigned 8 bit integer argument
int Arg_int8p(int8_t *v, Arg_list *s, char *name);           // add pointer to signed 8 bit integer argument
int Arg_uint16p(uint16_t *v, Arg_list *s, char *name);       // add pointer to unsigned 32 bit integer argument
int Arg_int16p(int16_t *v, Arg_list *s, char *name);         // add pointer to signed 32 bit integer argument
int Arg_uint32p(uint32_t *v, Arg_list *s, char *name);       // add pointer to unsigned 32 bit integer argument
int Arg_int32p(int32_t *v, Arg_list *s, char *name);         // add pointer to signed 32 bit integer argument
int Arg_uint64p(uint64_t *v, Arg_list *s, char *name);       // add pointer to unsigned 64 bit integer argument
int Arg_int64p(int64_t *v, Arg_list *s, char *name);         // add pointer to signed 64 bit integer argument
int Arg_floatp(float *v, Arg_list *s, char *name);           // add pointer to 32 bit float argument
int Arg_doublep(double *v, Arg_list *s, char *name);         // add pointer to 64 bit double argument

#endif
