/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <stdio.h>
#include <stdlib.h>

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "file/file.h"
#include "raster/raster.h"
#include "script/script.h"
#include "png.h"

#endif

static Sprite *load_png(const char *filename);
static int save_png(Sprite *sprite);

/* static int configure_png(void); */

FileType filetype_png =
{
  "png",
  "png",
  load_png,
  save_png,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_RGBA |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_GRAYA |
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_SEQUENCES
};

/* static void progress_monitor(j_common_ptr cinfo) */
/* { */
/*   if (cinfo->progress->pass_limit > 1) */
/*     do_progress(100 * */
/* 		(cinfo->progress->pass_counter) / */
/* 		(cinfo->progress->pass_limit-1)); */
/* } */

/* struct error_mgr { */
/*   struct jpeg_error_mgr pub; */
/*   jmp_buf setjmp_buffer; */
/* }; */

/* static void error_exit(j_common_ptr cinfo) */
/* { */
/*   /\* Display the message.  *\/ */
/*   (*cinfo->err->output_message)(cinfo); */

/*   /\* Return control to the setjmp point.  *\/ */
/*   longjmp(((struct error_mgr *)cinfo->err)->setjmp_buffer, 1); */
/* } */

/* static void output_message(j_common_ptr cinfo) */
/* { */
/*   char buffer[JMSG_LENGTH_MAX]; */

/*   /\* Format the message.  *\/ */
/*   (*cinfo->err->format_message)(cinfo, buffer); */

/*   /\* Put in the log file if.  *\/ */
/*   PRINTF("JPEG library: \"%s\"\n", buffer); */

/*   /\* Leave the message for the application.  *\/ */
/*   console_printf("%s\n", buffer); */
/* } */

static void report_png_error(png_structp png_ptr, png_const_charp error)
{
  console_printf("libpng: %s\n", error);
}

static Sprite *load_png(const char *filename)
{
  png_uint_32 width, height, y;
  unsigned int sig_read = 0;
  png_structp png_ptr;
  png_infop info_ptr;
  int bit_depth, color_type, interlace_type;
  int intent, screen_gamma;
  int max_screen_colors = 256;
/*   int pass, number_passes; */
  png_bytep row_pointer;
  Image *image;
  int imgtype;
  FILE *fp;

  fp = fopen(filename, "rb");
  if (!fp) {
    if (!file_sequence_sprite())
      console_printf(_("Error opening file.\n"));
    return NULL;
  }

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also supply the
   * the compiler header file version, so that we know if the application
   * was compiled with a compatible version of the library
   */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
 				   report_png_error, report_png_error);
  if (png_ptr == NULL) {
    console_printf("png_create_read_struct\n");
    fclose(fp);
    return NULL;
  }

  /* Allocate/initialize the memory for image information. */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    console_printf("png_create_info_struct\n");
    fclose(fp);
    png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
    return NULL;
  }

  /* Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    console_printf("Error reading PNG file\n");
    /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    fclose(fp);
    /* If we get here, we had a problem reading the file */
    return NULL;
  }

  /* Set up the input control if you are using standard C streams */
  png_init_io(png_ptr, fp);

  /* If we have already read some of the signature */
  png_set_sig_bytes(png_ptr, sig_read);

  /* The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).
   */
  png_read_info(png_ptr, info_ptr);
  
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	       &interlace_type, int_p_NULL, int_p_NULL);

  
  /* Set up the data transformations you want.  Note that these are all
   * optional.  Only call them if you want/need them.  Many of the
   * transformations only work on specific types of images, and many
   * are mutually exclusive.
   */

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  png_set_strip_16(png_ptr);

  /* Strip alpha bytes from the input data without combining with the
   * background (not recommended).
   */
/*   png_set_strip_alpha(png_ptr); */

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images).
   */
  png_set_packing(png_ptr);

  /* Change the order of packed pixels to least significant bit first
   * (not useful if you are using png_set_packing). */
/*   png_set_packswap(png_ptr); */

  /* Expand paletted colors into true RGB triplets */
/*   if (color_type == PNG_COLOR_TYPE_PALETTE) */
/*     png_set_palette_to_rgb(png_ptr); */

  /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_gray_1_2_4_to_8(png_ptr);

  /* Expand paletted or RGB images with transparency to full alpha channels
   * so the data will be available as RGBA quartets.
   */
