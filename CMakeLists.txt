# The minimum version can not be 3.16 because of the LINK_LANGUAGE generator expression in Config.cmake.in
cmake_minimum_required(VERSION 3.20)

# Append EC specific module path
list(APPEND CMAKE_MODULE_PATH $ENV{EC_CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake_rpn/modules)

if(NOT DEFINED COMPILER_SUITE)
  set(COMPILER_SUITE "gnu")
  message(STATUS "(rmntools) COMPILER_SUITE not defined, set to gnu" )
endif()

include(ec_init)           # Initialise compilers and ec specific functions
ec_git_version()           # Get version from git state
ec_parse_manifest()        # Parse MANIFEST file

project(${NAME} DESCRIPTION "${DESCRIPTION}" LANGUAGES C Fortran)
set(PROJECT_VERSION ${VERSION}${STATE})
set(rmntools_VERSION ${PROJECT_VERSION} CACHE INTERNAL "rmntools version" FORCE) # Needed for cascaded version identification
message(STATUS "(EC) ${PROJECT_NAME} version = ${PROJECT_VERSION}")

ec_build_info()            # Generate build information

include(ec_compiler_presets)

set(WITH_OPENMP TRUE CACHE BOOL "Control whether to use OpenMP")
include(ec_openmp)          # Enable OpenMP (-DWITH_OPENMP=TRUE)
message(STATUS "(EC ${PROJECT_NAME}) WITH_OPENMP=${WITH_OPENMP}" )
#include(ec_doxygen)         # Doxygen target doc

# Fortran compiler options required to build this project (used in tets also)
if(WITH_OPENMP)
  set(Intel_FFLAGS -convert big_endian -align array32byte -assume byterecl -qopenmp)
  set(Intel_CFLAGS -qopenmp)
  set(GNU_FFLAGS -fconvert=big-endian -fcray-pointer -frecord-marker=4 -fno-second-underscore -fopenmp)
  set(GNU_CFLAGS -fopenmp)
  set(Flang_FFLAGS -Mbyteswapio -fopenmp)
  set(Clang_CFLAGS -fopenmp)
  set(PGI_FFLAGS -byteswapio -mp)
  set(PGI_CFLAGS -mp)
  set(LLVM_FFLAGS -byteswapio)
else()
  set(Intel_FFLAGS -convert big_endian -align array32byte -assume byterecl)
  set(GNU_FFLAGS -fconvert=big-endian -fcray-pointer -frecord-marker=4 -fno-second-underscore -ldl -pthread)
  set(Flang_FFLAGS -Mbyteswapio)
  set(PGI_FFLAGS -byteswapio)
  set(LLVM_FFLAGS -fconvert=big-endian)
endif()

# ======================== PROJECT FILES ======================
add_subdirectory(tests)

include_directories(
   include
   src/PUBLIC_INCLUDES
)
set(MY_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../src/PUBLIC_INCLUDES )

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

# Fortran static library (will make calls to C library)
add_library(rmntoolsf STATIC ${PROJECT_F_FILES})
set_target_properties(rmntoolsf PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME rmntoolsf)

# C shared library
add_library(rmntoolsshared SHARED ${PROJECT_C_FILES})
set_target_properties(rmntoolsshared PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntools)

# Fortran shared library, needs to link to C shared library
add_library(rmntoolssharedf SHARED ${PROJECT_F_FILES})
set_target_properties(rmntoolssharedf PROPERTIES
    LINK_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/librmntools.so;
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/PUBLIC_INCLUDES/rmntools.h
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${SOVERSION}
    OUTPUT_NAME rmntoolsf)

# Required to prevent missing Fortran modules when building in parallel
# (make -j) with GNU compilers
add_dependencies(rmntoolssharedf rmntoolsshared)

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
    add_dependencies(${TARGET}
        ${PROJECT_NAME}_build_info)
endforeach()

install(TARGETS ${TARGETS})

# Symbolic link for archive numbered version
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ln -sfv lib${PROJECT_NAME}.a lib${PROJECT_NAME}_${PROJECT_VERSION}.a)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_${PROJECT_VERSION}.a DESTINATION lib)

#install public includes
file(GLOB PROJECT_INCLUDE_FILES src/PUBLIC_INCLUDES/rmn/*)
install(FILES ${PROJECT_INCLUDE_FILES} DESTINATION include/rmn)

# Generate the config file for the project to be usable via cmake's find_package command
set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)
set(CONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
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

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/tests/test_c"
        "${CMAKE_CURRENT_BINARY_DIR}/tests/test_f"
        "${CMAKE_CURRENT_BINARY_DIR}/tests/test_identify_compiler_c"
        "${CMAKE_CURRENT_BINARY_DIR}/tests/test_identify_compiler_f"
        "${CMAKE_CURRENT_BINARY_DIR}/tests/test_timers"
    DESTINATION "${BIN_INSTALL_DIR}"
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)
