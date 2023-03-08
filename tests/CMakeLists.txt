# The minimum version can not be 3.16 because of the LINK_LANGUAGE generator expression in Config.cmake.in
cmake_minimum_required(VERSION 3.20)

add_executable(test_c test_c.c)
target_compile_definitions(test_c PRIVATE TESTC)

add_executable(test_f test_f.F90)
target_compile_definitions(test_f PRIVATE TESTF)

add_executable(test_identify_compiler_c test_identify_compiler_c.c)
add_executable(test_identify_compiler_f test_identify_compiler_f.F90)
add_executable(test_timers test_timers.c)
add_executable(test_lorenzo_f test_lorenzo.F90)
add_executable(test_lorenzo_c test_lorenzo_c.c)
add_executable(test_lorenzo test_lorenzo.c)

set(TARGETS
    test_identify_compiler_c
    test_identify_compiler_f
    test_timers
    test_lorenzo
    test_lorenzo_c
    test_lorenzo_f
    test_f
    test_c)

# currently set to build with shared libraries, build with .a commented for now
# libraries expected to be found in ../lib relative to executable directory
# (an extra $ORIGIN/.. item will be found in the rpath)
foreach(TARGET IN ITEMS ${TARGETS})
    target_include_directories(${TARGET}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../modules/rmntoolssharedf
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src/PUBLIC_INCLUDES 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src
      )
    set_target_properties(${TARGET} PROPERTIES BUILD_RPATH "$ORIGIN/../lib" BUILD_RPATH_USE_ORIGIN true)
    target_link_options(${TARGET}
        PUBLIC $<$<COMPILE_LANG_AND_ID:Fortran,GNU>:${GNU_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,GNU>:${GNU_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,Intel>:${Intel_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,Intel>:${Intel_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,Flang>:${Flang_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,Clang>:${Clang_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,PGI>:${PGI_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,PGI>:${PGI_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,LLVMFlang>:${LLVM_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,LLVMClang>:${LLVM_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,NVHPC>:${PGI_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,NVHPC>:${PGI_CFLAGS}>
               )
    target_link_libraries(${TARGET} PRIVATE ${PROJECT_NAME}sharedf ${PROJECT_NAME}shared)
#     target_link_libraries(${TARGET} PRIVATE ${PROJECT_NAME}f ${PROJECT_NAME})
endforeach()

foreach(TARGET IN ITEMS ${TARGETS})
    install(
        PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}"
        DESTINATION "${BIN_INSTALL_DIR}"
    )
endforeach()
