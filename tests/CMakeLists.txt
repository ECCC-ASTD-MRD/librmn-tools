# target check to run all tests
add_custom_target(check COMMAND CTEST_OUTPUT_ON_FAILURE=true ${CMAKE_CTEST_COMMAND})
message(STATUS "(EC) Generating librmntools tests")

set(PROJECT_STATIC_TEST_LIBRARIES rmntoolsf rmntools)
set(PROJECT_SHARED_TEST_LIBRARIES rmntoolsf-shared rmntools-shared)

function(add_project_test name source)
  cmake_parse_arguments(PARSE_ARGV 0 "TN" "NOCTEST;NODIAG;STATIC;NO_TOOLS;DL" "" "MPI;LINK_LIBS")
  # Use same name for target and executable file
  add_executable(${name} ${source})
  if( TN_NODIAG AND ${COMPILER_SUITE} MATCHES  "nvhpc" AND ${source} MATCHES ".c")
    SET_SOURCE_FILES_PROPERTIES( ${source} PROPERTIES 
      COMPILE_OPTIONS "--diag_suppress;declared_but_not_referenced;--diag_suppress;set_but_not_used;--diag_suppress;code_is_unreachable" )
  endif()
  if(TN_DL)
    set(libdl "-ldl")
    if(NOT TN_STATIC)
      set(TN_LINK_LIBS "${TN_LINK_LIBS};-ldl")
    endif()
  endif()
  if( (TN_STATIC AND BUILD_STATIC_LIBS) OR (NOT BUILD_SHARED_LIBS) )
    if(TN_NO_TOOLS)
      target_link_libraries(${name} PRIVATE ${TN_LINK_LIBS})
#       message(STATUS "target_link_libraries(${name} PRIVATE ${TN_LINK_LIBS})")
    else()
      target_link_libraries(${name} PRIVATE -Bstatic ${PROJECT_STATIC_TEST_LIBRARIES} ${TN_LINK_LIBS} -Bdynamic ${libdl})
#       message(STATUS "target_link_libraries(${name} PRIVATE -Bstatic ${PROJECT_STATIC_TEST_LIBRARIES} ${TN_LINK_LIBS} -Bdynamic)")
    endif()
  else()
    if(TN_NO_TOOLS)
      target_link_libraries(${name} PRIVATE ${TN_LINK_LIBS})
#       message(STATUS "target_link_libraries(${name} PRIVATE ${TN_LINK_LIBS})")
    else()
      target_link_libraries(${name} PRIVATE ${PROJECT_SHARED_TEST_LIBRARIES} ${TN_LINK_LIBS})
