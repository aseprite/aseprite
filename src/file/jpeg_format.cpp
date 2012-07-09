/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "app.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "base/compiler_specific.h"
#include "base/memory.h"
#include "console.h"
#include "file/file.h"
#include "file/file_format.h"
#include "file/file_handle.h"
#include "file/format_options.h"
#include "ini_file.h"
#include "raster/raster.h"
#include "ui/gui.h"

#include <csetjmp>
#include <cstdio>
#include <cstdlib>

#include "jpeglib.h"

class JpegFormat : public FileFormat
{
  // Data for JPEG files
  class JpegOptions : public FormatOptions
  {
  public:
    float quality;              // 1.0 maximum quality.
  };

  const char* onGetName() const { return "jpeg"; }
  const char* onGetExtensions() const { return "jpeg,jpg"; }
  int onGetFlags() const {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop);
  bool onSave(FileOp* fop);

  SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) OVERRIDE;
};

FileFormat* CreateJpegFormat()
{
  return new JpegFormat;
}

struct error_mgr {
  struct jpeg_error_mgr head;
  jmp_buf setjmp_buffer;
  FileOp* fop;
};

static void error_exit(j_common_ptr cinfo)
{
  // Display the message.
  (*cinfo->err->output_message)(cinfo);

  // Return control to the setjmp point.
  longjmp(((struct error_mgr *)cinfo->err)->setjmp_buffer, 1);
}

static void output_message(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  // Format the message.
  (*cinfo->err->format_message)(cinfo, buffer);

  // Put in the log file if.
  PRINTF("JPEG library: \"%s\"\n", buffer);

  // Leave the message for the application.
  fop_error(((struct error_mgr *)cinfo->err)->fop, "%s\n", buffer);
}

