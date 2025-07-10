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
#include <rmn/bitstream.h>

uint32_t be_pack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options);
uint32_t be_pack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options);
uint32_t be_unpack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options);
uint32_t be_unpack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options);

uint32_t le_pack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options);
uint32_t le_pack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options);
uint32_t le_unpack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options);
uint32_t le_unpack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options);

uint32_t stream_pack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options){
  if(STREAM_IS_BIG_ENDIAN(*s)){
    return be_pack_u32(s, in, nbits, n, options) ;
  }
  if(STREAM_IS_LITTLE_ENDIAN(*s)){
    return le_pack_u32(s, in, nbits, n, options) ;
  }
  return 0 ;
}

uint32_t stream_unpack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options){
  if(STREAM_IS_BIG_ENDIAN(*s)){
    return be_unpack_u32(s, in, nbits, n, options) ;
  }
  if(STREAM_IS_LITTLE_ENDIAN(*s)){
    return le_unpack_u32(s, in, nbits, n, options) ;
  }
  return 0 ;
}

uint32_t stream_pack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options){
  if(STREAM_IS_BIG_ENDIAN(*s)){
    return be_pack_i32(s, in, nbits, n, options) ;
  }
  if(STREAM_IS_LITTLE_ENDIAN(*s)){
    return le_pack_i32(s, in, nbits, n, options) ;
  }
  return 0 ;
}

uint32_t stream_unpack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options){
  if(STREAM_IS_BIG_ENDIAN(*s)){
    return be_unpack_i32(s, in, nbits, n, options) ;
  }
  if(STREAM_IS_LITTLE_ENDIAN(*s)){
    return le_unpack_i32(s, in, nbits, n, options) ;
  }
  return 0 ;
}

#include<rmn/be_stream.h>
uint32_t be_pack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

uint32_t be_unpack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

uint32_t be_pack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

uint32_t be_unpack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

#include<rmn/le_stream.h>
uint32_t le_pack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

uint32_t le_unpack_u32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

uint32_t le_pack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

uint32_t le_unpack_i32(bitstream *s, void *in, int nbits, int n, uint32_t options){
}

