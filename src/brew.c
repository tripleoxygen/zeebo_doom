/*
 *  SPDX-FileCopyrightText: Copyright 2012-2023 Fausto "O3" Ribeiro | OpenZeebo <zeebo@tripleoxygen.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifdef AEE_SIMULATOR
#include <windows.h>
#endif

#include "brew.h"
#include "AEEStdLib.h"
#include "prboom.h"

unsigned int BREW_GetTicks()
{
  return ISHELL_GetUpTimeMS(pApp->a.m_pIShell);
}

void BREW_SetTimerCallback(void (*pfn)(void))
{
  ISHELL_SetTimer(pApp->a.m_pIShell, REFRESH_RATE, (PFNNOTIFY)pfn, NULL);
}

int errno;

void *BREW_malloc(size_t size) { return MALLOC(size); }

void BREW_free(void *ptr) { FREE(ptr); }

void *BREW_memmove(void *destination, const void *source, size_t num)
{
  return MEMMOVE(destination, source, num);
}

void *BREW_memcpy(void *destination, const void *source, size_t num)
{
  return MEMCPY(destination, source, num);
}

int BREW_atoi(const char *str) { return ATOI(str); }

char *BREW_strcpy(char *destination, const char *source)
{
  return STRCPY(destination, source);
}

size_t BREW_strlen(const char *str) { return STRLEN(str); }

int BREW_sprintf(char *str, const char *format, ...)
{
  int ret;

  va_list args;
  va_start(args, format);
  ret = VSPRINTF(str, format, args);
  va_end(args);

  return ret;
}

int BREW_printf(const char *format, ...)
{
  char message[512];
  int ret;

  va_list args;
  va_start(args, format);
  ret = VSNPRINTF(message, sizeof(message), format, args);
#ifdef AEE_SIMULATOR
  OutputDebugStringA(message);
#else
  DBGPRINTF(message);
#endif
  va_end(args);

  return ret;
}

int BREW_strncasecmp(const char *string1, const char *string2, size_t count)
{
  return STRNICMP(string1, string2, count);
}

int BREW_strcasecmp(const char *string1, const char *string2)
{
  return STRICMP(string1, string2);
}

void *BREW_memset(void *ptr, int value, size_t num)
{
  return MEMSET(ptr, value, num);
}

void *BREW_calloc(size_t m, size_t n)
{
  void *p = NULL;

  if (n && m > (size_t)-1 / n)
  {
    // errno = -1;
    return 0;
  }
  n *= m;
  p = BREW_malloc(n);

  if (!p)
    return p;
  // n = mal0_clear(p, n);
  return BREW_memset(p, 0, n);
}

void *BREW_memchr(void *ptr, int value, size_t num)
{
  return MEMCHR(ptr, value, num);
}

char *BREW_strcat(char *destination, const char *source)
{
  return STRCAT(destination, source);
}

int BREW_strncmp(const char *str1, const char *str2, size_t num)
{
  return STRNCMP(str1, str2, num);
}

const char *BREW_strchr(const char *str, int character) { return STRCHR(str, character); }

int BREW_memcmp(const void *ptr1, const void *ptr2, size_t num)
{
  return MEMCMP(ptr1, ptr2, num);
}

boolean BREW_isalpha(int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

boolean BREW_isspace(int c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\n' || c == '\r' ||
         c == '\f' || c == '\v';
}

boolean BREW_ispunct(int c)
{
  static const char *punct = ".;!?...";
  return strchr(punct, c) == NULL ? false : true; // you can make this shorter
}

char *BREW_strncpy(char *destination, const char *source, size_t num)
{
  return STRNCPY(destination, source, num);
}

int BREW_strcmp(const char *str1, const char *str2) { return STRCMP(str1, str2); }

int BREW_snprintf(char *s, size_t n, const char *format, ...)
{
  int ret;

  va_list args;
  va_start(args, format);
  ret = VSNPRINTF(s, n, format, args);
  va_end(args);

  return ret;
}

int BREW_vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
  return VSNPRINTF(s, n, format, arg);
}

char *BREW_strlwr(char *str) { return STRLOWER(str); }

int *__errno() { return &errno; }

char *BREW_getenv(const char *name) { return NULL; }

double BREW_pow(double x, double y) { return FPOW(x, y); }

char *BREW_strstr(char *str1, const char *str2) { return STRSTR(str1, str2); }

FILE *BREW_fopen(const char *filename, const char *mode)
{
  IFile *file = NULL;
  FileInfo pFileInfo;
  OpenFileMode ofm;

  switch (mode[0])
  {
  case 'w':
    ofm = _OFM_READWRITE;
    break;

  case 'a':
    ofm = _OFM_APPEND;
    break;

  default:
    ofm = _OFM_READ;
    break;
  }

  DBGPRINTF("fopen %s (%s | %d)", filename, mode, ofm);

  if(ofm == _OFM_READWRITE) {
    if(access(filename, 0) == SUCCESS) {
      if(IFILEMGR_Remove(pApp->m_pIFileMgr, filename) != SUCCESS)
        goto exit;
    }

    file = IFILEMGR_OpenFile(pApp->m_pIFileMgr, filename, _OFM_CREATE);
    if(file != NULL) {
      IFILE_Release(file);
    } else {
      goto exit;
    }
  }

  file = IFILEMGR_OpenFile(pApp->m_pIFileMgr, filename, ofm);

exit:
  if (file == NULL)
  {
    DBGPRINTF("fopen failed");
  }
  else
  {
    DBGPRINTF("fopen rets 0x%x", file);
  }

  return (FILE *)file;
}

size_t BREW_fread(void *ptr, size_t size, size_t count, FILE *stream)
{
  return IFILE_Read((IFile *)stream, ptr, (size * count));
}

int BREW_fclose(FILE *stream)
{
  return IFILE_Release((IFile *)stream);
}

int BREW_access(const char *pathname, int mode)
{
  return (IFILEMGR_Test(pApp->m_pIFileMgr, pathname) == SUCCESS) ? SUCCESS : -EFAILED;
}

int BREW_fseek(FILE *stream, long int offset, unsigned int origin)
{
  return IFILE_Seek((IFile *)stream, (FileSeekType)origin, offset);
}

void BREW_exit(int status)
{
  DBGPRINTF("Exit with status %d", status);
  PrBoomApp_Exit();
}

int BREW_fstat(IFile *fd, struct stat *buf)
{
  FileInfo pInfo;
  int ret;

  ret = IFILE_GetInfo(fd, &pInfo);
  buf->st_size = pInfo.dwSize;

  return (ret) ? -ret : ret;
}

size_t BREW_fwrite(const void *ptr, size_t size, size_t count, FILE *stream)
{
  return IFILE_Write((IFile *)stream, ptr, (size * count));
}

int BREW_mkdir(const char *_path, mode_t __mode)
{
  return (IFILEMGR_MkDir(pApp->m_pIFileMgr, _path) == SUCCESS) ? SUCCESS : -EFAILED;
}

int BREW_remove(const char *filename) {
  return (IFILEMGR_Remove(pApp->m_pIFileMgr, filename) == SUCCESS) ? SUCCESS : -EFAILED;
}

double BREW_sqrt(double x) { return FSQRT(x); }

// ***************** FIXME STUBBED *****************

char *BREW_strtok(char *str, const char *delimiters) { return NULL; }

int BREW_fprintf(FILE *stream, const char *format, ...) { return -1; }

int BREW_sscanf(const char *s, const char *format, ...) { return -1; }

char *BREW_fgets(char *str, int num, FILE *stream) { return NULL; }

int BREW_fgetc(FILE *stream) { return -1; }

int BREW_fscanf(FILE *stream, const char *format, ...) { return -1; }

long int BREW_ftell(FILE *stream) { return -1L; }

int BREW_feof(FILE *stream) { return -1; }

int BREW_atexit(void (*func)(void)) { return -1; }

char *BREW_strerror(int errnum) { return "[strerror stub]"; }

void BREW___assert_func(const char *a, int b, const char *c, const char *d) { return; }

int BREW_fputc(int c, FILE *stream) { return -1; }