#       message(STATUS "target_link_libraries(${name} PRIVATE ${PROJECT_SHARED_TEST_LIBRARIES} ${TN_LINK_LIBS})")
    endif()
  endif()
  extra_target_link_options(${name} PRIVATE)      # compiler specific flags
  extra_target_compile_options(${name} PRIVATE)   # compiler specific flags
  if(TN_NOCTEST)
    message(STATUS "(EC) test ${name} not included in target check(ctest)")
  else()
    if(NOT WITH_TEST_LOG)
      if(TN_MPI)
        add_test(NAME ${name} COMMAND bash -c "${TN_MPI}$<TARGET_FILE:${name}>")
      else()
        add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> ${name})
      endif()
      message(STATUS "(EC) no log for test ${TN_MPI}${name}")
    else()
      message(STATUS "(EC) log for test ${TN_MPI}${name} : tee ${TEE_LOG_MODE}  ${name}.log")
      add_test(NAME ${name}
#           COMMAND bash -o pipefail -c "${TN_MPI}$<TARGET_FILE:${name}> 2>&1 | tee ${name}_$(date +%Y-%m-%d_%H.%M.%S).log"
          COMMAND bash -o pipefail -c "${TN_MPI}$<TARGET_FILE:${name}> 2>&1 | tee ${TEE_LOG_MODE} ${name}.log"
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    set_property(TEST ${name} PROPERTY FAIL_REGULAR_EXPRESSION "Test STATUS : ERROR[(]S[)]")
  endif()
  add_dependencies(check ${name})                         # target check depends on this test
  add_dependencies(${name} ${PROJECT_TEST_DEPENDENCIES})  # some Fortran modules are needed for the tests
  if(INSTALL_TESTS)
      install(TARGETS ${name} RUNTIME DESTINATION bin)
  else()
      set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endfunction()

set(TEST_INCLUDES -I${CMAKE_SOURCE_DIR}/src/PUBLIC_INCLUDES)

SET_SOURCE_FILES_PROPERTIES( test_shared.c PROPERTIES COMPILE_OPTIONS ${TEST_INCLUDES} )
SET_SOURCE_FILES_PROPERTIES( ../src/diag/test_helpers.c PROPERTIES COMPILE_OPTIONS ${TEST_INCLUDES} )
add_library(shared0 SHARED plugin_0.c ../src/diag/test_helpers.c plugin_0.F90)
add_project_test(test_shared_0 test_shared.c LINK_LIBS shared0 NO_TOOLS)
add_library(shared1 SHARED plugin_1.c ../src/diag/test_helpers.c)
add_project_test(test_shared_1 test_shared.c LINK_LIBS shared1 NO_TOOLS)
add_library(shared2 SHARED plugin_2.c ../src/diag/test_helpers.c)
add_project_test(test_shared_2 test_shared.c LINK_LIBS shared2 NO_TOOLS)

SET_SOURCE_FILES_PROPERTIES( test_shared_012.c PROPERTIES COMPILE_OPTIONS ${TEST_INCLUDES} )
add_project_test(test_shared_012 test_shared_012.c LINK_LIBS "shared2;shared1;shared0" NO_TOOLS)

SET_SOURCE_FILES_PROPERTIES( sharedf1.F90 PROPERTIES COMPILE_OPTIONS ${TEST_INCLUDES} )
add_library(sharedf1 SHARED sharedf1.F90 ../src/plugins/plugin_constructor.c)

SET_SOURCE_FILES_PROPERTIES( sharedf2.F90 PROPERTIES COMPILE_OPTIONS ${TEST_INCLUDES} )
add_library(sharedf2 SHARED sharedf2.F90 ../src/plugins/plugin_constructor.c)

add_library(plugin0 MODULE plugin_0.c plugin_0.F90)
add_library(plugin1 MODULE plugin_1.c)
add_library(plugin2 MODULE plugin_2.c)

set(test_plugins plugin0 plugin1 plugin2 shared0 shared1 shared2 sharedf1 sharedf2)
foreach(ITEM IN ITEMS ${test_plugins})
  install(TARGETS ${ITEM} DESTINATION bin)
endforeach()

add_project_test(test_plugins_c test_plugins.c LINK_LIBS "rmn::rmn;libaec::aec" DL)
# SET_SOURCE_FILES_PROPERTIES( test_plugins.c PROPERTIES COMPILE_OPTIONS -I${CMAKE_SOURCE_DIR}/include )

add_project_test(test_plugins_f test_plugins.F90 LINK_LIBS "rmn::rmn;libaec::aec" DL)
# SET_SOURCE_FILES_PROPERTIES( test_plugins.F90 PROPERTIES COMPILE_OPTIONS -I${CMAKE_CURRENT_BINARY_DIR}/../modules )

# set compile/link special options for targets not created with add_project_test
set(XTRA_SHARED shared0 shared1 shared2 sharedf1 sharedf2 plugin0 plugin1 plugin2)
foreach(ITEM IN ITEMS ${XTRA_SHARED})
  extra_target_link_options(${ITEM} PRIVATE)      # compiler specific flags
  extra_target_compile_options(${ITEM} PRIVATE)   # compiler specific flags
endforeach()

# target_link_libraries(test_shared_012 PRIVATE dl)
# 

add_project_test(test_identify_mpi_child test_identify_mpi_child.c LINK_LIBS "rmn::rmn-shared" MPI "mpirun -n 2 ")

add_project_test(test_identify_compiler_c test_identify_compiler_c.c LINK_LIBS "rmn::rmn-shared")
add_project_test(test_identify_compiler_f test_identify_compiler_f.F90 DL LINK_LIBS "rmn::rmn-shared;libaec::aec")
add_project_test(test_timers test_timers.c LINK_LIBS "rmn::rmn-shared")

add_project_test(test_tee_file test_tee_file.c LINK_LIBS "rmn::rmn-shared")
add_project_test(test_tee_file_local test_tee_file.c LINK_LIBS "rmn::rmn-shared")
TARGET_COMPILE_DEFINITIONS(test_tee_file_local PRIVATE -DOVERRIDE_WITH_LOCAL)

add_project_test(test_bi_endian_pack test_bi_endian_pack.c LINK_LIBS "rmn::rmn-shared")
SET_SOURCE_FILES_PROPERTIES( test_bi_endian_pack PROPERTIES COMPILE_OPTIONS -O3 )

add_project_test(test_pack_macros test_pack_macros.c LINK_LIBS "rmn::rmn-shared")
SET_SOURCE_FILES_PROPERTIES( test_pack_macros PROPERTIES COMPILE_OPTIONS -O3 )

add_project_test(test_bi_endian_pack_macros test_bi_endian_pack_macros.c LINK_LIBS "rmn::rmn-shared")
SET_SOURCE_FILES_PROPERTIES( test_bi_endian_pack_macros PROPERTIES COMPILE_OPTIONS -O3 )

add_project_test(test_lorenzo_c test_lorenzo_c.c LINK_LIBS "rmn::rmn-shared")
add_project_test(test_lorenzo_f_shared test_lorenzo.F90 LINK_LIBS "rmn::rmn-shared;libaec::aec" DL)                        # build with shared libraries
if(NOT ( ${COMPILER_SUITE} MATCHES  "nvhpc") )
# problem building test_lorenzo_f_static with Nvidia compiler (dl library symbol missing)
add_project_test(test_lorenzo_f_static test_lorenzo.F90 STATIC LINK_LIBS "rmn::rmn-static" DL)        # build with static libraries
else()
# add_project_test(test_lorenzo_f_static test_lorenzo.F90 STATIC LINK_LIBS "rmn::rmn-static" DL)        # build with static libraries
endif()
# add_project_test(test_lorenzo_f_static test_lorenzo.F90 STATIC LINK_LIBS rmn::rmn-static)                          # build with static libraries
# SET_SOURCE_FILES_PROPERTIES( test_lorenzo.F90 PROPERTIES COMPILE_OPTIONS -DNO_FORTRAN_CODE )

add_project_test(test_fstd_compress  test_fstd_compress.F90 LINK_LIBS "rmn::rmn;libaec::aec" DL NOCTEST)   # not included in ctest for now
# add_project_test(fstd_to_raw  fstd_to_raw.F90 LINK_LIBS "rmn::rmn" DL NOCTEST)   # not included in ctest for now
add_project_test(fstd_to_raw  fstd_to_raw.F90 LINK_LIBS "rmn::rmn-shared;libaec::aec" DL NOCTEST)   # not included in ctest for now
if(NOT ( ${COMPILER_SUITE} MATCHES  "nvhpc") )
# problem building fstd_to_raw_s with Nvidia compiler (dl library symbol missing)
add_project_test(fstd_to_raw_s  fstd_to_raw.F90 STATIC LINK_LIBS "rmn::rmn-static" DL NOCTEST)   # not included in ctest for now
endif()

add_project_test(test_c_record_io test_c_record_io.c LINK_LIBS "rmn::rmn-shared" NOCTEST)    # not included in ctest for now
add_project_test(test_c_record_io_f test_c_record_io.F90 NOCTEST DL LINK_LIBS "rmn::rmn-shared;libaec::aec")    # not included in ctest for now

add_project_test(test_quantizer_errors test_quantizer_errors.c LINK_LIBS "m;rmn::rmn-shared;libaec::aec" DL NOCTEST NODIAG)    # not included in ctest for now

add_project_test(test_serialized_functions test_serialized_functions.c LINK_LIBS "rmn::rmn-shared" NODIAG)
# maybe needed depending upon print statement distribution in test code
# if(COMPILER_SUITE STREQUAL aocc)
# message(STATUS "(TESTS) using -O0 for test_serialized_functions with ${COMPILER_SUITE}")
# SET_SOURCE_FILES_PROPERTIES( test_serialized_functions PROPERTIES COMPILE_OPTIONS -O0 )
# endif()

add_project_test(test_ieeequantize test_ieeequantize.c LINK_LIBS "rmn::rmn-shared" NODIAG)

add_project_test(test_logquantize test_logquantize.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_encoders test_encoders.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_bitstream test_bitstream.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_entropy test_entropy.c LINK_LIBS "m;rmn::rmn;libaec::aec" DL NODIAG)
add_project_test(test_data_info_c test_data_info.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_data_info_f test_data_info.F90 LINK_LIBS "rmn::rmn;libaec::aec" DL)

add_project_test(test_bits test_bits.F90 DL LINK_LIBS "rmn::rmn;libaec::aec")
add_project_test(test_pipe_filters test_pipe_filters.c LINK_LIBS "rmn::rmn-shared")
add_project_test(test_atomic_functions test_atomic_functions.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_pixmaps test_pixmaps.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_wordstream test_wordstream.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_compress_expand test_compress_expand.c LINK_LIBS "rmn::rmn;libaec::aec" NODIAG)
add_project_test(test_copy_swap test_copy_swap.c  LINK_LIBS "rmn::rmn" NODIAG)

add_project_test(test_data_map test_data_map.c LINK_LIBS "rmn::rmn-shared" NODIAG)
add_project_test(test_readlx test_readlx.F90 LINK_LIBS "rmn::rmn-shared" NODIAG)
