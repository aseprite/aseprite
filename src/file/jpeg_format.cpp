/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "jinete/jinete.h"

#include "app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "file/file.h"
#include "file/format_options.h"
#include "modules/gui.h"
#include "raster/raster.h"

#include "jpeglib.h"

static bool load_JPEG(FileOp *fop);
static bool save_JPEG(FileOp *fop);
static FormatOptions *get_options_JPEG(FileOp *fop);

FileFormat format_jpeg =
{
  "jpeg",
  "jpeg,jpg",
  load_JPEG,
  save_JPEG,
  get_options_JPEG,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_SEQUENCES
};

struct error_mgr {
  struct jpeg_error_mgr head;
  jmp_buf setjmp_buffer;
  FileOp *fop;
};

static void error_exit(j_common_ptr cinfo)
{
  /* Display the message.  */
  (*cinfo->err->output_message)(cinfo);

  /* Return control to the setjmp point.  */
  longjmp(((struct error_mgr *)cinfo->err)->setjmp_buffer, 1);
}

static void output_message(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  /* Format the message.  */
  (*cinfo->err->format_message)(cinfo, buffer);

  /* Put in the log file if.  */
  PRINTF("JPEG library: \"%s\"\n", buffer);

  /* Leave the message for the application.  */
  fop_error(((struct error_mgr *)cinfo->err)->fop, "%s\n", buffer);
}

static bool load_JPEG(FileOp *fop)
{
  struct jpeg_decompress_struct cinfo;
  struct error_mgr jerr;
  Image *image;
  FILE *file;
  JDIMENSION num_scanlines;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  int c;

  file = fopen(fop->filename, "rb");
  if (!file)
    return false;

  /* initialize the JPEG decompression object with error handling */
  jerr.fop = fop;
  cinfo.err = jpeg_std_error(&jerr.head);

  jerr.head.error_exit = error_exit;
  jerr.head.output_message = output_message;

  /* establish the setjmp return context for error_exit to use */
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return false;
  }

  jpeg_create_decompress(&cinfo);

  /* specify data source for decompression */
  jpeg_stdio_src(&cinfo, file);

  /* read file header, set default decompression parameters */
  jpeg_read_header(&cinfo, true);

  if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
    cinfo.out_color_space = JCS_GRAYSCALE;
  else
    cinfo.out_color_space = JCS_RGB;

  /* start decompressor */
  jpeg_start_decompress(&cinfo);

  /* create the image */
  image = fop_sequence_image(fop,
			     (cinfo.out_color_space == JCS_RGB ? IMAGE_RGB:
								 IMAGE_GRAYSCALE),
			     cinfo.output_width,
			     cinfo.output_height);
  if (!image) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return false;
  }

  /* create the buffer */
  buffer_height = cinfo.rec_outbuf_height;
  buffer = (JSAMPARRAY)jmalloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return false;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = (JSAMPROW)jmalloc(sizeof(JSAMPLE) *
				  cinfo.output_width * cinfo.output_components);
    if (!buffer[c]) {
      for (c--; c>=0; c--)
        jfree(buffer[c]);
      jfree(buffer);
      jpeg_destroy_decompress(&cinfo);
      fclose(file);
      return false;
    }
  }

  /* generate a grayscale palette if is necessary */
  if (image->imgtype == IMAGE_GRAYSCALE)
    for (c=0; c<256; c++)
      fop_sequence_set_color(fop, c, c, c, c);

  /* read each scan line */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* TODO */
/*     if (plugin_want_close())  */
/*       break; */

    num_scanlines = jpeg_read_scanlines(&cinfo, buffer, buffer_height);

    /* RGB */
    if (image->imgtype == IMAGE_RGB) {
      ase_uint8 *src_address;
      ase_uint32 *dst_address;
      int x, y, r, g, b;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((ase_uint8 **)buffer)[y];
        dst_address = ((ase_uint32 **)image->line)[cinfo.output_scanline-1+y];

        for (x=0; x<image->w; x++) {
          r = *(src_address++);
          g = *(src_address++);
          b = *(src_address++);
          *(dst_address++) = _rgba(r, g, b, 255);
        }
      }
    }
    /* Grayscale */
    else {
      ase_uint8 *src_address;
      ase_uint16 *dst_address;
      int x, y;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((ase_uint8 **)buffer)[y];
        dst_address = ((ase_uint16 **)image->line)[cinfo.output_scanline-1+y];

        for (x=0; x<image->w; x++)
          *(dst_address++) = _graya(*(src_address++), 255);
      }
    }

    fop_progress(fop, (float)(cinfo.output_scanline+1) / (float)(cinfo.output_height));
    if (fop_is_stop(fop))
      break;
  }

  /* destroy all data */
  for (c=0; c<(int)buffer_height; c++)
    jfree(buffer[c]);
  jfree(buffer);

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(file);
  return true;
}

