# The minimum version can not be 3.16 because of the LINK_LANGUAGE generator expression in Config.cmake.in
cmake_minimum_required(VERSION 3.20)
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are disabled for this project")
else()
  # Git auto-ignore out-of-source build directory
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()
# what follows gets written in terminal window if 
# - the HELP nevironment variable is defined
# - or -DHELP=1 is used in cmake call
#[[StartHelp
optional cmake recognized variables :
  -DHELP=ON
  -DCMAKE_INSTALL_PREFIX=/some/path
  -DWITH_OPENMP=ON|OFF
  -DCOMPILER_SUITE=gnu|intel|aocc|llvm|nvhpc
  -DUSE_ICX
  -DINSTALL_TESTS=ON|OFF
  -DEXTRA_DEFINES=def1[=v1][ ,def2[=v2] ] ... [ ,defn[=vn] ]
     QUIET_SIMD 
     EXTRA_FORTRAN
EndHelp]]
if(HELP OR DEFINED ENV{HELP})
  unset(HELP CACHE)
  execute_process( 
    COMMAND bash -c "sed -n '/^#[[]*StartHelp/,/^EndHelp]]/{//!p;}' CMakeLists.txt"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  return()  # irrelevant error messages will appear about Ninja version when using Ninja
  # this is inelegant, but suppresses irrelevant error messages about Ninja version
#   message(FATAL_ERROR "HELP mode fake error to stop processing")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtras.txt)
  include(CMakeExtras.txt)
endif()
# conditional_return()    # return if help requested

if(DEFINED COMPILER_SUITE)
  string(TOLOWER "${COMPILER_SUITE}" COMPILER_SUITE)
else()
  if(NOT DEFINED ENV{BASE_ARCH} AND NOT DEFINED ENV{EC_ARCH} AND NOT DEFINED ENV{COMP_ARCH})
    set(COMPILER_SUITE "gnu" CACHE STRING "compiler to use")
    message(STATUS "(rmntools) EC environment not found, COMPILER_SUITE not defined, set to gnu" )
  endif()
endif()

# if(BUILD_SHARED_LIBS)
#   unset(BUILD_STATIC_LIBS)
# else()
#   set(BUILD_STATIC_LIBS BOOL ON)
# endif()

include(CMakePrintHelpers)
# Append EC specific module path
list(APPEND CMAKE_MODULE_PATH $ENV{EC_CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake_rpn/modules)

include(ec_init)           # Initialise compilers and ec specific functions
ec_git_version()           # Get version from git state
ec_parse_manifest()        # Parse MANIFEST file
project(${NAME} DESCRIPTION "${DESCRIPTION}" LANGUAGES C Fortran)

# enable_language(C Fortran)
if (NOT rmn_FOUND)
message(STATUS "NOT rmn_FOUND, looking for package rmn")
   find_package(rmn ${rmn_REQ_VERSION} COMPONENTS REQUIRED)
endif()

set(PROJECT_VERSION ${VERSION}${STATE})
set(rmntools_VERSION ${PROJECT_VERSION} CACHE INTERNAL "rmntools version" FORCE) # Needed for cascaded version identification
message(STATUS "(EC) ${PROJECT_NAME} version = ${PROJECT_VERSION} GENERATOR = ${CMAKE_GENERATOR}")

ec_build_info()            # Generate build information

include(ec_compiler_presets)

set(WITH_OPENMP TRUE CACHE BOOL "Control whether to use OpenMP")
include(ec_openmp)          # Enable OpenMP (-DWITH_OPENMP=TRUE)
message(STATUS "(EC ${PROJECT_NAME}) WITH_OPENMP=${WITH_OPENMP}" )
#include(ec_doxygen)         # Doxygen target doc

# Fortran compiler options required to build this project (also used in tests)
message(STATUS "CMAKE_C_COMPILER == ${CMAKE_C_COMPILER}")
# if(${CMAKE_C_COMPILER} MATCHES ".*icc\$")
# #   set(CMAKE_C_COMPILER "icx")
#   list(APPEND Intel_CFLAGS -xznver3)
# message(STATUS "CMAKE_C_COMPILER == ${CMAKE_C_COMPILER}")
# endif()
if(WITH_OPENMP)
  set(Intel_FFLAGS -convert big_endian -align array32byte -assume byterecl -qopenmp -march=skylake -diag-disable=5268,10121)
  list(APPEND IntelLLVM_FFLAGS -convert big_endian -align array32byte -assume byterecl -qopenmp -march=skylake)
  list(APPEND IntelLLVM_CFLAGS -qopenmp -Wno-static-in-inline -march=znver3)
  if(${CMAKE_C_COMPILER} MATCHES ".*icx\$")
    message(STATUS "(COMPILER) Intel icx detected")
    add_link_options( -march=native )
    add_compile_options( -march=native )
  else()
    set(Intel_CFLAGS -qopenmp -march=skylake -diag-disable=10441,10121)
    list(APPEND IntelLLVM_CFLAGS -qopenmp -march=skylake)
  endif()

  set(GNU_FFLAGS -fconvert=big-endian -fcray-pointer -frecord-marker=4 -fno-second-underscore -fopenmp)
  set(GNU_CFLAGS -fopenmp -march=native)

  set(Flang_FFLAGS -Mbyteswapio -fopenmp)
  if(${CMAKE_C_COMPILER} MATCHES ".*clang\$")
    set(Clang_CFLAGS -fopenmp -Wno-static-in-inline -march=native)
  else()
    set(Clang_CFLAGS -fopenmp -march=native)
  endif()

  set(PGI_FFLAGS -byteswapio -mp)
  set(PGI_CFLAGS -mp)

  set(LLVM_FFLAGS -fconvert=big-endian -fopenmp)
  set(LLVM_CFLAGS -fopenmp)
else()
  set(Intel_FFLAGS -convert big_endian -align array32byte -assume byterecl -march=skylake -diag-disable=5268,10121)
  list(APPEND IntelLLVM_FFLAGS -convert big_endian -align array32byte -assume byterecl -march=skylake)
  set(Intel_CFLAGS -Wno-static-in-inline -march=skylake -diag-disable=10441,10121)
  list(APPEND IntelLLVM_CFLAGS -Wno-static-in-inline -march=skylake)
  set(GNU_FFLAGS -fconvert=big-endian -fcray-pointer -frecord-marker=4 -fno-second-underscore -ldl -pthread)
  set(GNU_CFLAGS -march=native)
  set(Flang_FFLAGS -Mbyteswapio)
  set(Clang_CFLAGS -Wno-static-in-inline -march=native)
  set(PGI_FFLAGS -byteswapio)
  set(LLVM_FFLAGS -fconvert=big-endian)
endif()

include(GNUInstallDirs)
# Provides CMAKE_INSTALL_<dir> where
# - CMAKE_INSTALL_LIBDIR = lib
# - CMAKE_INSTALL_INCLUDEDIR = include
# - CMAKE_INSTALL_BINDIR = bin
# these are the defaults for X DESTINATION in install command

# ======================== PROJECT FILES ======================
set(PROJECT_C_FILES 
  src/id/identify_compiler.c
  src/id/identify_mpi_child.c
  src/timers/rmn_timers.c
  src/pred/lorenzo_c.c
  src/pack/bi_endian_pack.c
  src/diag/tee_print.c
)
set(PROJECT_PLUGIN_FILES 
  src/pred/lorenzo_c.c
  src/pack/bi_endian_pack.c
)
SET_SOURCE_FILES_PROPERTIES( src/pred/lorenzo_c.c PROPERTIES COMPILE_FLAGS "-O3 -DQUIET_SIMD -march=skylake")
SET_SOURCE_FILES_PROPERTIES( src/pred/bi_endian_pack.c PROPERTIES COMPILE_FLAGS "-O3 -march=skylake")
# file(GLOB PROJECT_C_FILES
#    src/*.c
#    src/*/*.c
# )
set(PROJECT_F_FILES
  src/timers/rmn_timers_mod.F90
  src/pred/lorenzo_mod.F90
  src/plugins/fortran_plugins.F90
)
# file(GLOB PROJECT_F_FILES
#    src/*.F90
#    src/*/*.F90
# )
SET_SOURCE_FILES_PROPERTIES( src/pred/lorenzo_mod.F90 PROPERTIES COMPILE_FLAGS -DNO_EXTRA_FORTRAN )
set(PROJECT_H_FILES
  src/PUBLIC_INCLUDES/rmntools.h
  src/PUBLIC_INCLUDES/rmn/lorenzo.h
  src/PUBLIC_INCLUDES/rmn/rmn_tools.h
  src/PUBLIC_INCLUDES/rmn/identify_compiler_f.hf
  src/PUBLIC_INCLUDES/rmn/timers.h
  src/PUBLIC_INCLUDES/rmn/tools_types.h
  src/PUBLIC_INCLUDES/rmn/pack_macros.h
  src/PUBLIC_INCLUDES/rmn/bi_endian_pack.h
  src/PUBLIC_INCLUDES/rmn/bits.h
  src/PUBLIC_INCLUDES/rmn/tee_print.h
  src/PUBLIC_INCLUDES/rmn/identify_compiler_c.h
  src/PUBLIC_INCLUDES/rmn/identify_mpi_child.h
  src/PUBLIC_INCLUDES/rmn/is_fortran_compiler.h
  src/PUBLIC_INCLUDES/rmn/plugins.h
  src/PUBLIC_INCLUDES/rmn/plugins.hf
)
# file(GLOB PROJECT_H_FILES
#    src/*/*.h
#    src/*/*/*.h
# )
set(PROJECT_INCLUDES
  -I${CMAKE_CURRENT_SOURCE_DIR}/include
  -I${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES
  )
# ================================================================

string(REGEX MATCH "([0-9]+)" SOVERSION ${PROJECT_VERSION})

if(BUILD_SHARED_LIBS)
set(CMAKE_BUILD_RPATH_USE_ORIGIN true)
set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
# set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

if(BUILD_STATIC_LIBS)
# C static library
add_library(rmntools STATIC ${PROJECT_C_FILES})
set_target_properties(rmntools PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME rmntools)
if(WITH_OPENMP)
    target_compile_definitions(rmntools PUBLIC WITH_OPENMP)
    target_link_libraries(rmntools PUBLIC OpenMP::OpenMP_C)
endif()

# Fortran static library (will make calls to C library)
# include C objects in Fortran library
add_library(rmntoolsf STATIC ${PROJECT_F_FILES} $<TARGET_OBJECTS:rmntools>)
set_target_properties(rmntoolsf PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME rmntoolsf)
# (will make calls to C library)
# target_link_libraries(rmntoolsf PUBLIC rmntools)
if(WITH_OPENMP)
    target_compile_definitions(rmntoolsf PUBLIC WITH_OPENMP)
    target_link_libraries(rmntoolsf PUBLIC OpenMP::OpenMP_Fortran)
endif()
endif(BUILD_STATIC_LIBS)

if(BUILD_SHARED_LIBS)
add_library(rmntools-shared SHARED $<TARGET_OBJECTS:rmntools>)
set_target_properties(rmntools-shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntools-shared)
# C shared library
add_library(rmntools-plugins MODULE ${PROJECT_PLUGIN_FILES})
# SET_SOURCE_FILES_PROPERTIES( src/pred/lorenzo_c.c PROPERTIES COMPILE_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(rmntools-plugins PROPERTIES
    VERSION ${PROJECT_VERSION}
#     PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    COMPILE_OPTIONS "${PROJECT_INCLUDES}"
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntools-plugins)
if(WITH_OPENMP)
    target_compile_definitions(rmntools-shared PUBLIC WITH_OPENMP)
    target_link_libraries(rmntools-shared PUBLIC OpenMP::OpenMP_C)
    target_compile_definitions(rmntools-plugins PUBLIC WITH_OPENMP)
    target_link_libraries(rmntools-plugins PUBLIC OpenMP::OpenMP_C)
endif()

# Fortran shared library, needs to link to C shared library
# include C objects in Fortran library
add_library(rmntoolsf-shared SHARED $<TARGET_OBJECTS:rmntoolsf>)
set_target_properties(rmntoolsf-shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntoolsf)
# needs to link to C shared library
# target_link_libraries(rmntoolsf-shared PUBLIC rmntools-shared)
if(WITH_OPENMP)
    target_compile_definitions(rmntoolsf-shared PUBLIC WITH_OPENMP)
    target_link_libraries(rmntoolsf-shared PUBLIC OpenMP::OpenMP_Fortran)
endif()
endif(BUILD_SHARED_LIBS)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
if(BUILD_STATIC_LIBS)
  list(APPEND TARGETS rmntools rmntoolsf)
endif()
if(BUILD_SHARED_LIBS)
  list(APPEND TARGETS rmntools-shared rmntoolsf-shared)
endif()
# set(TARGETS rmntools rmntoolsf rmntools-shared rmntoolsf-shared)

# if(BUILD_SHARED_LIBS)
# message(STATUS "add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}-shared)")
#     add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}-shared)
# set(TARGETS rmntools-shared rmntoolsf-shared)
# endif()
# if(BUILD_STATIC_LIBS)
# message(STATUS "add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})")
#     add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
# set(TARGETS rmntools rmntoolsf)
# endif()

# Required to prevent missing Fortran modules when building in parallel
# (make -j) with GNU compilers
# add_dependencies(rmntoolsf-shared rmntools-shared)

# set(TARGETS rmntools rmntoolsf rmntools-shared rmntoolsf-shared)

foreach(TARGET IN ITEMS ${TARGETS})
    # avoid ninja problem with multiple rules generating the same target
    # only install from modules, but build into/from modules or modules/${TARGET}
    # after install one can ignore extra  modules/${TARGET}
    if(${TARGET} MATCHES ".*sharedf" )
      set_target_properties(${TARGET} PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modules_${TARGET})
    else()
      set_target_properties(${TARGET} PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modules)
    endif()
    target_include_directories(${TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES>
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/modules>
            $<INSTALL_INTERFACE:modules>
    )
    target_compile_options(${TARGET}
        PUBLIC $<$<COMPILE_LANG_AND_ID:Fortran,GNU>:${GNU_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,GNU>:${GNU_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,Intel>:${Intel_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,IntelLLVM>:${IntelLLVM_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,Intel>:${Intel_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,IntelLLVM>:${IntelLLVM_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,Flang>:${Flang_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,Clang>:${Clang_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,PGI>:${PGI_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,PGI>:${PGI_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,LLVMFlang>:${LLVM_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,NVHPC>:${PGI_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,NVHPC>:${PGI_CFLAGS}>
    )
#    add_dependencies(${TARGET} ${PROJECT_NAME}_build_info)
endforeach()

cmake_print_variables(TARGETS)
install(TARGETS ${TARGETS}
    EXPORT rmntools-targets
    INCLUDES DESTINATION include
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib # Defaults to lib64, so this is necessary for the links to work
    PUBLIC_HEADER DESTINATION include)

# Symbolic link for archive numbered version
function(add_static_lib_link target link_name)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ln -sfv lib${target}.a ${link_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${link_name} DESTINATION lib)
endfunction()

if(BUILD_STATIC_LIBS)
add_static_lib_link(rmntools librmntools_${PROJECT_VERSION}.a)
add_static_lib_link(rmntoolsf librmntoolsf_${PROJECT_VERSION}.a)
endif()

#install public includes
file(GLOB PROJECT_INCLUDE_FILES src/PUBLIC_INCLUDES/rmn/*)
install(FILES ${PROJECT_INCLUDE_FILES} DESTINATION include/rmn)

# Generate the config file for the project to be usable via cmake's find_package command
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")

install(EXPORT rmntools-targets DESTINATION ${CONFIG_INSTALL_DIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Fortran modules are installed in the modules or modules/TARGET directory
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modules/
        DESTINATION modules
        FILES_MATCHING PATTERN "*.mod"
        PATTERN "*.smod"
        )

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CONFIG_INSTALL_DIR}"
)

include(CTest)
add_subdirectory(tests)
