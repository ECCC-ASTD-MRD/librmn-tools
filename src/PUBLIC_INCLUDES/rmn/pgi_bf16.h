// temporarily compensate for lack of __bf16 definition in some compilers
#if defined(__PGI) && ! defined(__PGI__BF16__)
#define __PGI__BF16__
  typedef struct{
    uint16_t v ;
  } __bf16 ;
#endif
