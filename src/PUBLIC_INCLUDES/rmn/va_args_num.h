//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//

// VA_ARGS_NUM returns the number of its arguments (up to 20)

#if ! defined(VA_ARGS_NUM)

#if 0
// this will work if all arguments are integers and there is at least one argument
#define NuMvArG(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))
#endif

#undef VA_ARGS_NUM_
#define VA_ARGS_NUM_(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, N, ...)    N
#define VA_ARGS_NUM(...)  VA_ARGS_NUM_(__VA_ARGS__ __VA_OPT__(,) 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#endif
