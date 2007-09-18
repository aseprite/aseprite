/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "art_misc.h"
#include "art_pixbuf.h"


/**
 * art_pixbuf_new_rgb_dnotify: Create a new RGB #ArtPixBuf with explicit destroy notification.
 * @pixels: A buffer containing the actual pixel data.
 * @width: The width of the pixbuf.
 * @height: The height of the pixbuf.
 * @rowstride: The rowstride of the pixbuf.
 * @dfunc_data: The private data passed to @dfunc.
 * @dfunc: The destroy notification function.
 *
 * Creates a generic data structure for holding a buffer of RGB
 * pixels.  It is possible to think of an #ArtPixBuf as a
 * virtualization over specific pixel buffer formats.
 *
 * @dfunc is called with @dfunc_data and @pixels as arguments when the
 * #ArtPixBuf is destroyed. Using a destroy notification function
 * allows a wide range of memory management disciplines for the pixel
 * memory. A NULL value for @dfunc is also allowed and means that no
 * special action will be taken on destruction.
 *
 * Return value: The newly created #ArtPixBuf.
 **/
ArtPixBuf *
art_pixbuf_new_rgb_dnotify (art_u8 *pixels, int width, int height, int rowstride,
			    void *dfunc_data, ArtDestroyNotify dfunc)
{
  ArtPixBuf *pixbuf;

  pixbuf = art_new (ArtPixBuf, 1);

  pixbuf->format = ART_PIX_RGB;
  pixbuf->n_channels = 3;
  pixbuf->has_alpha = 0;
  pixbuf->bits_per_sample = 8;

  pixbuf->pixels = (art_u8 *) pixels;
  pixbuf->width = width;
  pixbuf->height = height;
  pixbuf->rowstride = rowstride;
  pixbuf->destroy_data = dfunc_data;
  pixbuf->destroy = dfunc;

  return pixbuf;
}

/**
 * art_pixbuf_new_rgba_dnotify: Create a new RGBA #ArtPixBuf with explicit destroy notification.
 * @pixels: A buffer containing the actual pixel data.
 * @width: The width of the pixbuf.
 * @height: The height of the pixbuf.
 * @rowstride: The rowstride of the pixbuf.
 * @dfunc_data: The private data passed to @dfunc.
 * @dfunc: The destroy notification function.
 *
 * Creates a generic data structure for holding a buffer of RGBA
 * pixels.  It is possible to think of an #ArtPixBuf as a
 * virtualization over specific pixel buffer formats.
 *
 * @dfunc is called with @dfunc_data and @pixels as arguments when the
 * #ArtPixBuf is destroyed. Using a destroy notification function
 * allows a wide range of memory management disciplines for the pixel
 * memory. A NULL value for @dfunc is also allowed and means that no
 * special action will be taken on destruction.
 *
 * Return value: The newly created #ArtPixBuf.
 **/
ArtPixBuf *
art_pixbuf_new_rgba_dnotify (art_u8 *pixels, int width, int height, int rowstride,
			     void *dfunc_data, ArtDestroyNotify dfunc)
{
  ArtPixBuf *pixbuf;

  pixbuf = art_new (ArtPixBuf, 1);

  pixbuf->format = ART_PIX_RGB;
  pixbuf->n_channels = 4;
  pixbuf->has_alpha = 1;
  pixbuf->bits_per_sample = 8;

  pixbuf->pixels = (art_u8 *) pixels;
  pixbuf->width = width;
  pixbuf->height = height;
  pixbuf->rowstride = rowstride;
  pixbuf->destroy_data = dfunc_data;
  pixbuf->destroy = dfunc;

  return pixbuf;
}

/**
 * art_pixbuf_new_const_rgb: Create a new RGB #ArtPixBuf with constant pixel data.
 * @pixels: A buffer containing the actual pixel data.
 * @width: The width of the pixbuf.
 * @height: The height of the pixbuf.
 * @rowstride: The rowstride of the pixbuf.
 *
 * Creates a generic data structure for holding a buffer of RGB
 * pixels.  It is possible to think of an #ArtPixBuf as a
 * virtualization over specific pixel buffer formats.
 *
 * No action is taken when the #ArtPixBuf is destroyed. Thus, this
 * function is useful when the pixel data is constant or statically
 * allocated.
 *
 * Return value: The newly created #ArtPixBuf.
 **/
ArtPixBuf *
art_pixbuf_new_const_rgb (const art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgb_dnotify ((art_u8 *) pixels, width, height, rowstride, NULL, NULL);
}

