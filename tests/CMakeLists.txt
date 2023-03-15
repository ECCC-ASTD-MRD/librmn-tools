add_custom_target(check COMMAND CTEST_OUTPUT_ON_FAILURE=true ${CMAKE_CTEST_COMMAND})
message(STATUS "(EC) Generating librmn tests")

option(INSTALL_TESTS OFF)
function(add_rmntools_test name source link_libs)
    # Use same name for target as for file
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${link_libs})
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> ${name})
    add_dependencies(check ${name})
    if(INSTALL_TESTS)
        install(TARGETS ${name} RUNTIME DESTINATION bin)
    else()
        set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endfunction()

add_rmntools_test(test_c test_c.c rmntools)
target_compile_definitions(test_c PRIVATE TESTC)

add_rmntools_test(test_f test_f.F90 rmntoolsf)
target_compile_definitions(test_f PRIVATE TESTF)

add_rmntools_test(test_identify_compiler_c test_identify_compiler_c.c rmntools)
add_rmntools_test(test_identify_compiler_f test_identify_compiler_f.F90 rmntoolsf)
add_rmntools_test(test_timers test_timers.c rmntools)
add_rmntools_test(test_lorenzo_f test_lorenzo.F90 rmntoolsf)
add_rmntools_test(test_lorenzo_c test_lorenzo_c.c rmntools)
