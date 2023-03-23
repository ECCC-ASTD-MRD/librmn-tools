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
#if ! defined(TEE_PRINT_DEFINED)
#define TEE_PRINT_DEFINED

#include <stdio.h>

void print_diag(FILE *f, char *what, int level) ;  // print_diag is a WEAK entry point

int32_t set_tee_auto_open(int32_t mode) ;
FILE *open_tee_file(char *fname) ;
FILE *set_tee_file(FILE *f) ;
FILE *get_tee_file(void) ;

// "replacement" for printf
#define TEE_PRINTF(level, ...) { char _TeMp_[4096] ; snprintf(_TeMp_, sizeof(_TeMp_),  __VA_ARGS__) ; print_diag(stdout, _TeMp_, level) ; }

// "replacement" for fprintf
#define TEE_FPRINTF(level, file, ...) { char _TeMp_[4096] ; snprintf(_TeMp_, sizeof(_TeMp_),  __VA_ARGS__) ; print_diag(file, _TeMp_, level) ; }

#endif
