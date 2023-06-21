
// inspired by https://www.pixelbeat.org/programming/gcc/static_assert.html
#if ! defined(CT_ASSERT)
#define CT_ASSERT(e) extern char (*DuMmY_NaMe(void)) [sizeof(char[1 - 2*!(e)])] ;
#endif
