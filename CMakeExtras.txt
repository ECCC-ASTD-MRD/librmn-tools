# test log mode
if(APPEND_LOG)
  set(WITH_TEST_LOG BOOL ON)   # override NO_TEST_LOG=ON
  set(TEE_LOG_MODE "-a")
  message(STATUS "(EC) test log mode : append to log files")
else()
  if(NOT WITH_TEST_LOG)
    message(STATUS "(EC) test log mode : NO LOG")
  else()
    message(STATUS "(EC) test log mode : overwrite log files")
  endif()
endif()
#
# get extra defines for this configuration run
# these extra definitions should not be cached
if(EXTRA_DEFINES)
  string(REGEX REPLACE "[, ]" ";" LOCAL ${EXTRA_DEFINES})
  unset(EXTRA_DEFINES CACHE)
  foreach(ITEM IN ITEMS ${LOCAL})
    set(EXTRA_DEFINES "${EXTRA_DEFINES} -D${ITEM}")
    add_compile_options(-D${ITEM})
  endforeach()
  message(STATUS "(EC) extra defines : ${EXTRA_DEFINES}")
endif()
#
# get extra compilation options for this configuration run
# these extra definitions should not be cached
if(EXTRA_OPTIONS)
  string(REGEX REPLACE "[, ]" ";" LOCAL ${EXTRA_OPTIONS})
  unset(EXTRA_OPTIONS CACHE)
  foreach(ITEM IN ITEMS ${LOCAL})
    set(EXTRA_OPTIONS "${EXTRA_OPTIONS} ${ITEM}")
    add_compile_options(${ITEM})
  endforeach()
  message(STATUS "(EC) extra compilation options : ${EXTRA_OPTIONS}")
endif()
