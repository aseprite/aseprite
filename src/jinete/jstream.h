/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JSTREAM_H_INCLUDED
#define JINETE_JSTREAM_H_INCLUDED

#include "jinete/jbase.h"

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
