//
// Hopefully useful code for C
// Copyright (C) 2023-2024  Recherche en Prevision Numerique
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
// print messages from application, possibly duplicating them to a file
//
// void Lib_Log(
//     //! [in] Library id
//     const TApp_Lib Lib,
//     //! [in] Message level. See \ref TApp_LogLevel
//     const TApp_LogLevel Level,
//     //! [in] printf style format string
//     const char * const Format,
//     //! [in] Variables referrenced in the format string
//     ...
// )
// typedef enum {
//     //! Written even if the selected level is quiet
//     APP_VERBATIM = -1,
//     APP_ALWAYS = 0,
//     //! Fatal error. Will cause the application to be terminated.
//     APP_FATAL = 1,
//     //! System error. Will cause the application to be terminated.
//     APP_SYSTEM = 2,
//     //! Error. Written to stderr
//     APP_ERROR = 3,
//     //! Warning
//     APP_WARNING = 4,
//     //! Informational
//     APP_INFO = 5,
//     //! Stats about process
//     APP_STAT = 6,
//     //! Trivial
//     APP_TRIVIAL = 7,
//     //! Debug
//     APP_DEBUG = 8,
//     //! Extra
//     APP_EXTRA = 9,
//     //! Quiet \todo Say what quiet actually does
//     APP_QUIET = 10,
//     // ! Fatal error, and collect from all PEs
//     APP_COLLECT = 128
// } TApp_LogLevel;
// Lib_Log(APP_LIBRMN, APP_ERROR/APP_INFO/APP_WARNING, "%s", msg)

#define USE_APP

#if ! defined(TEE_PRINT_DEFINED)
#define TEE_PRINT_DEFINED

#include <stdio.h>
#include <stdint.h>

// print_diag is a WEAK entry point that can be overriden by a user supplied function
void print_diag(FILE *f, char *what, int32_t level) ;

int32_t set_msg_level(int32_t level);
int32_t set_tee_msg_level(int32_t level);
int32_t set_use_app(int32_t yes);

int32_t set_tee_auto_open(int32_t mode) ;
FILE *open_tee_file(char *fname) ;
FILE *set_tee_file(FILE *f) ;
FILE *get_tee_file(void) ;

void hexprintf_08(FILE *f, void *what, int n, char *msg, int level);
void hexprintf_16(FILE *f, void *what, int n, char *msg, int level);
void hexprintf_32(FILE *f, void *what, int n, char *msg, int level);
void hexprintf_64(FILE *f, void *what, int n, char *msg, int level);

#if defined(USE_APP)
#include <App.h>

#define TEE_ALWAYS  APP_ALWAYS
#define TEE_EXTRA   APP_EXTRA
#define TEE_DEBUG   APP_DEBUG
#define TEE_INFO    APP_INFO
#define TEE_WARNING APP_WARNING
#define TEE_ERROR   APP_ERROR
#define TEE_SYSTEM  APP_SYSTEM
#define TEE_FATAL   APP_FATAL

#define TEE_PRINTF(level, ...)        Lib_Log(APP_LIBRMN, (TApp_LogLevel)level, __VA_ARGS__)
#define TEE_FPRINTF( file, level,...) Lib_Log(APP_LIBRMN, (TApp_LogLevel)level, __VA_ARGS__)

#else

#define TEE_ALWAYS  0
#define TEE_EXTRA   9
#define TEE_DEBUG   8
#define TEE_INFO    5
#define TEE_WARNING 4
#define TEE_ERROR   3
#define TEE_SYSTEM  2
#define TEE_FATAL   1
void Lib_Log(
    //! [in] Library id
    const int32_t Lib,
    //! [in] Message level. See \ref TApp_LogLevel
    const int32_t Level,
    //! [in] printf style format string
    const char * const Format,
    //! [in] Variables referrenced in the format string
    ...
)

// "replacement" for printf
#define TEE_PRINTF(level, ...) { char _TeMp_[4096] ; snprintf(_TeMp_, sizeof(_TeMp_),  __VA_ARGS__) ; print_diag(stdout, _TeMp_, (int32_t)level) ; }

// "replacement" for fprintf
#define TEE_FPRINTF( file, level,...) { char _TeMp_[4096] ; snprintf(_TeMp_, sizeof(_TeMp_),  __VA_ARGS__) ; print_diag(file, _TeMp_, (int32_t)level) ; }

#endif

#endif