/*   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) */
/*     png_set_tRNS_to_alpha(png_ptr); */

  /* Set the background color to draw transparent and alpha images over.
   * It is possible to set the red, green, and blue components directly
   * for paletted images instead of supplying a palette index.  Note that
   * even if the PNG file supplies a background, you are not required to
   * use it - you should use the (solid) application background if it has one.
   */

/*   png_color_16 my_background, *image_background; */

/*   if (png_get_bKGD(png_ptr, info_ptr, &image_background)) */
/*     png_set_background(png_ptr, image_background, */
/* 		       PNG_BACKGROUND_GAMMA_FILE, 1, 1.0); */
/*   else */
/*     png_set_background(png_ptr, &my_background, */
/* 		       PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0); */

  /* Some suggestions as to how to get a screen gamma value */

  /* Note that screen gamma is the display_exponent, which includes
   * the CRT_exponent and any correction for viewing conditions */
/*   screen_gamma = 2.2;  /\* A good guess for a PC monitors in a dimly lit room *\/ */
/*   screen_gamma = 1.7 or 1.0;  /\* A good guess for Mac systems *\/ */
  screen_gamma = 2.2;

  /* Tell libpng to handle the gamma conversion for you.  The final call
   * is a good guess for PC generated images, but it should be configurable
   * by the user at run time by the user.  It is strongly suggested that
   * your application support gamma correction.
   */

/*   if (png_get_sRGB(png_ptr, info_ptr, &intent)) */
/*     png_set_gamma(png_ptr, screen_gamma, 0.45455); */
/*   else { */
/*     double image_gamma; */
/*     if (png_get_gAMA(png_ptr, info_ptr, &image_gamma)) */
/*       png_set_gamma(png_ptr, screen_gamma, image_gamma); */
/*     else */
/*       png_set_gamma(png_ptr, screen_gamma, 0.45455); */
/*   } */

  /* Dither RGB files down to 8 bit palette or reduce palettes
   * to the number of colors available on your screen.
   */
/*   if (color_type & PNG_COLOR_MASK_COLOR) { */
/*     int num_palette; */
/*     png_colorp palette; */

/*     /\* This reduces the image to the application supplied palette *\/ */
/* /\*     if (/\\* we have our own palette *\\/) *\/ */
/* /\*       { *\/ */
/* /\* 	/\\* An array of colors to which the image should be dithered *\\/ *\/ */
/* /\* 	png_color std_color_cube[MAX_SCREEN_COLORS]; *\/ */

/* /\* 	png_set_dither(png_ptr, std_color_cube, MAX_SCREEN_COLORS, *\/ */
/* /\* 		       MAX_SCREEN_COLORS, png_uint_16p_NULL, 0); *\/ */
/* /\*       } *\/ */
/*     /\* This reduces the image to the palette supplied in the file *\/ */
/*     /\* else *\/ */
/*     if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) */
/*       { */
/* 	png_uint_16p histogram = NULL; */

/* 	png_get_hIST(png_ptr, info_ptr, &histogram); */

/* 	png_set_dither(png_ptr, palette, num_palette, */
/* 		       max_screen_colors, histogram, 0); */
/*       } */
/*   } */

  /* invert monochrome files to have 0 as white and 1 as black */
/*   png_set_invert_mono(png_ptr); */

  /* If you want to shift the pixel values from the range [0,255] or
   * [0,65535] to the original [0,7] or [0,31], or whatever range the
   * colors were originally in:
   */
/*   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT)) { */
/*     png_color_8p sig_bit; */

/*     png_get_sBIT(png_ptr, info_ptr, &sig_bit); */
/*     png_set_shift(png_ptr, sig_bit); */
/*   } */

  /* flip the RGB pixels to BGR (or RGBA to BGRA) */
/*   if (color_type & PNG_COLOR_MASK_COLOR) */
/*     png_set_bgr(png_ptr); */

  /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
/*   png_set_swap_alpha(png_ptr); */

  /* swap bytes of 16 bit files to least significant byte first */
/*   png_set_swap(png_ptr); */

  /* Add filler (or alpha) byte (before/after each RGB triplet) */
/*   png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER); */

  /* Turn on interlace handling.  REQUIRED if you are not using
   * png_read_image().  To see how to handle interlacing passes,
   * see the png_read_row() method below:
   */
