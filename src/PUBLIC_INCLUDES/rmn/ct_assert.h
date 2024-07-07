
#if ! defined(CT_ASSERT)

#if (__STDC_VERSION__ >= 201112L)  // C11+

#define CT_ASSERT(e, message) _Static_assert(e, message) ;
// #define CT_ASSERT_(e) _Static_assert(e, "") ;

#else

// inspired by https://www.pixelbeat.org/programming/gcc/static_assert.html
#define CT_ASSERT(e, message) extern char (*DuMmY_NaMe(void)) [sizeof(char[1 - 2*!(e)])] ;

#endif

#define CT_ASSERT_(e) CT_ASSERT(e, "")

#endif   // ! defined(CT_ASSERT)
