
#if ! defined(TOOLS_FUNCTION_TYPES)
#define TOOLS_FUNCTION_TYPES

#include <rmn/tools_types.h>

// issue a call to the function targeted by Arg_callback structure c
#define CALL_BACK(c) c->fn(&(c->s))

Arg_callback *Arg_init(Arg_fn fn, int maxargs);            // initialize Arg_callback structure
int Arg_uint8(uint8_t v, Arg_callback *c, char *name);     // add unsigned 8 bit integer argument
int Arg_int8(int8_t v, Arg_callback *c, char *name);       // add signed 8 bit integer argument
int Arg_uint16(uint16_t v, Arg_callback *c, char *name);   // add unsigned 16 bit integer argument
int Arg_int16(int16_t v, Arg_callback *c, char *name);     // add signed 16 bit integer argument
int Arg_uint32(uint32_t v, Arg_callback *c, char *name);   // add unsigned 32 bit integer argument
int Arg_int32(int32_t v, Arg_callback *c, char *name);     // add signed 32 bit integer argument
int Arg_uint64(uint64_t v, Arg_callback *c, char *name);   // add unsigned 64 bit integer argument
int Arg_int64(int64_t v, Arg_callback *c, char *name);     // add signed 64 bit integer argument
int Arg_float(float v, Arg_callback *c, char *name);       // add 32 bit float argument
int Arg_double(double v, Arg_callback *c, char *name);     // add 64 bit double argument
int Arg_ptr(void *v, Arg_callback *c, char *name);         // add pointer argument

#endif
