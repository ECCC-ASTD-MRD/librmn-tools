/*
 * Hopefully useful code for C
 * Copyright (C) 2023  Recherche en Prevision Numerique
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * print messages from application, possibly duplicating them to a file
 * 
 */
#include <rmn/tee_print.h>

// print bytes / halfwords / words / doublewords in hexadecimal ( 68 - 96 characters per line)

// print n bytes into file f in hexadecimal format
// f     [IN] : file to print into
// what  [IN] : address of data ro be printed
// n     [IN] : number of items to print
// level [IN] : message level TEE_DEBUG/.../TEE_FATAL (see rmn/tee_print.h)
void hexprintf_08(FILE *f, void *what, int n, char *msg, int level){
  uint8_t *c08 = (uint8_t *) what ;
  int i ;
  TEE_FPRINTF(f, level, " %s", msg) ;
  for(i=0 ; i<n ; i++) { TEE_FPRINTF(f, level, " %2.2x",c08[i]); if((i&31) == 31) TEE_FPRINTF(f, level, "\n"); }
  TEE_FPRINTF(f, level, "\n");
}

// print n halfwords into file f in hexadecimal format
// f     [IN] : file to print into
// what  [IN] : address of data ro be printed
// n     [IN] : number of items to print
// level [IN] : message level TEE_DEBUG/.../TEE_FATAL (see rmn/tee_print.h)
void hexprintf_16(FILE *f, void *what, int n, char *msg, int level){
  uint16_t *h16 = (uint16_t *) what ;
  int i ;
  TEE_FPRINTF(f, level, " %s", msg) ;
  for(i=0 ; i<n ; i++) { TEE_FPRINTF(f, level, " %4.4x",h16[i]); if((i&15) == 15) TEE_FPRINTF(f, level, "\n"); }
  TEE_FPRINTF(f, level, "\n");
}

// print n words into file f in hexadecimal format
// f     [IN] : file to print into
// what  [IN] : address of data ro be printed
// n     [IN] : number of items to print
// level [IN] : message level TEE_DEBUG/.../TEE_FATAL (see rmn/tee_print.h)
void hexprintf_32(FILE *f, void *what, int n, char *msg, int level){
  uint32_t *w32 = (uint32_t *) what ;
  int i ;
  TEE_FPRINTF(f, level, " %s", msg) ;
  for(i=0 ; i<n ; i++) { TEE_FPRINTF(f, level, " %8.8x",w32[i]); if((i&7) == 7) TEE_FPRINTF(f, level, "\n"); }
  TEE_FPRINTF(f, level, "\n");
}

// print n doublewords into file f in hexadecimal format
// f     [IN] : file to print into
// what  [IN] : address of data ro be printed
// n     [IN] : number of items to print
// level [IN] : message level TEE_DEBUG/.../TEE_FATAL (see rmn/tee_print.h)
void hexprintf_64(FILE *f, void *what, int n, char *msg, int level){
  uint64_t *l64 = (uint64_t *) what ;
  int i ;
  TEE_FPRINTF(f, level, " %s", msg) ;
  for(i=0 ; i<n ; i++) { TEE_FPRINTF(f, level, " %16.16lx",l64[i]); if((i&31) == 3) TEE_FPRINTF(f, level, "\n"); }
  TEE_FPRINTF(f, level, "\n");
}
