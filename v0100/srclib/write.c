/*
  Copyright (c) 2014, Alexey Frunze
  2-clause BSD license.
*/
#include <unistd.h>

#ifdef _DOS

#ifdef __HUGE__
static
int DosWrite(int handle, void* buf, unsigned size, unsigned* sizeOrError)
{
  asm("mov ah, 0x40\n"
      "mov bx, [bp + 8]\n"
      "mov edx, [bp + 12]\n"
      "ror edx, 4\n"
      "mov ds, dx\n"
      "shr edx, 28\n"
      "mov cx, [bp + 16]\n"
      "int 0x21");
  asm("movzx ebx, ax\n"
      "cmc\n"
      "sbb ax, ax\n"
      "and eax, 1\n"
      "mov esi, [bp + 20]\n"
      "ror esi, 4\n"
      "mov ds, si\n"
      "shr esi, 28\n"
      "mov [si], ebx");
}
#endif // __HUGE__

#ifdef __SMALLER_C_16__
static
int DosWrite(int handle, void* buf, unsigned size, unsigned* sizeOrError)
{
  asm("mov ah, 0x40\n"
      "mov bx, [bp + 4]\n"
      "mov dx, [bp + 6]\n"
      "mov cx, [bp + 8]\n"
      "int 0x21");
  asm("mov bx, ax\n"
      "cmc\n"
      "sbb ax, ax\n"
      "and ax, 1\n"
      "mov si, [bp + 10]\n"
      "mov [si], bx");
}
#endif // __SMALLER_C_16__

ssize_t write(int fd, void* buf, size_t size)
{
  ssize_t cnt = 0;
  char* p = buf;

  if ((ssize_t)size < 0)
  {
    return -1;
  }

  while (size)
  {
#ifndef __HUGE__
    unsigned sz = size;
#else
    // DOS can read/write at most 65535 bytes at a time.
    // An arbitrary 20-bit physical address can be transformed
    // into a segment:offset pair such that offset is always <= 15
    // (which is what we do under the hood in the huge mode(l))
    // and therefore the size is additionally limited by this
    // offset that can be as high as 15 and DOS will only have
    // the range from this offset (at most 15(0xF)) to 65535(0xFFFF)
    // within a segment. So, cap the size at 0xFFF0.
    unsigned sz = (size > 0xFFF0) ? 0xFFF0 : size;
#endif
    unsigned writtenOrError;
    if (DosWrite(fd, p, sz, &writtenOrError))
    {
      p += writtenOrError;
      cnt += writtenOrError;
      size -= writtenOrError;
      if (writtenOrError < sz)
        return cnt;
    }
    else
    {
      return -1;
    }
  }

  return cnt;
}

#endif // _DOS

#ifdef _WINDOWS

#include "iwin32.h"

ssize_t write(int fd, void* buf, size_t size)
{
  unsigned writtenOrError;

  if ((ssize_t)size < 0)
  {
    return -1;
  }

  if (!size)
    return 0;

  // TBD??? Fix this hack???
  // GetStdHandle(STD_INPUT_HANDLE), GetStdHandle(STD_OUTPUT_HANDLE) and GetStdHandle(STD_ERROR_HANDLE)
  // appear to always return 3, 7 and 11 when there's no redirection. Other handles (e.g. those of files)
  // appear to have values that are multiples of 4. I'm not sure if GetStdHandle() can ever return values
  // 0, 1 and 2 or if any other valid handle can ever be equal to 0, 1 or 2.
  // If 0, 1 and 2 can be valid handles in the system, I'll need to renumber/translate handles in the C library.
  if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
    fd = GetStdHandle(STD_INPUT_HANDLE - fd);

  if (WriteFile(fd, buf, size, &writtenOrError, 0))
  {
    return writtenOrError;
  }
  else
  {
    return -1;
  }
}

#endif // _WINDOWS
