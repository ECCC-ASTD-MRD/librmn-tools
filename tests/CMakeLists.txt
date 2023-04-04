# target check to run all tests
add_custom_target(check COMMAND CTEST_OUTPUT_ON_FAILURE=true ${CMAKE_CTEST_COMMAND})
add_custom_target(mpicheck COMMAND CTEST_OUTPUT_ON_FAILURE=true ${CMAKE_CTEST_COMMAND})
message(STATUS "(EC) Generating librmntools tests")

if(BUILD_STATIC_LIBS)
  set(PROJECT_TEST_LIBRARIES rmntoolsf rmntools)
endif()
if(BUILD_SHARED_LIBS)
  set(PROJECT_TEST_LIBRARIES rmntoolsf-shared rmntools-shared)
endif()

function(add_project_test name source link_libs)
    # Use same name for target and executable file
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${PROJECT_TEST_LIBRARIES} ${link_libs})
    extra_target_link_options(${name} PRIVATE)      # compiler specific flags
    extra_target_compile_options(${name} PRIVATE)   # compiler specific flags
    set(extra_args ${ARGN})
    list(LENGTH extra_args extra_count)
    if(${extra_count} GREATER 0)      # extra arguments ?
      list(GET ARGN 0 RUN_WITH_MPI)   # first optional argument
    endif()
    if(NOT WITH_TEST_LOG)
      add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> ${name})
      message(STATUS "(EC) no log for test ${name}")
    else()
      message(STATUS "(EC) log for test ${name} : tee ${TEE_LOG_MODE}  ${name}.log")
      add_test(NAME ${name}
#           COMMAND bash -o pipefail -c "${RUN_WITH_MPI}$<TARGET_FILE:${name}> 2>&1 | tee ${name}_$(date +%Y-%m-%d_%H.%M.%S).log"
          COMMAND bash -o pipefail -c "${RUN_WITH_MPI}$<TARGET_FILE:${name}> 2>&1 | tee ${TEE_LOG_MODE} ${name}.log"
#           COMMAND "${RUN_WITH_MPI}$<TARGET_FILE:${name}>"
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    set_property(TEST ${name} PROPERTY FAIL_REGULAR_EXPRESSION "Test STATUS : ERROR[(]S[)]")
    add_dependencies(check ${name})                         # target check depends on this test
    add_dependencies(${name} ${PROJECT_TEST_DEPENDENCIES})  # some Fortran modules are needed for the tests
    if(INSTALL_TESTS)
        install(TARGETS ${name} RUNTIME DESTINATION bin)
    else()
        set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endfunction()

add_library(shared0 SHARED plugin_0.c plugin_0.F90)
add_project_test(test_shared_0 test_shared.c shared0)
add_library(shared1 SHARED plugin_1.c)
add_project_test(test_shared_1 test_shared.c shared1)
add_library(shared2 SHARED plugin_2.c)
add_project_test(test_shared_2 test_shared.c shared2)
add_project_test(test_shared_012 test_shared_012.c "shared2;shared1;shared0")

add_library(sharedf1 SHARED sharedf1.F90 ../src/plugins/plugin_constructor.c)

SET_SOURCE_FILES_PROPERTIES( sharedf1.F90 PROPERTIES COMPILE_FLAGS -I${CMAKE_SOURCE_DIR}/src/PUBLIC_INCLUDES )

add_library(sharedf2 SHARED sharedf2.F90 ../src/plugins/plugin_constructor.c)
SET_SOURCE_FILES_PROPERTIES( sharedf2.F90 PROPERTIES COMPILE_FLAGS -I${CMAKE_SOURCE_DIR}/src/PUBLIC_INCLUDES )

add_library(plugin0 MODULE plugin_0.c plugin_0.F90)
add_library(plugin1 MODULE plugin_1.c)
add_library(plugin2 MODULE plugin_2.c)

add_project_test(test_plugins_c test_plugins.c "rmn::rmn;dl")
# SET_SOURCE_FILES_PROPERTIES( test_plugins.c PROPERTIES COMPILE_FLAGS -I${CMAKE_SOURCE_DIR}/include )

add_project_test(test_plugins_f test_plugins.F90 "rmn::rmn;dl")
# SET_SOURCE_FILES_PROPERTIES( test_plugins.F90 PROPERTIES COMPILE_FLAGS -I${CMAKE_CURRENT_BINARY_DIR}/../modules )

# set compile/link special options for targets not created with add_project_test
set(XTRA_SHARED shared0 shared1 shared2 sharedf1 sharedf2 plugin0 plugin1 plugin2)
foreach(ITEM IN ITEMS ${XTRA_SHARED})
  extra_target_link_options(${ITEM} PRIVATE)      # compiler specific flags
  extra_target_compile_options(${ITEM} PRIVATE)   # compiler specific flags
endforeach()

# target_link_libraries(test_shared_012 PRIVATE dl)
# 

# add_project_test(test_c test_c.c rmntoolsshared)   # link with shared libraries
# target_compile_definitions(test_c PRIVATE TESTC)
# add_project_test(test_f test_f.F90 RMNTOOLSF)   # link with static libraries
# target_compile_definitions(test_f PRIVATE TESTF)

add_project_test(test_identify_mpi_child test_identify_mpi_child.c "dl" "mpirun -n 2 ")
add_dependencies(mpicheck test_identify_mpi_child)

add_project_test(test_identify_compiler_c test_identify_compiler_c.c "dl")
add_project_test(test_identify_compiler_f test_identify_compiler_f.F90 "dl")
add_project_test(test_timers test_timers.c "dl")

add_project_test(test_tee_file test_tee_file.c "dl")
add_project_test(test_tee_file_local test_tee_file.c "dl")
TARGET_COMPILE_DEFINITIONS(test_tee_file_local PRIVATE -DOVERRIDE_WITH_LOCAL)

add_project_test(test_bi_endian_pack test_bi_endian_pack.c "dl")
SET_SOURCE_FILES_PROPERTIES( test_bi_endian_pack PROPERTIES COMPILE_FLAGS -O3 )

add_project_test(test_pack_macros test_pack_macros.c "dl")
SET_SOURCE_FILES_PROPERTIES( test_pack_macros PROPERTIES COMPILE_FLAGS -O3 )

add_project_test(test_lorenzo_c test_lorenzo_c.c "dl")
add_project_test(test_lorenzo_f test_lorenzo.F90 "rmn::rmn")
SET_SOURCE_FILES_PROPERTIES( test_lorenzo.F90 PROPERTIES COMPILE_FLAGS -DNO_FORTRAN_CODE )

