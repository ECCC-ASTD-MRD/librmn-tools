#
# help macro (macro rather than function because of return() statement)
macro(display_help)
  if(HELP OR DEFINED ENV{HELP})
    unset(HELP CACHE)
    execute_process( 
      COMMAND bash -c "sed -n '/^#[[]*StartHelp/,/^EndHelp]]/{//!p;}' CMakeLists.txt"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    return()  # irrelevant error messages will appear about Ninja version when using Ninja
    # this is inelegant, but suppresses irrelevant error messages about Ninja version
#     message(FATAL_ERROR "HELP mode fake error to stop processing")
  endif()
endmacro()
#
# evaluate a logical expression and set variable named ${result} in caller scope accordingly
# condition : string, result : variable name
macro(evaluate_condition condition result)
  cmake_language(EVAL CODE "
    if(${condition})
      set(${result} TRUE)
    else()
      set(${result} FALSE)
    endif()" 
  )
endmacro()
#
# assert function
function(assert condition type text)
  evaluate_condition("${condition}" local_condition)
  if(NOT local_condition )
    unset(local_condition)
    set(HELP BOOL ON PARENT_SCOPE)  # set HELP variable if assert fails
    message(STATUS "assert failed : '${condition}'")
    message( ${type} "(EC) ${text}" )
  endif()
endfunction()
#
# process environment variable and set variable with same name in caller scope if not already set
function(set_from_environment name)
  if(${name})
    message(STATUS "(EC) set_from_environment : variable ${name} already set to '${${name}}'")
    return()
  endif()
  if(DEFINED ENV{${name}})
    set(${name} $ENV{${name}} PARENT_SCOPE)
    message(STATUS "(EC) setting variable ${name} to '$ENV{${name}}' from environment variable ${name}")
  endif()
endfunction()
#
# set COMPILER_SUITE if appropriate
function(set_compiler_suite)
  if(DEFINED COMPILER_SUITE)
    string(TOLOWER "${COMPILER_SUITE}" COMPILER_SUITE)
    message(STATUS "(EC) COMPILER_SUITE = '${COMPILER_SUITE}'")
  else()
    if(NOT DEFINED ENV{BASE_ARCH} AND NOT DEFINED ENV{EC_ARCH} AND NOT DEFINED ENV{COMP_ARCH})   # NOT EC environment
      set_from_environment(COMPILER_SUITE)        # get environment variable if present
      if(NOT DEFINED COMPILER_SUITE)              # nothing found
        set(COMPILER_SUITE "gnu" CACHE STRING "compiler suite to use")
        message(STATUS "(rmntools) EC environment not found, COMPILER_SUITE not defined, set to 'gnu'" )
      else()
        set(COMPILER_SUITE "${COMPILER_SUITE}" CACHE STRING "compiler suite to use")
        message(STATUS "(rmntools) EC environment not found, COMPILER_SUITE set to '${COMPILER_SUITE}'" )
      endif()
    endif()
  endif()
endfunction()
#
# test log mode
function(set_log_mode)
  if(APPEND_LOG)
    set(WITH_TEST_LOG BOOL ON PARENT_SCOPE)   # override NO_TEST_LOG=ON
    set(TEE_LOG_MODE "-a" PARENT_SCOPE)
    message(STATUS "(EC) test log mode : append to log files")
  else()
    if(NOT WITH_TEST_LOG)
      message(STATUS "(EC) test log mode : NO LOG")
    else()
      message(STATUS "(EC) test log mode : overwrite log files")
    endif()
  endif()
endfunction()
#
# get extra defines for this configuration run
# these extra definitions should not be cached
function(set_extra_defines)
  if(EXTRA_DEFINES)
    string(REGEX REPLACE "[, ]" ";" LOCAL ${EXTRA_DEFINES})
    foreach(ITEM IN ITEMS ${LOCAL})
      add_compile_options(-D${ITEM})
    endforeach()
    message(STATUS "(EC) extra defines : ${EXTRA_DEFINES}")
  endif()
endfunction()
#
# get extra compilation flags for this configuration run
# these extra definitions should not be cached
function(set_extra_flags)
  if(EXTRA_FLAGS)
    string(REGEX REPLACE "[, ]" ";" LOCAL ${EXTRA_FLAGS})
    foreach(ITEM IN ITEMS ${LOCAL})
      add_compile_options(${ITEM})
    endforeach()
    message(STATUS "(EC) extra compilation flags : ${EXTRA_FLAGS}")
  endif()
endfunction()
 
