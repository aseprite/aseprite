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

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "jinete/jinete.h"

#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "file/file.h"
#include "raster/raster.h"
#include "script/script.h"

#include "jpeglib.h"

static bool load_JPEG(FileOp *fop);
static bool save_JPEG(FileOp *fop);

static bool configure_jpeg(void); /* TODO warning: not thread safe */

FileFormat format_jpeg =
{
  "jpeg",
  "jpeg,jpg",
  load_JPEG,
  save_JPEG,
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
    return FALSE;

  /* initialize the JPEG decompression object with error handling */
  jerr.fop = fop;
  cinfo.err = jpeg_std_error(&jerr.head);

  jerr.head.error_exit = error_exit;
  jerr.head.output_message = output_message;

  /* establish the setjmp return context for error_exit to use */
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return FALSE;
  }

  jpeg_create_decompress(&cinfo);

  /* specify data source for decompression */
  jpeg_stdio_src(&cinfo, file);

  /* read file header, set default decompression parameters */
  jpeg_read_header(&cinfo, TRUE);

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
    return FALSE;
  }

  /* create the buffer */
  buffer_height = cinfo.rec_outbuf_height;
  buffer = jmalloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return FALSE;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = jmalloc(sizeof(JSAMPLE) *
			cinfo.output_width * cinfo.output_components);
    if (!buffer[c]) {
      for (c--; c>=0; c--)
        jfree(buffer[c]);
      jfree(buffer);
      jpeg_destroy_decompress(&cinfo);
      fclose(file);
      return FALSE;
    }
  }

  /* generate a grayscale palette if is necessary */
  if (image->imgtype == IMAGE_GRAYSCALE)
    for (c=0; c<256; c++)
      fop_sequence_set_color(fop, c, c >> 2, c >> 2, c >> 2);

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
  return TRUE;
}

static bool save_JPEG(FileOp *fop)
{
  struct jpeg_compress_struct cinfo;
  struct error_mgr jerr;
  Image *image = fop->seq.image;
  FILE *file;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  int c;
  int smooth;
  int quality;
  J_DCT_METHOD method;

  /* Configure JPEG compression only in the first frame.  */
  if (fop->sprite->frame == 0 && !configure_jpeg())
    return FALSE;

  /* Options.  */
  smooth = get_config_int("JPEG", "Smooth", 0);
  quality = get_config_int("JPEG", "Quality", 100);
  method = get_config_int("JPEG", "Method", JDCT_DEFAULT);

  /* Open the file for write in it.  */
  file = fopen(fop->filename, "wb");
  if (!file) {
    fop_error(fop, _("Error creating file.\n"));
    return FALSE;
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
  jpeg_set_quality(&cinfo, quality, TRUE);
  cinfo.dct_method = method;
  cinfo.smoothing_factor = smooth;

  /* Start compressor.  */
  jpeg_start_compress(&cinfo, TRUE);

  /* Create the buffer.  */
  buffer_height = 1;
  buffer = jmalloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    fop_error(fop, _("Not enough memory for the buffer.\n"));
    jpeg_destroy_compress(&cinfo);
    fclose(file);
    return FALSE;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = jmalloc(sizeof(JSAMPLE) *
			cinfo.image_width * cinfo.num_components);
    if (!buffer[c]) {
      fop_error(fop, _("Not enough memory for buffer scanlines.\n"));
      for (c--; c>=0; c--)
        jfree(buffer[c]);
      jfree(buffer);
      jpeg_destroy_compress(&cinfo);
      fclose(file);
      return FALSE;
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
  return TRUE;
}

/**
 * Shows the JPEG configuration dialog.
 */
static bool configure_jpeg(void)
{
  JWidget window, box1, box2, box3, box4, box5;
  JWidget label_quality, label_smooth, label_method;
  JWidget slider_quality, slider_smooth, view_method;
  JWidget list_method, button_ok, button_cancel;
  int quality, smooth, method;
  bool ret;

  /* interactive mode */
  if (!is_interactive())
    return TRUE;

  /* configuration parameters */
  quality = get_config_int("JPEG", "Quality", 100);
  smooth = get_config_int("JPEG", "Smooth", 0);
  method = get_config_int("JPEG", "Method", 0);

  /* widgets */
  window = jwindow_new(_("Configure JPEG Compression"));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_VERTICAL + JI_HOMOGENEOUS);
  box4 = jbox_new(JI_VERTICAL + JI_HOMOGENEOUS);
  box5 = jbox_new(JI_HORIZONTAL + JI_HOMOGENEOUS);
  label_quality = jlabel_new(_("Quality:"));
  label_smooth = jlabel_new(_("Smooth:"));
  label_method = jlabel_new(_("Method:"));
  slider_quality = jslider_new(0, 100, quality);
  slider_smooth = jslider_new(0, 100, smooth);
  view_method = jview_new();
  list_method = jlistbox_new();
  button_ok = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  jwidget_add_child(list_method, jlistitem_new(_("Slow")));
  jwidget_add_child(list_method, jlistitem_new(_("Fast")));
  jwidget_add_child(list_method, jlistitem_new(_("Float")));
  jlistbox_select_index(list_method, method);

    jview_attach(view_method, list_method);
  jview_maxsize(view_method);

  jwidget_expansive(box4, TRUE);
  jwidget_expansive(view_method, TRUE);

  jwidget_add_child(box3, label_quality);
  jwidget_add_child(box3, label_smooth);
  jwidget_add_child(box4, slider_quality);
  jwidget_add_child(box4, slider_smooth);
  jwidget_add_child(box2, box3);
  jwidget_add_child(box2, box4);
  jwidget_add_child(box1, box2);
  jwidget_add_child(box1, label_method);
  jwidget_add_child(box1, view_method);
  jwidget_add_child(box5, button_ok);
  jwidget_add_child(box5, button_cancel);
  jwidget_add_child(box1, box5);
  jwidget_add_child(window, box1);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    ret = TRUE;

    quality = jslider_get_value(slider_quality);
    smooth = jslider_get_value(slider_smooth);
    method = jlistbox_get_selected_index(list_method);

    set_config_int("JPEG", "Quality", quality);
    set_config_int("JPEG", "Smooth", smooth);
    set_config_int("JPEG", "Method", method);
  }
  else
    ret = FALSE;

  jwidget_free(window);
  return ret;
}