/*   number_passes = png_set_interlace_handling(png_ptr); */

  /* Optional call to gamma correct and add the background to the palette
   * and update info structure.
   */
  png_read_update_info(png_ptr, info_ptr);

  /* create the output image */
  switch (info_ptr->color_type) {
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
       imgtype = IMAGE_GRAYSCALE;
       break;
    case PNG_COLOR_TYPE_PALETTE:
      imgtype = IMAGE_INDEXED;
       break;
    case PNG_COLOR_TYPE_RGB: 
    case PNG_COLOR_TYPE_RGB_ALPHA:
       imgtype = IMAGE_RGB;
       break;
    default:
      console_printf("Color type not supported\n)");
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      fclose(fp);
      return NULL;
  }
  
  image = file_sequence_image(imgtype, info_ptr->width, info_ptr->height);
  if (!image) {
    console_printf("file_sequence_image %dx%d\n", info_ptr->width, info_ptr->height);
    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    fclose(fp);
    return NULL;
  }
  
  /* Allocate the memory to hold the image using the fields of info_ptr. */

   /* The easiest way to read the image: */
  row_pointer = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
/*   for (pass = 0; pass < number_passes; pass++) { */
    for (y = 0; y < height; y++) {
      png_read_row(png_ptr, row_pointer, png_bytepp_NULL);
       
      if (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
	unsigned char *src_address = row_pointer;
	unsigned long *dst_address = ((unsigned long **)image->line)[y];
	int x, r, g, b, a;

	for (x=0; x<width; x++) {
	  r = *(src_address++);
	  g = *(src_address++);
	  b = *(src_address++);
	  a = *(src_address++);
	  *(dst_address++) = _rgba(r, g, b, a);
	}
      }
      else if (info_ptr->color_type == PNG_COLOR_TYPE_RGB) {
	unsigned char *src_address = row_pointer;
	unsigned long *dst_address = ((unsigned long **)image->line)[y];
	int x, y, r, g, b;

	for (x=0; x<width; x++) {
	  r = *(src_address++);
	  g = *(src_address++);
	  b = *(src_address++);
	  *(dst_address++) = _rgba(r, g, b, 255);
	}
      }
      
      do_progress(100 * y / height);
    }
/*   } */
  png_free(png_ptr, row_pointer);

  /* read rest of file, and get additional chunks in info_ptr */
/*   png_read_end(png_ptr, info_ptr); */

  /* clean up after the read, and free any memory allocated */
  png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

  /* close the file */
  fclose(fp);

  /* return the sprite */
  return file_sequence_sprite();

#if 0
  struct jpeg_decompress_struct cinfo;
  struct error_mgr jerr;
  struct jpeg_progress_mgr progress;
  Image *image;
  JDIMENSION num_scanlines;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  int c;

  /* initialize the JPEG decompression object with error handling */
  cinfo.err = jpeg_std_error(&jerr.pub);

  jerr.pub.error_exit = error_exit;
  jerr.pub.output_message = output_message;

  /* establish the setjmp return context for error_exit to use */
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return NULL;
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
  image = file_sequence_image((cinfo.out_color_space == JCS_RGB ? IMAGE_RGB:
								  IMAGE_GRAYSCALE),
			      cinfo.output_width,
			      cinfo.output_height);
  if (!image) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return NULL;
  }

  /* create the buffer */
  buffer_height = cinfo.rec_outbuf_height;
  buffer = jmalloc(sizeof(JSAMPROW) * buffer_height);
  if (!buffer) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return NULL;
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
      return NULL;
    }
  }

  /* generate a grayscale palette if is necessary */
  if (image->imgtype == IMAGE_GRAYSCALE)
    for (c=0; c<256; c++)
      file_sequence_set_color(c, c >> 2, c >> 2, c >> 2);

  /* for progress bar */
  progress.progress_monitor = progress_monitor;
  cinfo.progress = &progress;

  /* read each scan line */
  while (cinfo.output_scanline < cinfo.output_height) {
/*     if (plugin_want_close())  */
/*       break; */

    num_scanlines = jpeg_read_scanlines(&cinfo, buffer, buffer_height);

    /* RGB */
    if (image->imgtype == IMAGE_RGB) {
      unsigned char *src_address;
      unsigned long *dst_address;
      int x, y, r, g, b;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((unsigned char **)buffer)[y];
        dst_address = ((unsigned long **)image->line)[cinfo.output_scanline-1+y];

        for (x=0; x<image->w; x++) {
          r = *(src_address++);
          g = *(src_address++);
          b = *(src_address++);
          *(dst_address++) = _rgba (r, g, b, 255);
        }
      }
    }
    /* Grayscale */
    else {
      unsigned char *src_address;
      unsigned short *dst_address;
      int x, y;

      for (y=0; y<(int)num_scanlines; y++) {
        src_address = ((unsigned char **)buffer)[y];
        dst_address = ((unsigned short **)image->line)[cinfo.output_scanline-1+y];

        for (x=0; x<image->w; x++)
          *(dst_address++) = _graya(*(src_address++), 255);
      }
    }
  }

  /* destroy all data */
  for (c=0; c<(int)buffer_height; c++)
    jfree(buffer[c]);
  jfree(buffer);

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  fclose(file);
  return file_sequence_sprite();
