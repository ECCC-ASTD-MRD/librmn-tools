// Hopefully useful code for C
// Copyright (C) 2024  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// used by block movers and array_nd
//
#if ! defined(DATA_KINDS_DEFINED)
#define DATA_KINDS_DEFINED

// expected data type codes
typedef enum {
  bad_data   = 0,     // invalid
  int_data   = 1,     // 32 bit signed integers
  uint_data  = 2,     // 32 bit unsigned integers
  float_data = 3,     // 32 bit floats
  raw_data   = 4,     // any 32 bit quantities (block_properties likely to be meaningless)
  large_data = 5,     // multiple of 32 bit quantities (block_properties meaningless)
  any_data   = 6      // unknown or unspecified
} data_kind ;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *printable_type[7] = { "INVALID", "INT_32", "UINT_32", "FLOAT_32", "RAW_32", "LARGE", "UNKNOWN" } ;
#pragma GCC diagnostic pop

#endif
