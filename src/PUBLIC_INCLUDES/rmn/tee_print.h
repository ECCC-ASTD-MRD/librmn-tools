//
// Hopefully useful code for C
// Copyright (C) 2023-2026  Recherche en Prevision Numerique
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
// unless WITHOUT_APP is defined before inclusion, App will be used for messages
#if defined(WITHOUT_APP)
#undef WITH_APP
#else
#define WITH_APP
#endif

#if ! defined(TEE_PRINT_DEFINED)
#define TEE_PRINT_DEFINED

#include <stdio.h>
#include <stdint.h>

#if defined(WITH_APP)

// if WITH_APP is defined, TEE_PRINTF and TEE_FPRINTF WILL NOT USE User_Tee_Log

#include <App.h>
// TEE_xxx constants
#define TEE_LIBRMN  APP_LIBRMN
//
#define TEE_VERBATIM APP_VERBATIM
#define TEE_ALWAYS   APP_ALWAYS
#define TEE_EXTRA    APP_EXTRA
#define TEE_DEBUG    APP_DEBUG
#define TEE_STAT     APP_STAT
#define TEE_TRIVIAL  APP_TRIVIAL
#define TEE_INFO     APP_INFO
#define TEE_WARNING  APP_WARNING
#define TEE_ERROR    APP_ERROR
#define TEE_SYSTEM   APP_SYSTEM
#define TEE_FATAL    APP_FATAL

// "replacement" for printf, using Lib_Log
#define TEE_PRINTF(level, ...)     { set_diag_errno(level) ; Lib_Log(APP_LIBRMN, (TApp_LogLevel)level, __VA_ARGS__) ; }
// "replacement" for fprintf, using Lib_Log, ignores file
#define TEE_FPRINTF(file, level,...)  TEE_PRINTF(level, __VA_ARGS__)

#define LIB_LOG(lib, level, ...) Lib_Log(lib, level, __VA_ARGS__)
void Lib_Log(const TApp_Lib Lib, const TApp_LogLevel Level, const char * const Format, ...);

#define LIB_LOG_LEVEL(lib, level) Lib_LogLevelNo(lib, level)
int  Lib_LogLevelNo(TApp_Lib Lib, TApp_LogLevel Level);

#define APP_LOG_LEVEL(level) App_LogLevelNo(level)
int  App_LogLevelNo(const TApp_LogLevel Level);

#else    // defined(WITH_APP)

// if WITH_APP is not defined, TEE_PRINTF and TEE_FPRINTF WILL USE User_Tee_Log

// User_Tee_Log "replacement" for printf, uses msg_file if not NULL, stdout otherwise
#define TEE_PRINTF(level, ...) \
    { char _TeMp_[4096] ; FILE *mfile = get_msg_file() ; \
      snprintf(_TeMp_, sizeof(_TeMp_), __VA_ARGS__) ; _TeMp_[4095] = '\0' ; \
      set_diag_errno(level) ; \
      User_Tee_Log(mfile ? mfile : stdout, _TeMp_, level) ; \
    }

// User_Tee_Log "replacement" for fprintf, uses file if not NULL, stdout otherwise
#define TEE_FPRINTF( file, level,...) \
    { char _TeMp_[4096] ; \
      snprintf(_TeMp_, sizeof(_TeMp_), __VA_ARGS__) ; _TeMp_[4095] = '\0' ; \
      set_diag_errno(level) ; \
      User_Tee_Log(file ? file : stdout, _TeMp_, level) ; \
    }

// needed if not including App.h (borrowed from App.h)
typedef enum {
    APP_LIBRMN = 1,
    TEE_LIBRMN = 1
} TApp_Lib ;
typedef enum {
    TEE_VERBATIM = -1,
    TEE_ALWAYS   =  0,   // identified as INFO in logs
    TEE_FATAL    =  1,
    TEE_SYSTEM   =  2,
    TEE_ERROR    =  3,
    TEE_WARNING  =  4,
    TEE_INFO     =  5,
    TEE_STAT     =  6,
    TEE_TRIVIAL  =  7,
    TEE_DEBUG    =  8,
    TEE_EXTRA    =  9,
} TApp_LogLevel ;
//
#define APP_VERBATIM TEE_VERBATIM
#define APP_ALWAYS   TEE_ALWAYS
#define APP_EXTRA    TEE_EXTRA
#define APP_DEBUG    TEE_DEBUG
#define APP_STAT     TEE_STAT
#define APP_TRIVIAL  TEE_TRIVIAL
#define APP_INFO     TEE_INFO
#define APP_WARNING  TEE_WARNING
#define APP_ERROR    TEE_ERROR
#define APP_SYSTEM   TEE_SYSTEM
#define APP_FATAL    TEE_FATAL