#endif
}

static int save_png(Sprite *sprite)
{
#if 0
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_progress_mgr progress;
  FILE *file;
  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
  Image *image;
  int c;
  int smooth;
  int quality;
  J_DCT_METHOD method;

  /* Configure JPEG compression only in the first frame.  */
  if (sprite->frpos == 0 && configure_jpeg() < 0)
    return 0;

  /* Options.  */
  smooth = get_config_int("JPEG", "Smooth", 0);
  quality = get_config_int("JPEG", "Quality", 100);
  method = get_config_int("JPEG", "Method", JDCT_DEFAULT);

  /* Open the file for write in it.  */
  file = fopen(sprite->filename, "wb");
  if (!file) {
    console_printf(_("Error creating file.\n"));
    return -1;
  }

  image = file_sequence_image_to_save();

  /* Allocate and initialize JPEG compression object.  */
  cinfo.err = jpeg_std_error(&jerr);
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
    console_printf(_("Not enough memory for the buffer.\n"));
    jpeg_destroy_compress(&cinfo);
    fclose(file);
    return -1;
  }

  for (c=0; c<(int)buffer_height; c++) {
    buffer[c] = jmalloc(sizeof(JSAMPLE) *
			cinfo.image_width * cinfo.num_components);
    if (!buffer[c]) {
      console_printf(_("Not enough memory for buffer scanlines.\n"));
      for (c--; c>=0; c--)
        jfree(buffer[c]);
      jfree(buffer);
      jpeg_destroy_compress(&cinfo);
      fclose(file);
      return -1;
    }
  }

  /* For progress bar.  */
  progress.progress_monitor = progress_monitor;
  cinfo.progress = &progress;

  /* Write each scan line.  */
  while (cinfo.next_scanline < cinfo.image_height) {
    /* RGB */
    if (image->imgtype == IMAGE_RGB) {
      unsigned long *src_address;
      unsigned char *dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = ((unsigned long **)image->line)[cinfo.next_scanline+y];
        dst_address = ((unsigned char **)buffer)[y];
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
      unsigned short *src_address;
      unsigned char *dst_address;
      int x, y;
      for (y=0; y<(int)buffer_height; y++) {
        src_address = ((unsigned short **)image->line)[cinfo.next_scanline+y];
        dst_address = ((unsigned char **)buffer)[y];
        for (x=0; x<image->w; x++)
          *(dst_address++) = _graya_getk(*(src_address++));
      }
    }
    jpeg_write_scanlines(&cinfo, buffer, buffer_height);
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
  return 0;
#else
  return -1;
#endif
}

/* static int configure_jpeg(void) */
/* { */
/*   /\* interactive mode *\/ */
/*   if (is_interactive()) { */
/*     lua_State *L = get_lua_state(); */
/*     int ret; */

/*     /\* call the ConfigureJPEG() script routine, it must return "true" */
/*        to save the image *\/ */
/*     lua_pushstring(L, "ConfigureJPEG"); */
/*     lua_gettable(L, LUA_GLOBALSINDEX); */
/*     do_script_raw(L, 0, 1); */
/*     ret = lua_toboolean(L, -1); */
/*     lua_pop(L, 1); */

/*     if (!ret) */
/*       return -1; */
/*   } */

/*   return 0; */
/* } */
