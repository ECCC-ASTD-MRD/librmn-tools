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

#include <stddef.h>

size_t CopyFileData(int fdi, int fdo, size_t nbytes) ;
size_t SparseConcatFd(int fdi, int fdo, int diag) ;
size_t SparseConcatFile(const char *name1, const char *name2, const int diag) ;
int SparseConcat_main(int argc, char **argv) ;