#define LIB_LOG(lib, level, ...) Lib_Log(lib, level, __VA_ARGS__)
void Lib_Log(const TApp_Lib Lib, const TApp_LogLevel Level, const char * const Format, ...);

#define LIB_LOG_LEVEL(lib, level) Lib_LogLevelNo(lib, level)
int  Lib_LogLevelNo(TApp_Lib Lib, TApp_LogLevel Level);

#define APP_LOG_LEVEL(level) App_LogLevelNo(level)
int  App_LogLevelNo(const TApp_LogLevel Level);

#endif     // defined(WITH_APP)

// User_Tee_Log is a WEAK entry point that can be overriden by a user supplied function
// a default User_Tee_Log function is provided by tee_print.c
void User_Tee_Log(FILE *f, char *what, TApp_LogLevel level) ;
// package_log returns composite error code (using id and msg)
int package_log(char *pkg, int id, char *msg, TApp_LogLevel level);

// level as first argument, consistent with TEE_DIAG, TEE_PRINTF (package_log calls User_Tee_Log)
#define TEE_PKG_LOG(level, pkg, id, msg) { set_diag_errno(level) ; package_log(pkg, id, msg, level) ; }

// whether WITH_APP is defined or not, TEE_DIAG ALWAYS USES User_Tee_Log
// "replacement" for printf, uses User_Tee_Log, uses msg_file if not NULL, stdout otherwise
#define TEE_DIAG(level, ...)  \
    { char _TeMp_[4096] ; FILE *mfile = get_msg_file() ; \
      snprintf(_TeMp_, sizeof(_TeMp_), __VA_ARGS__) ; _TeMp_[4095] = '\0' ; \
      set_diag_errno(level) ; \
      User_Tee_Log(mfile ? mfile : stdout, _TeMp_, level) ; \
    }
// use App for messages if yes  != 0, otherwise use User_Tee_Log
int32_t get_use_app(void);
int32_t set_use_app(int32_t yes);

// duplicate stdout messages to a file
FILE *set_msg_file(FILE *f);
FILE *get_msg_file(void);
// get printable name of message level
char *msg_level_name(TApp_LogLevel level);

// get message level
TApp_LogLevel get_msg_level(void);
TApp_LogLevel set_msg_level(TApp_LogLevel level);
// get message level for messages also sent to tee file
TApp_LogLevel get_tee_msg_level(void);
TApp_LogLevel set_tee_msg_level(TApp_LogLevel level);

// activate tee file auto open functionality if mode != 0
int32_t set_tee_auto_open(int32_t mode) ;
// open a tee log file (if fname == NULL, generate name automatically)
FILE *open_tee_file(char *fname) ;
// set tee log file ao an open file
FILE *set_tee_file(FILE *f) ;
// get tee file descriptor pointer
FILE *get_tee_file(void) ;

// set errno to -1 if value is 0 when level is SYSTEM
void set_diag_errno(TApp_LogLevel level);
// print to file in hex using 2/4/8/16 characters for item length 8/16/32/64 bits
// set errno value to use for SYSTEM level messages
int32_t set_tee_errno(int32_t err_val);

void hexprintf_08(FILE *f, void *what, int n, char *msg, TApp_LogLevel level);
void hexprintf_16(FILE *f, void *what, int n, char *msg, TApp_LogLevel level);
void hexprintf_32(FILE *f, void *what, int n, char *msg, TApp_LogLevel level);
void hexprintf_64(FILE *f, void *what, int n, char *msg, TApp_LogLevel level);

#endif     // defined(TEE_PRINT_DEFINED)