/**
 * art_pixbuf_new_const_rgba: Create a new RGBA #ArtPixBuf with constant pixel data.
 * @pixels: A buffer containing the actual pixel data.
 * @width: The width of the pixbuf.
 * @height: The height of the pixbuf.
 * @rowstride: The rowstride of the pixbuf.
 *
 * Creates a generic data structure for holding a buffer of RGBA
 * pixels.  It is possible to think of an #ArtPixBuf as a
 * virtualization over specific pixel buffer formats.
 *
 * No action is taken when the #ArtPixBuf is destroyed. Thus, this
 * function is suitable when the pixel data is constant or statically
 * allocated.
 *
 * Return value: The newly created #ArtPixBuf.
 **/
ArtPixBuf *
art_pixbuf_new_const_rgba (const art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgba_dnotify ((art_u8 *) pixels, width, height, rowstride, NULL, NULL);
}

static void
art_pixel_destroy (void *func_data, void *data)
{
  art_free (data);
}

/**
 * art_pixbuf_new_rgb: Create a new RGB #ArtPixBuf.
 * @pixels: A buffer containing the actual pixel data.
 * @width: The width of the pixbuf.
 * @height: The height of the pixbuf.
 * @rowstride: The rowstride of the pixbuf.
 *
 * Creates a generic data structure for holding a buffer of RGB
 * pixels.  It is possible to think of an #ArtPixBuf as a
 * virtualization over specific pixel buffer formats.
 *
 * The @pixels buffer is freed with art_free() when the #ArtPixBuf is
 * destroyed. Thus, this function is suitable when the pixel data is
 * allocated with art_alloc().
 *
 * Return value: The newly created #ArtPixBuf.
 **/
ArtPixBuf *
art_pixbuf_new_rgb (art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgb_dnotify (pixels, width, height, rowstride, NULL, art_pixel_destroy);
}

/**
 * art_pixbuf_new_rgba: Create a new RGBA #ArtPixBuf.
 * @pixels: A buffer containing the actual pixel data.
 * @width: The width of the pixbuf.
 * @height: The height of the pixbuf.
 * @rowstride: The rowstride of the pixbuf.
 *
 * Creates a generic data structure for holding a buffer of RGBA
 * pixels.  It is possible to think of an #ArtPixBuf as a
 * virtualization over specific pixel buffer formats.
 *
 * The @pixels buffer is freed with art_free() when the #ArtPixBuf is
 * destroyed. Thus, this function is suitable when the pixel data is
 * allocated with art_alloc().
 *
 * Return value: The newly created #ArtPixBuf.
 **/
ArtPixBuf *
art_pixbuf_new_rgba (art_u8 *pixels, int width, int height, int rowstride)
{
  return art_pixbuf_new_rgba_dnotify (pixels, width, height, rowstride, NULL, art_pixel_destroy);
}

/**
 * art_pixbuf_free: Destroy an #ArtPixBuf.
 * @pixbuf: The #ArtPixBuf to be destroyed.
 *
 * Destroys the #ArtPixBuf, calling the destroy notification function
 * (if non-NULL) so that the memory for the pixel buffer can be
 * properly reclaimed.
 **/
void
art_pixbuf_free (ArtPixBuf *pixbuf)
{
  ArtDestroyNotify destroy = pixbuf->destroy;
  void *destroy_data = pixbuf->destroy_data;
  art_u8 *pixels = pixbuf->pixels;

  pixbuf->pixels = NULL;
  pixbuf->destroy = NULL;
  pixbuf->destroy_data = NULL;

  if (destroy)
    destroy (destroy_data, pixels);

  art_free (pixbuf);
}

/**
 * art_pixbuf_free_shallow:
 * @pixbuf: The #ArtPixBuf to be destroyed.
 *
 * Destroys the #ArtPixBuf without calling the destroy notification function.
 *
 * This function is deprecated. Use the _dnotify variants for
 * allocation instead.
 **/
void
art_pixbuf_free_shallow (ArtPixBuf *pixbuf)
{
  art_free (pixbuf);
}

/**
 * art_pixbuf_duplicate: Duplicate a pixbuf.
 * @pixbuf: The #ArtPixBuf to duplicate.
 *
 * Duplicates a pixbuf, including duplicating the buffer.
 *
 * Return value: The duplicated pixbuf.
 **/
ArtPixBuf *
art_pixbuf_duplicate (const ArtPixBuf *pixbuf)
{
  ArtPixBuf *result;
  int size;

  result = art_new (ArtPixBuf, 1);

  result->format = pixbuf->format;
  result->n_channels = pixbuf->n_channels;
  result->has_alpha = pixbuf->has_alpha;
  result->bits_per_sample = pixbuf->bits_per_sample;

  size = (pixbuf->height - 1) * pixbuf->rowstride +
    pixbuf->width * ((pixbuf->n_channels * pixbuf->bits_per_sample + 7) >> 3);
  result->pixels = art_alloc (size);
  memcpy (result->pixels, pixbuf->pixels, size);

  result->width = pixbuf->width;
  result->height = pixbuf->height;
  result->rowstride = pixbuf->rowstride;
  result->destroy_data = NULL;
  result->destroy = art_pixel_destroy;

  return result;
}
