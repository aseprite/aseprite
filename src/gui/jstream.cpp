// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

/* jstream_string is based on streams of HTMLEX */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base/memory.h"
#include "gui/jinete.h"

/* jinete stream file/string */
#define JSF		((struct jstream_file *)(stream))
#define JSS		((struct jstream_string *)(stream))

/* size of chunks to add to the buffer for jstream_strings */
#define BLOCKSIZE	1024

struct jstream_file
{
  struct jstream head;
  FILE *file;
};

struct jstream_string
{
  struct jstream head;
  char *buf;			/* buffer for the characters */
  int size;			/* real size of the buffer */
  int end;			/* index of the last character used in the buffer */
  int pos;			/* the position for read data */
};

static void stream_file_close(JStream stream);
static bool stream_file_eof(JStream stream);
static void stream_file_flush(JStream stream);
static int stream_file_getc(JStream stream);
static char *stream_file_gets(JStream stream, char *s, int size);
static int stream_file_putc(JStream stream, int ch);
static int stream_file_seek(JStream stream, int offset, int whence);
static int stream_file_tell(JStream stream);

static void stream_string_close(JStream stream);
static bool stream_string_eof(JStream stream);
static void stream_string_flush(JStream stream);
static int stream_string_getc(JStream stream);
static char *stream_string_gets(JStream stream, char *s, int size);
static int stream_string_putc(JStream stream, int ch);
static int stream_string_seek(JStream stream, int offset, int whence);
static int stream_string_tell(JStream stream);

JStream jstream_new(int size)
{
  JStream stream = (JStream)base_malloc0(MAX(size, (int)sizeof(struct jstream)));
  if (!stream)
    return NULL;

  return stream;
}

JStream jstream_new_for_file(FILE *f)
{
  JStream stream = jstream_new(sizeof(struct jstream_file));
  if (!stream)
    return NULL;

  stream->close = stream_file_close;
  stream->eof = stream_file_eof;
  stream->flush = stream_file_flush;
  stream->getc = stream_file_getc;
  stream->gets = stream_file_gets;
  stream->putc = stream_file_putc;
  stream->seek = stream_file_seek;
  stream->tell = stream_file_tell;

  JSF->file = f;

  return stream;
}

JStream jstream_new_for_string(const char *buffer)
{
  JStream stream = jstream_new(sizeof(struct jstream_string));
  if (!stream)
    return NULL;

  stream->close = stream_string_close;
  stream->eof = stream_string_eof;
  stream->flush = stream_string_flush;
  stream->getc = stream_string_getc;
  stream->gets = stream_string_gets;
  stream->putc = stream_string_putc;
  stream->seek = stream_string_seek;
  stream->tell = stream_string_tell;

  JSS->buf = buffer ? base_strdup(buffer): NULL;
  if (buffer && !JSS->buf) {
    base_free(stream);
    return NULL;
  }

  JSS->end = buffer ? strlen(buffer): 0;
  JSS->size = buffer ? ((JSS->end / BLOCKSIZE) +
			((JSS->end % BLOCKSIZE) > 0 ? 1: 0)): 0;
  JSS->pos = 0;

  return stream;
}

void jstream_free(JStream stream)
{
  if (stream->close)
    (*stream->close)(stream);

  base_free(stream);
}

bool jstream_eof(JStream stream)
{
  if (stream->eof)
    return (*stream->eof)(stream);
  else
    return true;
}

void jstream_flush(JStream stream)
{
  if (stream->flush)
    (*stream->flush)(stream);
}

int jstream_getc(JStream stream)
{
  if (stream->getc)
    return (*stream->getc)(stream);
  else
    return EOF;
}

char *jstream_gets(JStream stream, char *s, int size)
{
  if (stream->gets)
    return (*stream->gets)(stream, s, size);
  else
    return NULL;
}

int jstream_putc(JStream stream, int ch)
{
  if (stream->putc)
    return (*stream->putc)(stream, ch);
  else
    return EOF;
}

/* int jstream_puts(JStream stream, const char *s) */
/* { */
/*   return 0; */
/* } */

int jstream_seek(JStream stream, int offset, int whence)
{
  if (stream->seek)
    return (*stream->seek)(stream, offset, whence);
  else
    return EOF;
}

int jstream_tell(JStream stream)
{
  if (stream->tell)
    return (*stream->tell)(stream);
  else
    return EOF;
}

/**********************************************************************/
/* jstream_file */

static void stream_file_close(JStream stream)
{
  FILE *f = JSF->file;
  if ((f != stdin) && (f != stdout) && (f != stderr)) {
    /* you must to close the file, here we can't do:
         fclose(f);
     */
  }
}

static bool stream_file_eof(JStream stream)
{
  return feof(JSF->file) != 0;
}

static void stream_file_flush(JStream stream)
{
  fflush(JSF->file);
}

static int stream_file_getc(JStream stream)
{
  return fgetc(JSF->file);
}

static char *stream_file_gets(JStream stream, char *s, int size)
{
  return fgets(s, size, JSF->file);
}

static int stream_file_putc(JStream stream, int ch)
{
  return fputc(ch, JSF->file);
}

static int stream_file_seek(JStream stream, int offset, int whence)
{
  return fseek(JSF->file, offset, whence);
}

static int stream_file_tell(JStream stream)
{
  return ftell(JSF->file);
}

/**********************************************************************/
/* jstream_string */

static void stream_string_close(JStream stream)
{
  if (JSS->buf)
    base_free(JSS->buf);
}

static bool stream_string_eof(JStream stream)
{
  return (JSS->pos == JSS->end);
}

static void stream_string_flush(JStream stream)
{
  /* do nothing */
}

static int stream_string_getc(JStream stream)
{
  if (JSS->pos < JSS->end)
    return JSS->buf[JSS->pos++];
  else
    return EOF;
}

static char *stream_string_gets(JStream stream, char *s, int size)
{
  if (JSS->pos == JSS->end)
    return NULL;
  else {
    char *r = s;
    int c, i;
    for (i = 0; i < size; ++i) {
      if (JSS->pos < JSS->end) {
	c = JSS->buf[JSS->pos++];
	*(s++) = c;
	if (c == '\n')
	  break;
      }
      else
	break;
    }
    *s = 0;
    return r;
  }
}

static int stream_string_putc(JStream stream, int ch)
{
  if (JSS->end >= JSS->size) {
    JSS->size += BLOCKSIZE;
    JSS->buf = (char*)base_realloc(JSS->buf, JSS->size);
    if (!JSS->buf) {
      JSS->size = 0;
      JSS->end = 0;
      JSS->pos = 0;
      return EOF;
    }
  }

  if (JSS->pos < JSS->end)
    memmove(JSS->buf + JSS->pos + 1,
	    JSS->buf + JSS->pos,
	    JSS->end - JSS->pos);

  JSS->end++;
  return JSS->buf[JSS->pos++] = ch;
}

static int stream_string_seek(JStream stream, int offset, int whence)
{
  switch (whence) {
    case SEEK_SET:
      JSS->pos = offset;
      break;
    case SEEK_CUR:
      JSS->pos += offset;
      break;
    case SEEK_END:
      JSS->pos = JSS->end + offset;
      break;
    default:
      return -1;
  }
  if (JSS->pos < 0)
    JSS->pos = 0;
  else if (JSS->pos > JSS->end)
    JSS->pos = JSS->end;
  return 0;
}

static int stream_string_tell(JStream stream)
{
  return JSS->pos;
}
