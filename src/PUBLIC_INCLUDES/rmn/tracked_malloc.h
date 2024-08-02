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
// support of tracked memory blocks
//
#if ! defined(TRACKED_BLOCK_KEEP)

#include <stdint.h>
#include <stdlib.h>

#define TRACKED_BLOCK_KEEP_DATA  1

int tracked_block_check(void *p);
void *tracked_block_alloc(size_t size);
void *tracked_block_resize(void *p, size_t size, int options);
int tracked_block_free(void *p);

#endif
