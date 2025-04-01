//
// Copyright (C) 2025  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#if ! defined(CONCAT2_)

#define CONCAT2_(A,B) A##B
#define CONCAT2(A,B) CONCAT2_(A,B)

#define CONCAT3_(A,B,C) A##B##C
#define CONCAT3(A,B,C) CONCAT3_(A,B,C)

#define CONCAT4_(A,B,C,D) A##B##C##D
#define CONCAT4(A,B,C) CONCAT4_(A,B,C,D)

#endif
