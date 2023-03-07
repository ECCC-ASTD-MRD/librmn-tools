/*
 * Copyright (C) 2023  Environnement et Changement climatique Canada
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Author:
 *     M. Valin,   Recherche en Prevision Numerique, 2023
 *
 * attempt to identify the C compiler used to compile code including this file
*/
#if ! defined(IDENTIFY_C_COMPILER)
#define IDENTIFY_C_COMPILER

#if defined(__x86_64__)
static int ADDRESS_MODE = 64 ;

#elif defined(__i386__)
static int ADDRESS_MODE = 32 ;

#else
static int ADDRESS_MODE = -1 ;

#endif

#if defined(__INTEL_LLVM_COMPILER)
  static char C_COMPILER[] = "INTEL_ICX" ;  // icx

#elif defined(__INTEL_COMPILER)
  static char C_COMPILER[] = "INTEL_ICC" ;  // icc

#elif defined(__PGI)
static char C_COMPILER[] = "PGI/Nvidia" ;   // pgcc nvcc

#elif defined(__clang__)
static char C_COMPILER[] = "CLANG" ;        // llvm/aocc clang

#elif defined(__GNUC__)
static char C_COMPILER[] = "GNU" ;          // gcc or lookalike

#else
static char C_COMPILER[] = "UNKNOWN" ;      // unknown

#endif

#endif
