program test_f
#include <internal.hf>
#include <rmntools.hf>
#include <rmn/rmn_tools.hf>
#if defined(TESTF)
  print *,"testf O.K."
#else
  print *,"ERROR: test_f, TESTF not defined"
#endif
call dummy_f
call pred_dummy_f
end
