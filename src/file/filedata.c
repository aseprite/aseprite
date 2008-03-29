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

#include "file/filedata.h"

FileData *filedata_new(int type, int size)
{
  FileData *filedata;

  assert(size >= sizeof(FileData));

  filedata = jmalloc0(size);
  if (filedata == NULL)
    return NULL;

  filedata->type = type;
  filedata->size = size;

  return filedata;
}

void filedata_free(FileData *filedata)
{
  assert(filedata != NULL);
  jfree(filedata);
}

BmpData *bmpdata_new(void)
{
  BmpData *bmpdata = (BmpData *)filedata_new(FILEDATA_BMP,
					     sizeof(BmpData));

  if (bmpdata == NULL)
    return NULL;

  return bmpdata;
}

JpegData *jpegdata_new(void)
{
  JpegData *jpegdata = (JpegData *)filedata_new(FILEDATA_JPEG,
						sizeof(JpegData));

  if (jpegdata == NULL)
    return NULL;

  return jpegdata;
}
