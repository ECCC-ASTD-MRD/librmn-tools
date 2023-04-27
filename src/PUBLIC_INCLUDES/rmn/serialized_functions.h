
#if ! defined(TOOLS_FUNCTION_TYPES)
#define TOOLS_FUNCTION_TYPES

#include <rmn/tools_types.h>

// issue a call to the function targeted by Arg_fn_list structure c
#define CALL_SERIALIZED(c) c->fn(&(c->s))
// #define ARG_LIST_POINTER(c) (&(c->s))

Arg_fn_list *Arg_init(Arg_fn fn, int maxargs);            // initialize Arg_fn_list structure
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
