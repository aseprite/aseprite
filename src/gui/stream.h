// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_STREAM_H_INCLUDED
#define GUI_STREAM_H_INCLUDED

#include "gui/base.h"

#include <stdio.h>

struct jstream
{
  void (*close)(JStream stream);
  bool (*eof)(JStream stream);
  void (*flush)(JStream stream);
  int (*getc)(JStream stream);
  char *(*gets)(JStream stream, char *s, int size);
  int (*putc)(JStream stream, int ch);
  int (*seek)(JStream stream, int offset, int whence);
  int (*tell)(JStream stream);
  /* void (*error)(JStream stream, const char *err); */
};

JStream jstream_new(int size);
JStream jstream_new_for_file(FILE *f);
JStream jstream_new_for_string(const char *string);
void jstream_free(JStream stream);

bool jstream_eof(JStream stream);
void jstream_flush(JStream stream);
int jstream_getc(JStream stream);
char *jstream_gets(JStream stream, char *s, int size);
int jstream_putc(JStream stream, int ch);
/* int jstream_puts(JStream stream, const char *s); */
int jstream_seek(JStream stream, int offset, int whence);
int jstream_tell(JStream stream);

#endif
