add_custom_target(check COMMAND CTEST_OUTPUT_ON_FAILURE=true ${CMAKE_CTEST_COMMAND})
message(STATUS "(EC) Generating librmn tests")

option(INSTALL_TESTS OFF)
function(add_rmntools_test name source link_libs)
    # Use same name for target as for file
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${link_libs})
    #     add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> ${name})
    list(APPEND ARGN " ")           # make sure that list is not empty
    list(GET ARGN 0 RUN_WITH_MPI)   # first optional argument " " if none
    add_test(NAME ${name}
        COMMAND bash -c "${RUN_WITH_MPI} $<TARGET_FILE:${name}> 2>&1 | tee ${name}_$(date +%Y-%m-%d_%H.%M.%S).log"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(check ${name})
    if(INSTALL_TESTS)
        install(TARGETS ${name} RUNTIME DESTINATION bin)
    else()
        set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endfunction()

# add_rmntools_test(test_c test_c.c rmntoolsshared)   # link with shared libraries
# target_compile_definitions(test_c PRIVATE TESTC)
# add_rmntools_test(test_f test_f.F90 RMNTOOLSF)   # link with static libraries
# target_compile_definitions(test_f PRIVATE TESTF)
if(BUILD_SHARED_LIBS)
  set(RMNTOOLSF rmntoolsf-shared)
  set(RMNTOOLS rmntools-shared)
endif()
if(BUILD_STATIC_LIBS)
  set(RMNTOOLSF rmntoolsf)
  set(RMNTOOLS rmntools)
endif()

add_rmntools_test(test_identify_mpi_child test_identify_mpi_child.c ${RMNTOOLS} "mpirun -n 2")

add_rmntools_test(test_identify_compiler_c test_identify_compiler_c.c ${RMNTOOLS})
add_rmntools_test(test_identify_compiler_f test_identify_compiler_f.F90 ${RMNTOOLSF})
add_rmntools_test(test_timers test_timers.c ${RMNTOOLS})

add_rmntools_test(test_tee_file test_tee_file.c ${RMNTOOLS})
add_rmntools_test(test_tee_file_local test_tee_file.c ${RMNTOOLS})
TARGET_COMPILE_DEFINITIONS(test_tee_file_local PRIVATE -DOVERRIDE_WITH_LOCAL)

add_rmntools_test(test_bi_endian_pack test_bi_endian_pack.c ${RMNTOOLS})
SET_SOURCE_FILES_PROPERTIES( test_bi_endian_pack PROPERTIES COMPILE_FLAGS -O3 )

add_rmntools_test(test_pack_macros test_pack_macros.c ${RMNTOOLS})
SET_SOURCE_FILES_PROPERTIES( test_pack_macros PROPERTIES COMPILE_FLAGS -O3 )

add_rmntools_test(test_lorenzo_c test_lorenzo_c.c ${RMNTOOLS})
add_rmntools_test(test_lorenzo_f test_lorenzo.F90 "rmn::rmn;${RMNTOOLSF}")
SET_SOURCE_FILES_PROPERTIES( test_lorenzo.F90 PROPERTIES COMPILE_FLAGS -DNO_FORTRAN_CODE )

