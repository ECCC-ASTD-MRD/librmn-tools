cmake_minimum_required(VERSION 3.20)

include(CMakePrintHelpers)
# Append EC specific module path
list(APPEND CMAKE_MODULE_PATH $ENV{EC_CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake_rpn/modules)

# It was setting my COMPILER_SUITE to GNU even if I had loaded the intel ssm
# package, which caused find_package(OpenMP REQUIRED) to fail.  I didn't want
# to bother troubleshooting that so I commented this part:
# if(NOT DEFINED COMPILER_SUITE)
#   set(COMPILER_SUITE "gnu")
#   message(STATUS "(rmntools) COMPILER_SUITE not defined, set to gnu" )
# endif()

include(ec_init)           # Initialise compilers and ec specific functions
ec_git_version()           # Get version from git state
ec_parse_manifest()        # Parse MANIFEST file

project(${NAME} DESCRIPTION "${DESCRIPTION}" LANGUAGES C Fortran)
set(PROJECT_VERSION ${VERSION}${STATE})
set(rmntools_VERSION ${PROJECT_VERSION} CACHE INTERNAL "rmntools version" FORCE) # Needed for cascaded version identification
message(STATUS "(EC) ${PROJECT_NAME} version = ${PROJECT_VERSION}")

ec_build_info()            # Generate build information

include(ec_compiler_presets)
option(NO_SIMD "Control the use of SIMD.  Set to ON to use pure C code" OFF)
if(NOT NO_SIMD)
    message(STATUS "NOTE: Project will be built with SIMD, re-run CMake with -DNO_SIMD=ON to use pure C code")
    # If you really want people to see it, but it's pretty scary
    # message(WARNING "Project will be built with SIMD, re-run CMake with -DNO_SIMD=ON to use pure C code")
endif()

set(WITH_OPENMP TRUE CACHE BOOL "Control whether to use OpenMP")
include(ec_openmp)          # Enable OpenMP (-DWITH_OPENMP=TRUE)
message(STATUS "(EC ${PROJECT_NAME}) WITH_OPENMP=${WITH_OPENMP}" )
#include(ec_doxygen)         # Doxygen target doc

# Fortran compiler options required to build this project (also used in tests)
if(WITH_OPENMP)
  set(Intel_FFLAGS -convert big_endian -align array32byte -assume byterecl -qopenmp)
  set(Intel_CFLAGS -qopenmp)
  set(GNU_FFLAGS -fconvert=big-endian -fcray-pointer -frecord-marker=4 -fno-second-underscore -fopenmp)
  set(GNU_CFLAGS -fopenmp)
  set(Flang_FFLAGS -Mbyteswapio -fopenmp)
  set(Clang_CFLAGS -fopenmp)
  set(PGI_FFLAGS -byteswapio -mp)
  set(PGI_CFLAGS -mp)
  set(LLVM_FFLAGS -fconvert=big-endian -fopenmp)
  set(LLVM_CFLAGS -fopenmp)
else()
  set(Intel_FFLAGS -convert big_endian -align array32byte -assume byterecl)
  set(GNU_FFLAGS -fconvert=big-endian -fcray-pointer -frecord-marker=4 -fno-second-underscore -ldl -pthread)
  set(Flang_FFLAGS -Mbyteswapio)
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
file(GLOB PROJECT_C_FILES
   src/*.c
   src/*/*.c
)
file(GLOB PROJECT_F_FILES
   src/*.F90
   src/*/*.F90
)
file(GLOB PROJECT_H_FILES
   src/*/*.h
   src/*/*/*.h
)
# ================================================================

string(REGEX MATCH "([0-9]+)" SOVERSION ${PROJECT_VERSION})

set(CMAKE_BUILD_RPATH_USE_ORIGIN true)
# set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
set(CMAKE_INSTALL_RPATH "$ORIGIN")

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

# Fortran static library
add_library(rmntoolsf STATIC ${PROJECT_F_FILES})
set_target_properties(rmntoolsf PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME rmntoolsf)
# (will make calls to C library)
target_link_libraries(rmntoolsf PUBLIC rmntools)
if(WITH_OPENMP)
    target_compile_definitions(rmntoolsf PUBLIC WITH_OPENMP)
    target_link_libraries(rmntoolsf PUBLIC OpenMP::OpenMP_Fortran)
endif()

# C shared library
add_library(rmntoolsshared SHARED $<TARGET_OBJECTS:rmntools>)
set_target_properties(rmntoolsshared PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntools)
if(WITH_OPENMP)
    target_compile_definitions(rmntoolsshared PUBLIC WITH_OPENMP)
    target_link_libraries(rmntoolsshared PUBLIC OpenMP::OpenMP_C)
endif()

# Fortran shared library
add_library(rmntoolssharedf SHARED $<TARGET_OBJECTS:rmntoolsf>)
set_target_properties(rmntoolssharedf PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntoolsf)
# needs to link to C shared library
target_link_libraries(rmntoolssharedf PUBLIC rmntoolsshared)
if(WITH_OPENMP)
    target_compile_definitions(rmntoolssharedf PUBLIC WITH_OPENMP)
    target_link_libraries(rmntoolssharedf PUBLIC OpenMP::OpenMP_Fortran)
endif()


set(TARGETS rmntools rmntoolsf rmntoolsshared rmntoolssharedf)

foreach(TARGET IN ITEMS ${TARGETS})
    set_target_properties(${TARGET} PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modules/${TARGET})
    target_compile_options(${TARGET}
        PUBLIC $<$<COMPILE_LANG_AND_ID:Fortran,GNU>:${GNU_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,GNU>:${GNU_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,Intel>:${Intel_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,Intel>:${Intel_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,Flang>:${Flang_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,Clang>:${Clang_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,PGI>:${PGI_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,PGI>:${PGI_CFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,LLVMFlang>:${LLVM_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:Fortran,NVHPC>:${PGI_FFLAGS}>
               $<$<COMPILE_LANG_AND_ID:C,NVHPC>:${PGI_CFLAGS}>
    )
    if(NO_SIMD)
        target_compile_definitions(${TARGET} PUBLIC NO_SIMD)
    endif()
    target_include_directories(${TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES>
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/modules/${TARGET}>
            $<INSTALL_INTERFACE:modules/${TARGET}>
    )
    # add_dependencies(${TARGET} ${PROJECT_NAME}_build_info)
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
add_static_lib_link(rmntools librmntools_${PROJECT_VERSION}.a)
add_static_lib_link(rmntoolsf librmntoolsf_${PROJECT_VERSION}.a)

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

# Fortran modules are installed in the modules/TARGET directory
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