static bool save_JPEG(FileOp *fop)
{
  struct jpeg_compress_struct cinfo;
  struct error_mgr jerr;
  Image *image = fop->seq.image;
  FILE *file;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  JpegOptions *jpeg_options = (JpegOptions *)fop->seq.format_options;
  int c;

  /* Open the file for write in it.  */
  file = fopen(fop->filename, "wb");
  if (!file) {
    fop_error(fop, _("Error creating file.\n"));
    return false;
  }

  /* Allocate and initialize JPEG compression object.  */
  jerr.fop = fop;
  cinfo.err = jpeg_std_error(&jerr.head);
  jpeg_create_compress(&cinfo);

  /* Specify data destination file.  */
  jpeg_stdio_dest(&cinfo, file);

  /* Set parameters for compression.  */
  cinfo.image_width = image->w;
  cinfo.image_height = image->h;

  if (image->imgtype == IMAGE_GRAYSCALE) {
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
  }
  else {
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
  }

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, (int)MID(0, 100.0f * jpeg_options->quality, 100), true);
  cinfo.dct_method = JDCT_ISLOW;
  cinfo.smoothing_factor = 0;

  /* Start compressor.  */
  jpeg_start_compress(&cinfo, true);

  /* Create the buffer.  */
  buffer_height = 1;
  buffer = (JSAMPARRAY)jmalloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    fop_error(fop, _("Not enough memory for the buffer.\n"));
    jpeg_destroy_compress(&cinfo);
    fclose(file);
    return false;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = (JSAMPROW)jmalloc(sizeof(JSAMPLE) *
				  cinfo.image_width * cinfo.num_components);
    if (!buffer[c]) {
      fop_error(fop, _("Not enough memory for buffer scanlines.\n"));
      for (c--; c>=0; c--)
        jfree(buffer[c]);
      jfree(buffer);
      jpeg_destroy_compress(&cinfo);
      fclose(file);
      return false;
    }
  }

  /* Write each scan line.  */
  while (cinfo.next_scanline < cinfo.image_height) {
    /* RGB */
    if (image->imgtype == IMAGE_RGB) {
      ase_uint32 *src_address;
      ase_uint8 *dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = ((ase_uint32 **)image->line)[cinfo.next_scanline+y];
        dst_address = ((ase_uint8 **)buffer)[y];
        for (x=0; x<image->w; x++) {
          c = *(src_address++);
          *(dst_address++) = _rgba_getr(c);
          *(dst_address++) = _rgba_getg(c);
          *(dst_address++) = _rgba_getb(c);
        }
      }
    }
    /* Grayscale */
    else {
      ase_uint16 *src_address;
      ase_uint8 *dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = ((ase_uint16 **)image->line)[cinfo.next_scanline+y];
        dst_address = ((ase_uint8 **)buffer)[y];
        for (x=0; x<image->w; x++)
          *(dst_address++) = _graya_getv(*(src_address++));
      }
    }
    jpeg_write_scanlines(&cinfo, buffer, buffer_height);
    
    fop_progress(fop, (float)(cinfo.next_scanline+1) / (float)(cinfo.image_height));
  }

  /* Destroy all data.  */
  for (c=0; c<(int)buffer_height; c++)
    jfree(buffer[c]);
  jfree(buffer);

  /* Finish compression.  */
  jpeg_finish_compress(&cinfo);

  /* Release JPEG compression object.  */
  jpeg_destroy_compress(&cinfo);

  /* We can close the output file.  */
  fclose(file);

  /* All fine.  */
  return true;
}

/**
 * Shows the JPEG configuration dialog.
 */
static FormatOptions *get_options_JPEG(FileOp *fop)
{
  JpegOptions *jpeg_options = jpeg_options_new();
  try {
    /* configuration parameters */
    jpeg_options->quality = get_config_float("JPEG", "Quality", 1.0f);

    /* interactive mode */
    if (!is_interactive())
      return (FormatOptions*)jpeg_options;

    /* widgets */
    FramePtr window(load_widget("jpeg_options.xml", "jpeg_options"));
    JWidget slider_quality, ok;
    get_widgets(window,
		"quality", &slider_quality,
		"ok", &ok, NULL);

    jslider_set_value(slider_quality, jpeg_options->quality * 10.0f);

    window->open_window_fg();

    if (window->get_killer() == ok) {
      jpeg_options->quality = jslider_get_value(slider_quality) / 10.0f;
      set_config_float("JPEG", "Quality", jpeg_options->quality);
    }
    else {
      format_options_free((FormatOptions *)jpeg_options);
      jpeg_options = NULL;
    }

    return (FormatOptions *)jpeg_options;
  }
  catch (ase_exception& e) {
    format_options_free((FormatOptions *)jpeg_options);

    e.show();
    return NULL;
  }
}
