/*
  Copyright (c) 2014, Alexey Frunze
  2-clause BSD license.
*/
#include "istdio.h"

int ungetc(int c, FILE* f)
{
  if (c == EOF)
    return EOF;

  // It doesn't make much sense to support ungetc() before
  // at least one fgetc() is done.
  // TBD??? Support it nonetheless???
  if (f->ptr == f->buf)
    return EOF;

  f->ptr--;
  f->cnt++;
  if (*f->ptr != (unsigned char)c)
  {
    *f->ptr = c;
    f->flags |= _IOUNGOT; // fseek() must undo changes by ungetc()
  }
  return (unsigned char)c;
}
