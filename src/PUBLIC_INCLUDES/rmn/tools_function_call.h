
#if ! defined(TOOLS_FUNCTION_TYPES)
#define TOOLS_FUNCTION_TYPES

#include <rmn/tools_types.h>

Arg_callback *Arg_init(Arg_fn fn, int maxargs);
int Arg_uint8(uint8_t v, Arg_callback *c, char *name);
int Arg_int8(int8_t v, Arg_callback *c, char *name);
int Arg_uint16(uint16_t v, Arg_callback *c, char *name);
int Arg_int16(int16_t v, Arg_callback *c, char *name);
int Arg_uint32(uint32_t v, Arg_callback *c, char *name);
int Arg_int32(int32_t v, Arg_callback *c, char *name);
int Arg_uint64(uint64_t v, Arg_callback *c, char *name);
int Arg_int64(int64_t v, Arg_callback *c, char *name);
int Arg_float(float v, Arg_callback *c, char *name);
int Arg_double(double v, Arg_callback *c, char *name);
int Arg_ptr(void *v, Arg_callback *c, char *name);

#endif
