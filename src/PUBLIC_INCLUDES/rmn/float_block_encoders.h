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

uint16_t *float_encode_4x4(float *f, int lni, int nbits, uint16_t *stream, uint32_t ni, uint32_t nj, uint32_t *head);
uint16_t *float_decode_4x4(float *f, int nbits, void *stream, uint32_t *head);
