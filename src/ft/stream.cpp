// Aseprite FreeType Wrapper
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "ft/stream.h"

#include "base/file_handle.h"

#include <ft2build.h>

#define STREAM_FILE(stream) ((FILE*)stream->descriptor.pointer)

namespace ft {

static void ft_stream_close(FT_Stream stream)
{
  fclose(STREAM_FILE(stream));
  free(stream);
}

static unsigned long ft_stream_io(FT_Stream stream,
                                  unsigned long offset,
                                  unsigned char* buffer,
                                  unsigned long count)
{
  if (!count && offset > stream->size)
    return 1;

  FILE* file = STREAM_FILE(stream);
  if (stream->pos != offset)
    fseek(file, (long)offset, SEEK_SET);

  return (unsigned long)fread(buffer, 1, count, file);
}

FT_Stream open_stream(const std::string& utf8Filename)
{

  FT_Stream stream = nullptr;
  stream = (FT_Stream)malloc(sizeof(*stream));
  memset(stream, 0, sizeof(*stream));
  if(stream == NULL)
    return nullptr;

  TRACE("FT: Loading font %s... ", utf8Filename.c_str());

  FILE* file = base::open_file_raw(utf8Filename, "rb");
  if (!file) {
    free(stream);
    TRACE("FAIL\n");
    return nullptr;
  }

  fseek(file, 0, SEEK_END);
  stream->size = (unsigned long)ftell(file);
  if (!stream->size) {
    fclose(file);
    free(stream);
    TRACE("FAIL\n");
    return nullptr;
  }
  fseek(file, 0, SEEK_SET);

  stream->descriptor.pointer = file;
  stream->base = nullptr;
  stream->pos = 0;
  stream->read = ft_stream_io;
  stream->close = ft_stream_close;

  TRACE("OK\n");
  return stream;
}

} // namespace ft
