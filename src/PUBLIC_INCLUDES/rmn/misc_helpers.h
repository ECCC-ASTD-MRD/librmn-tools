/* 
 * Copyright (C) 2026 Recherche en Prevision Numerique
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
// helper functions
#include <stdint.h>

// hash an unsigned integer into an unsigned integer
// value should be non zero
// value [IN] : unsigned integer to hash
// nbits [IN} : number of bits to keep (1 -> 32)
static inline uint32_t kwik_hash(uint32_t value, uint32_t nbits)
{
  if(nbits == 0 || nbits > 32) return 0 ;
  uint64_t t = value ;
  t *= 25214903917L ;                       // next in sequence
  uint32_t t32 = t >> 4 ;                   // keep bits 35 : 04
  t32 >>= (32 - nbits) ;                    // keep bits 35 : 35-nbits+1
  return (t32) ;
}
