/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __BREW__
#define __BREW__

#include <stdarg.h>
#include <sys/stat.h>
#include "AEEFile.h"

#ifdef AEE_SIMULATOR
#define __func__ __FUNCTION__
#endif

#undef WIN32
#undef _WIN32

//#undef _MSC_VER

#define REFRESH_RATE 20

#define false 0
#define true 1

#undef FILE
#define FILE IFile

typedef uint32 mode_t;

#define SEEK_CUR _SEEK_CURRENT
#define SEEK_SET _SEEK_START
#define SEEK_END _SEEK_END
#define EOF -1

#ifdef _DEBUG
#define BREW_LIB(FUNC) BREW_##FUNC
#else
#define BREW_LIB(FUNC) BREW_##FUNC
#endif

unsigned int BREW_GetTicks();
void BREW_SetTimerCallback(void (*pfn)(void));
void *BREW_malloc(size_t size);
void BREW_free(void *ptr);
void *BREW_memmove(void *destination, const void *source, size_t num);
void *BREW_memcpy(void *destination, const void *source, size_t num);
int BREW_atoi(const char *str);
char *BREW_strcpy(char *destination, const char *source);
size_t BREW_strlen(const char *str);
int BREW_sprintf(char *str, const char *format, ...);
int BREW_printf(const char *format, ...);
int BREW_strncasecmp(const char *string1, const char *string2, size_t count);
int BREW_strcasecmp(const char *string1, const char *string2);
void *BREW_memset(void *ptr, int value, size_t num);
void *BREW_calloc(size_t m, size_t n);
void *BREW_memchr(void *ptr, int value, size_t num);
char *BREW_strcat(char *destination, const char *source);
int BREW_strncmp(const char *str1, const char *str2, size_t num);
const char *BREW_strchr(const char *str, int character);
int BREW_memcmp(const void *ptr1, const void *ptr2, size_t num);
boolean BREW_isalpha(int c);
boolean BREW_isspace(int c);
boolean BREW_ispunct(int c);
char *BREW_strncpy(char *destination, const char *source, size_t num);
int BREW_strcmp(const char *str1, const char *str2);
int BREW_snprintf(char *s, size_t n, const char *format, ...);
int BREW_vsnprintf(char *s, size_t n, const char *format, va_list arg);
char *BREW_strlwr(char *str);
char *BREW_getenv(const char *name);
double BREW_pow(double x, double y);
double BREW_sqrt(double x);
char *BREW_strstr(char *str1, const char *str2);
FILE *BREW_fopen(const char *filename, const char *mode);
size_t BREW_fread(void *ptr, size_t size, size_t count, FILE *stream);
int BREW_fclose(FILE *stream);
int BREW_access(const char *pathname, int mode);
int BREW_fseek(FILE *stream, long int offset, unsigned int origin);
void BREW_exit(int status);
int BREW_fstat(IFile *fd, struct stat *buf);
char *BREW_strtok(char *str, const char *delimiters);
int BREW_fprintf(FILE *stream, const char *format, ...);
int BREW_sscanf(const char *s, const char *format, ...);
char *BREW_fgets(char *str, int num, FILE *stream);
int BREW_fgetc(FILE *stream);
size_t BREW_fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int BREW_fscanf(FILE *stream, const char *format, ...);
long int BREW_ftell(FILE *stream);
int BREW_feof(FILE *stream);
int BREW_fputc(int c, FILE *stream);
char *BREW_strerror(int errnum);
int BREW_atexit(void (*func)(void));
void BREW___assert_func(const char *a, int b, const char *c, const char *d);
int BREW_remove(const char *filename);
int BREW_mkdir(const char *_path, mode_t __mode);

//#define malloc BREW_LIB(malloc)
//#define free BREW_LIB(free)
#define memmove BREW_LIB(memmove)
#define memcpy BREW_LIB(memcpy)
#define atoi BREW_LIB(atoi)
#define strcpy BREW_LIB(strcpy)
#define strlen BREW_LIB(strlen)
#define sprintf BREW_LIB(sprintf)
#define printf BREW_LIB(printf)
#define strncasecmp BREW_LIB(strncasecmp)
#define strcasecmp BREW_LIB(strcasecmp)
#define memset BREW_LIB(memset)
//#define calloc BREW_LIB(calloc)
#define memchr BREW_LIB(memchr)
#define strcat BREW_LIB(strcat)
#define strncmp BREW_LIB(strncmp)
#define strchr BREW_LIB(strchr)
#define memcmp BREW_LIB(memcmp)
#define isalpha BREW_LIB(isalpha)
#define isspace BREW_LIB(isspace)
#define ispunct BREW_LIB(ispunct)
#define strncpy BREW_LIB(strncpy)
#define strcmp BREW_LIB(strcmp)
#define snprintf BREW_LIB(snprintf)
#define vsnprintf BREW_LIB(vsnprintf)
#define strlwr BREW_LIB(strlwr)
#define getenv BREW_LIB(getenv)
#define pow BREW_LIB(pow)
#define sqrt BREW_LIB(sqrt)
#define strstr BREW_LIB(strstr)
#define fopen BREW_LIB(fopen)
#define fread BREW_LIB(fread)
#define fclose BREW_LIB(fclose)
#define access BREW_LIB(access)
#define fseek BREW_LIB(fseek)
#define exit BREW_LIB(exit)
#define fstat BREW_LIB(fstat)
#define strtok BREW_LIB(strtok)
#define fprintf BREW_LIB(fprintf)
#define sscanf BREW_LIB(sscanf)
#define fgets BREW_LIB(fgets)
#define fgetc BREW_LIB(fgetc)
#define fwrite BREW_LIB(fwrite)
#define fscanf BREW_LIB(fscanf)
#define ftell BREW_LIB(ftell)
#define feof BREW_LIB(feof)
#define fputc BREW_LIB(fputc)
#define remove BREW_LIB(remove)
#define strerror BREW_LIB(strerror)
#define atexit BREW_LIB(atexit)
#define mkdir BREW_LIB(mkdir)
#define __assert_func BREW_LIB(__assert_func)

#endif