bool JpegFormat::onLoad(FileOp* fop)
{
  struct jpeg_decompress_struct cinfo;
  struct error_mgr jerr;
  Image *image;
  JDIMENSION num_scanlines;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  int c;

  FileHandle file(fop->filename.c_str(), "rb");
  if (!file)
    return false;

  // Initialize the JPEG decompression object with error handling.
  jerr.fop = fop;
  cinfo.err = jpeg_std_error(&jerr.head);

  jerr.head.error_exit = error_exit;
  jerr.head.output_message = output_message;

  // Establish the setjmp return context for error_exit to use.
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  jpeg_create_decompress(&cinfo);

  // Specify data source for decompression.
  jpeg_stdio_src(&cinfo, file);

  // Read file header, set default decompression parameters.
  jpeg_read_header(&cinfo, true);

  if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
    cinfo.out_color_space = JCS_GRAYSCALE;
  else
    cinfo.out_color_space = JCS_RGB;

  // Start decompressor.
  jpeg_start_decompress(&cinfo);

  // Create the image.
  image = fop_sequence_image(fop,
                             (cinfo.out_color_space == JCS_RGB ? IMAGE_RGB:
                                                                 IMAGE_GRAYSCALE),
                             cinfo.output_width,
                             cinfo.output_height);
  if (!image) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  // Create the buffer.
  buffer_height = cinfo.rec_outbuf_height;
  buffer = (JSAMPARRAY)base_malloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = (JSAMPROW)base_malloc(sizeof(JSAMPLE) *
                                      cinfo.output_width * cinfo.output_components);
    if (!buffer[c]) {
      for (c--; c>=0; c--)
        base_free(buffer[c]);
      base_free(buffer);
      jpeg_destroy_decompress(&cinfo);
      return false;
    }
  }

  // Generate a grayscale palette if is necessary.
  if (image->getPixelFormat() == IMAGE_GRAYSCALE)
    for (c=0; c<256; c++)
      fop_sequence_set_color(fop, c, c, c, c);

  // Read each scan line.
  while (cinfo.output_scanline < cinfo.output_height) {
    // TODO
/*     if (plugin_want_close())  */
/*       break; */

    num_scanlines = jpeg_read_scanlines(&cinfo, buffer, buffer_height);

    /* RGB */
    if (image->getPixelFormat() == IMAGE_RGB) {
      uint8_t* src_address;
      uint32_t* dst_address;
      int x, y, r, g, b;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((uint8_t**)buffer)[y];
        dst_address = ((uint32_t**)image->line)[cinfo.output_scanline-1+y];

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
      uint8_t* src_address;
      uint16_t* dst_address;
      int x, y;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((uint8_t**)buffer)[y];
        dst_address = ((uint16_t**)image->line)[cinfo.output_scanline-1+y];

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
    base_free(buffer[c]);
  base_free(buffer);

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return true;
}

bool JpegFormat::onSave(FileOp* fop)
{
  struct jpeg_compress_struct cinfo;
  struct error_mgr jerr;
  Image *image = fop->seq.image;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  SharedPtr<JpegOptions> jpeg_options = fop->seq.format_options;
  int c;

  // Open the file for write in it.
  FileHandle file(fop->filename.c_str(), "wb");
  if (!file) {
    fop_error(fop, "Error creating file.\n");
    return false;
  }

  // Allocate and initialize JPEG compression object.
  jerr.fop = fop;
  cinfo.err = jpeg_std_error(&jerr.head);
  jpeg_create_compress(&cinfo);

  // SPECIFY data destination file.
  jpeg_stdio_dest(&cinfo, file);

  // SET parameters for compression.
  cinfo.image_width = image->w;
  cinfo.image_height = image->h;

  if (image->getPixelFormat() == IMAGE_GRAYSCALE) {
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

  // START compressor.
  jpeg_start_compress(&cinfo, true);

  // CREATE the buffer.
  buffer_height = 1;
  buffer = (JSAMPARRAY)base_malloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    fop_error(fop, "Not enough memory for the buffer.\n");
    jpeg_destroy_compress(&cinfo);
    return false;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = (JSAMPROW)base_malloc(sizeof(JSAMPLE) *
                                      cinfo.image_width * cinfo.num_components);
    if (!buffer[c]) {
      fop_error(fop, "Not enough memory for buffer scanlines.\n");
      for (c--; c>=0; c--)
        base_free(buffer[c]);
      base_free(buffer);
      jpeg_destroy_compress(&cinfo);
      return false;
    }
  }

  // Write each scan line.
  while (cinfo.next_scanline < cinfo.image_height) {
    // RGB
    if (image->getPixelFormat() == IMAGE_RGB) {
      uint32_t* src_address;
      uint8_t* dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = ((uint32_t**)image->line)[cinfo.next_scanline+y];
        dst_address = ((uint8_t**)buffer)[y];
        for (x=0; x<image->w; x++) {
          c = *(src_address++);
          *(dst_address++) = _rgba_getr(c);
          *(dst_address++) = _rgba_getg(c);
          *(dst_address++) = _rgba_getb(c);
        }
      }
    }
    // Grayscale.
    else {
      uint16_t* src_address;
      uint8_t* dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = ((uint16_t**)image->line)[cinfo.next_scanline+y];
        dst_address = ((uint8_t**)buffer)[y];
        for (x=0; x<image->w; x++)
          *(dst_address++) = _graya_getv(*(src_address++));
      }
    }
    jpeg_write_scanlines(&cinfo, buffer, buffer_height);

    fop_progress(fop, (float)(cinfo.next_scanline+1) / (float)(cinfo.image_height));
  }

  // Destroy all data.
  for (c=0; c<(int)buffer_height; c++)
    base_free(buffer[c]);
  base_free(buffer);

  // Finish compression.
  jpeg_finish_compress(&cinfo);

  // Release JPEG compression object.
  jpeg_destroy_compress(&cinfo);

  // All fine.
  return true;
}

// Shows the JPEG configuration dialog.
SharedPtr<FormatOptions> JpegFormat::onGetFormatOptions(FileOp* fop)
{
  SharedPtr<JpegOptions> jpeg_options(new JpegOptions());
  try {
    // Configuration parameters
    jpeg_options->quality = get_config_float("JPEG", "Quality", 1.0f);

    // Interactive mode
    if (!App::instance()->isGui())
      return jpeg_options;

    // Load the window to ask to the user the JPEG options he wants.
    UniquePtr<ui::Window> window(app::load_widget<ui::Window>("jpeg_options.xml", "jpeg_options"));
    ui::Slider* slider_quality = app::find_widget<ui::Slider>(window, "quality");
    ui::Widget* ok = app::find_widget<ui::Widget>(window, "ok");

    slider_quality->setValue(jpeg_options->quality * 10.0f);

    window->openWindowInForeground();

    if (window->get_killer() == ok) {
      jpeg_options->quality = slider_quality->getValue() / 10.0f;
      set_config_float("JPEG", "Quality", jpeg_options->quality);
    }
    else {
      jpeg_options.reset(NULL);
    }

    return jpeg_options;
  }
  catch (base::Exception& e) {
    Console::showException(e);
    return SharedPtr<JpegOptions>(0);
  }
}
