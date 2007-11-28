/*
 * wrtarga.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in Targa format.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 *
 * Based on code contributed by Lee Daniel Crocker.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef TARGA_SUPPORTED


/*
 * To support 12-bit JPEG data, we'd have to scale output down to 8 bits.
 * This is not yet implemented.
 */

#if BITS_IN_JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif

/*
 * The output buffer needs to be writable by fwrite().  On PCs, we must
 * allocate the buffer in near data space, because we are assuming small-data
 * memory model, wherein fwrite() can't reach far memory.  If you need to
 * process very wide images on a PC, you might have to compile in large-memory
 * model, or else replace fwrite() with a putc() loop --- which will be much
 * slower.
 */


/* Private version of data destination object */

typedef struct {
  struct djpeg_dest_struct pub;	/* public fields */

  char *iobuffer;		/* physical I/O buffer */
  JDIMENSION buffer_width;	/* width of one row */
} tga_dest_struct;

typedef tga_dest_struct * tga_dest_ptr;


LOCAL(void)
write_header (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo, int num_colors)
/* Create and write a Targa header */
{
  char targaheader[18];

  /* Set unused fields of header to 0 */
  MEMZERO(targaheader, SIZEOF(targaheader));

  if (num_colors > 0) {
    targaheader[1] = 1;		/* color map type 1 */
    targaheader[5] = (char) (num_colors & 0xFF);
    targaheader[6] = (char) (num_colors >> 8);
    targaheader[7] = 24;	/* 24 bits per cmap entry */
  }

  targaheader[12] = (char) (cinfo->output_width & 0xFF);
  targaheader[13] = (char) (cinfo->output_width >> 8);
  targaheader[14] = (char) (cinfo->output_height & 0xFF);
  targaheader[15] = (char) (cinfo->output_height >> 8);
  targaheader[17] = 0x20;	/* Top-down, non-interlaced */

  if (cinfo->out_color_space == JCS_GRAYSCALE) {
    targaheader[2] = 3;		/* image type = uncompressed gray-scale */
    targaheader[16] = 8;	/* bits per pixel */
  } else {			/* must be RGB */
    if (num_colors > 0) {
      targaheader[2] = 1;	/* image type = colormapped RGB */
      targaheader[16] = 8;
    } else {
      targaheader[2] = 2;	/* image type = uncompressed RGB */
      targaheader[16] = 24;
    }
  }

  if (JFWRITE(dinfo->output_file, targaheader, 18) != (size_t) 18)
    ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
 * Write some pixel data.
 * In this module rows_supplied will always be 1.
 */

METHODDEF(void)
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		JDIMENSION rows_supplied)
/* used for unquantized full-color output */
{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  register JSAMPROW inptr;
  register char * outptr;
  register JDIMENSION col;

  inptr = dest->pub.buffer[0];
  outptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    outptr[0] = (char) GETJSAMPLE(inptr[2]); /* RGB to BGR order */
    outptr[1] = (char) GETJSAMPLE(inptr[1]);
    outptr[2] = (char) GETJSAMPLE(inptr[0]);
    inptr += 3, outptr += 3;
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}

METHODDEF(void)
put_gray_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
	       JDIMENSION rows_supplied)
/* used for grayscale OR quantized color output */
{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  register JSAMPROW inptr;
  register char * outptr;
  register JDIMENSION col;

  inptr = dest->pub.buffer[0];
  outptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    *outptr++ = (char) GETJSAMPLE(*inptr++);
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * Write some demapped pixel data when color quantization is in effect.
 * For Targa, this is only applied to grayscale data.
 */

METHODDEF(void)
put_demapped_gray (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		   JDIMENSION rows_supplied)
{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  register JSAMPROW inptr;
  register char * outptr;
  register JSAMPROW color_map0 = cinfo->colormap[0];
  register JDIMENSION col;

  inptr = dest->pub.buffer[0];
  outptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    *outptr++ = (char) GETJSAMPLE(color_map0[GETJSAMPLE(*inptr++)]);
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * Startup: write the file header.
 */

METHODDEF(void)
start_output_tga (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  tga_dest_ptr dest = (tga_dest_ptr) dinfo;
  int num_colors, i;
  FILE *outfile;

  if (cinfo->out_color_space == JCS_GRAYSCALE) {
    /* Targa doesn't have a mapped grayscale format, so we will */
    /* demap quantized gray output.  Never emit a colormap. */
    write_header(cinfo, dinfo, 0);
    if (cinfo->quantize_colors)
      dest->pub.put_pixel_rows = put_demapped_gray;
    else
      dest->pub.put_pixel_rows = put_gray_rows;
  } else if (cinfo->out_color_space == JCS_RGB) {
    if (cinfo->quantize_colors) {
      /* We only support 8-bit colormap indexes, so only 256 colors */
      num_colors = cinfo->actual_number_of_colors;
      if (num_colors > 256)
	ERREXIT1(cinfo, JERR_TOO_MANY_COLORS, num_colors);
      write_header(cinfo, dinfo, num_colors);
      /* Write the colormap.  Note Targa uses BGR byte order */
      outfile = dest->pub.output_file;
      for (i = 0; i < num_colors; i++) {
	putc(GETJSAMPLE(cinfo->colormap[2][i]), outfile);
	putc(GETJSAMPLE(cinfo->colormap[1][i]), outfile);
	putc(GETJSAMPLE(cinfo->colormap[0][i]), outfile);
      }
      dest->pub.put_pixel_rows = put_gray_rows;
    } else {
      write_header(cinfo, dinfo, 0);
      dest->pub.put_pixel_rows = put_pixel_rows;
    }
  } else {
    ERREXIT(cinfo, JERR_TGA_COLORSPACE);
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish_output_tga (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  /* Make sure we wrote the output file OK */
  fflush(dinfo->output_file);
  if (ferror(dinfo->output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
 * The module selection routine for Targa format output.
 */

GLOBAL(djpeg_dest_ptr)
jinit_write_targa (j_decompress_ptr cinfo)
{
  tga_dest_ptr dest;

  /* Create module interface object, fill in method pointers */
  dest = (tga_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(tga_dest_struct));
  dest->pub.start_output = start_output_tga;
  dest->pub.finish_output = finish_output_tga;

  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  /* Create I/O buffer.  Note we make this near on a PC. */
  dest->buffer_width = cinfo->output_width * cinfo->output_components;
  dest->iobuffer = (char *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(size_t) (dest->buffer_width * SIZEOF(char)));

  /* Create decompressor output buffer. */
  dest->pub.buffer = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, dest->buffer_width, (JDIMENSION) 1);
  dest->pub.buffer_height = 1;

  return (djpeg_dest_ptr) dest;
}

#endif /* TARGA_SUPPORTED */
