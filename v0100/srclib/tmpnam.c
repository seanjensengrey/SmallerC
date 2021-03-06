/*
  Copyright (c) 2014, Alexey Frunze
  2-clause BSD license.
*/
#include "istdio.h"
#include <string.h>
#include <stdlib.h>

#ifdef _DOS

#ifdef __HUGE__
static
int DosQueryAttr(char* name, unsigned* attrOrError)
{
  asm("mov ax, 0x4300\n"
      "mov edx, [bp + 8]\n"
      "ror edx, 4\n"
      "mov ds, dx\n"
      "shr edx, 28\n"
      "int 0x21");
  asm("movzx ebx, ax\n"
      "cmc\n"
      "sbb ax, ax");
  asm("and cx, ax\n"
      "mov dx, ax\n"
      "not dx\n"
      "and bx, dx\n"
      "or  bx, cx");
  asm("and eax, 1\n"
      "mov esi, [bp + 12]\n"
      "ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov [si], ebx");
}
#endif // __HUGE__

#ifdef __SMALLER_C_16__
static
int DosQueryAttr(char* name, unsigned* attrOrError)
{
  asm("mov ax, 0x4300\n"
      "mov dx, [bp + 4]\n"
      "int 0x21");
  asm("mov bx, ax\n"
      "cmc\n"
      "sbb ax, ax");
  asm("and cx, ax\n"
      "mov dx, ax\n"
      "not dx\n"
      "and bx, dx\n"
      "or  bx, cx");
  asm("and ax, 1\n"
      "mov si, [bp + 6]\n"
      "mov [si], bx");
}
#endif // __SMALLER_C_16__

#endif // _DOS

#ifdef _WINDOWS

#include "iwin32.h"

#endif // _WINDOWS

static unsigned num;

static char name[L_tmpnam];
static size_t plen;

static
void TryPath(char* path)
{
  unsigned attrOrError;

  if ((plen = strlen(path)) >= L_tmpnam - (1/*slash*/ + 8+1+3/*8.3 name*/))
    return;

  strcpy(name, path);
  if (name[plen - 1] != '\\' && name[plen - 1] != '/' && name[plen - 1] != ':')
    strcat(name, "\\"), plen++;

  // Check if name exists in the file system and is a directory
#ifdef _DOS
  if (!DosQueryAttr(name, &attrOrError) || !(attrOrError & 0x10))
    *name = '\0';
#endif
#ifdef _WINDOWS
  attrOrError = GetFileAttributesA(name);
  if (attrOrError == INVALID_FILE_ATTRIBUTES || !(attrOrError & FILE_ATTRIBUTE_DIRECTORY))
    *name = '\0';
#endif
}

static
void TryEnvPath(char* evname)
{
  char* path;
  char* ev = __EnvVar;
  __EnvVar = NULL;
  path = getenv(evname);
  __EnvVar = ev; // restore the string previously returned by getenv()

  if (path)
  {
    if (*path)
      TryPath(path);
    free(path);
  }
}

char* __tmpnam(char* buf, unsigned iterations)
{
  unsigned attrOrError;
  unsigned i;

  if (!*name)
  {
    // On the first use, determine the directory for temporary files
    TryEnvPath("TEMP");
    if (!*name)
      TryEnvPath("TMP");
    // If neither %TEMP% nor %TMP% is set, try to use the first hard disk
    if (!*name)
      TryPath("C:\\");
    // Otherwise stick to the current directory
    if (!*name)
    {
      strcpy(name, ".\\");
      plen = 2;
    }
    strcat(name, "TMP00000.$$$");
  }

  if (buf)
    strcpy(buf, name);
  else
    buf = name;

  for (i = 0; i < iterations; i++)
  {
    char* p = buf + plen + 8/*strlen("TMP00000")*/;
    unsigned j = 5/*strlen("00000")*/;

    unsigned n = num++;
    if (num >= TMP_MAX)
      num = 0;

    while (j--)
    {
      *--p = '0' + n % 10;
      n /= 10;
    }

    // Check if buf does not exist in the file system
#ifdef _DOS
    if (!DosQueryAttr(buf, &attrOrError))
    {
      if (attrOrError == 2)
        return buf;
      break;
    }
#endif
#ifdef _WINDOWS
    attrOrError = GetFileAttributesA(buf);
    if (attrOrError == INVALID_FILE_ATTRIBUTES)
    {
      if (GetLastError() == ERROR_FILE_NOT_FOUND)
        return buf;
      break;
    }
#endif
  }

  return NULL;
}

char* tmpnam(char* buf)
{
  return __tmpnam(buf, TMP_MAX);
}
