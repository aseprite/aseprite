/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>

#include "jinete/jbase.h"

#include "file/format_options.h"

FormatOptions* format_options_new(int type, int size)
{
  FormatOptions* format_options;

  assert(size >= sizeof(FormatOptions));

  format_options = (FormatOptions*)jmalloc0(size);
  if (format_options == NULL)
    return NULL;

  format_options->type = type;
  format_options->size = size;

  return format_options;
}

void format_options_free(FormatOptions* format_options)
{
  assert(format_options != NULL);
  jfree(format_options);
}

BmpOptions *bmp_options_new()
{
  BmpOptions *bmp_options = (BmpOptions *)
    format_options_new(FORMAT_OPTIONS_BMP,
		       sizeof(BmpOptions));

  if (bmp_options == NULL)
    return NULL;

  return bmp_options;
}

JpegOptions* jpeg_options_new()
{
  JpegOptions *jpeg_options = (JpegOptions *)
    format_options_new(FORMAT_OPTIONS_JPEG,
		       sizeof(JpegOptions));

  if (jpeg_options == NULL)
    return NULL;

  return jpeg_options;
}
