/*
 * wrgif.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in GIF format.
 *
 **************************************************************************
 * NOTE: to avoid entanglements with Unisys' patent on LZW compression,   *
 * this code has been modified to output "uncompressed GIF" files.        *
 * There is no trace of the LZW algorithm in this file.                   *
 **************************************************************************
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 */

/*
 * This code is loosely based on ppmtogif from the PBMPLUS distribution
 * of Feb. 1991.  That file contains the following copyright notice:
 *    Based on GIFENCODE by David Rowley <mgardi@watdscu.waterloo.edu>.
 *    Lempel-Ziv compression based on "compress" by Spencer W. Thomas et al.
 *    Copyright (C) 1989 by Jef Poskanzer.
 *    Permission to use, copy, modify, and distribute this software and its
 *    documentation for any purpose and without fee is hereby granted, provided
 *    that the above copyright notice appear in all copies and that both that
 *    copyright notice and this permission notice appear in supporting
 *    documentation.  This software is provided "as is" without express or
 *    implied warranty.
 *
 * We are also required to state that
 *    "The Graphics Interchange Format(c) is the Copyright property of
 *    CompuServe Incorporated. GIF(sm) is a Service Mark property of
 *    CompuServe Incorporated."
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef GIF_SUPPORTED


/* Private version of data destination object */

typedef struct {
  struct djpeg_dest_struct pub;	/* public fields */

  j_decompress_ptr cinfo;	/* back link saves passing separate parm */

  /* State for packing variable-width codes into a bitstream */
  int n_bits;			/* current number of bits/code */
  int maxcode;			/* maximum code, given n_bits */
  INT32 cur_accum;		/* holds bits not yet output */
  int cur_bits;			/* # of bits in cur_accum */

  /* State for GIF code assignment */
  int ClearCode;		/* clear code (doesn't change) */
  int EOFCode;			/* EOF code (ditto) */
  int code_counter;		/* counts output symbols */

  /* GIF data packet construction buffer */
  int bytesinpkt;		/* # of bytes in current packet */
  char packetbuf[256];		/* workspace for accumulating packet */

} gif_dest_struct;

typedef gif_dest_struct * gif_dest_ptr;

/* Largest value that will fit in N bits */
#define MAXCODE(n_bits)	((1 << (n_bits)) - 1)


/*
 * Routines to package finished data bytes into GIF data blocks.
 * A data block consists of a count byte (1..255) and that many data bytes.
 */

LOCAL(void)
flush_packet (gif_dest_ptr dinfo)
/* flush any accumulated data */
{
  if (dinfo->bytesinpkt > 0) {	/* never write zero-length packet */
    dinfo->packetbuf[0] = (char) dinfo->bytesinpkt++;
    if (JFWRITE(dinfo->pub.output_file, dinfo->packetbuf, dinfo->bytesinpkt)
	!= (size_t) dinfo->bytesinpkt)
      ERREXIT(dinfo->cinfo, JERR_FILE_WRITE);
    dinfo->bytesinpkt = 0;
  }
}


/* Add a character to current packet; flush to disk if necessary */
#define CHAR_OUT(dinfo,c)  \
	{ (dinfo)->packetbuf[++(dinfo)->bytesinpkt] = (char) (c);  \
	    if ((dinfo)->bytesinpkt >= 255)  \
	      flush_packet(dinfo);  \
	}


/* Routine to convert variable-width codes into a byte stream */

LOCAL(void)
output (gif_dest_ptr dinfo, int code)
/* Emit a code of n_bits bits */
/* Uses cur_accum and cur_bits to reblock into 8-bit bytes */
{
  dinfo->cur_accum |= ((INT32) code) << dinfo->cur_bits;
  dinfo->cur_bits += dinfo->n_bits;

  while (dinfo->cur_bits >= 8) {
    CHAR_OUT(dinfo, dinfo->cur_accum & 0xFF);
    dinfo->cur_accum >>= 8;
    dinfo->cur_bits -= 8;
  }
}


/* The pseudo-compression algorithm.
 *
 * In this module we simply output each pixel value as a separate symbol;
 * thus, no compression occurs.  In fact, there is expansion of one bit per
 * pixel, because we use a symbol width one bit wider than the pixel width.
 *
 * GIF ordinarily uses variable-width symbols, and the decoder will expect
 * to ratchet up the symbol width after a fixed number of symbols.
 * To simplify the logic and keep the expansion penalty down, we emit a
 * GIF Clear code to reset the decoder just before the width would ratchet up.
 * Thus, all the symbols in the output file will have the same bit width.
 * Note that emitting the Clear codes at the right times is a mere matter of
 * counting output symbols and is in no way dependent on the LZW patent.
 *
 * With a small basic pixel width (low color count), Clear codes will be
 * needed very frequently, causing the file to expand even more.  So this
 * simplistic approach wouldn't work too well on bilevel images, for example.
 * But for output of JPEG conversions the pixel width will usually be 8 bits
 * (129 to 256 colors), so the overhead added by Clear symbols is only about
 * one symbol in every 256.
 */

LOCAL(void)
compress_init (gif_dest_ptr dinfo, int i_bits)
/* Initialize pseudo-compressor */
{
  /* init all the state variables */
  dinfo->n_bits = i_bits;
  dinfo->maxcode = MAXCODE(dinfo->n_bits);
  dinfo->ClearCode = (1 << (i_bits - 1));
  dinfo->EOFCode = dinfo->ClearCode + 1;
  dinfo->code_counter = dinfo->ClearCode + 2;
  /* init output buffering vars */
  dinfo->bytesinpkt = 0;
  dinfo->cur_accum = 0;
  dinfo->cur_bits = 0;
  /* GIF specifies an initial Clear code */
  output(dinfo, dinfo->ClearCode);
}


LOCAL(void)
compress_pixel (gif_dest_ptr dinfo, int c)
/* Accept and "compress" one pixel value.
 * The given value must be less than n_bits wide.
 */
{
  /* Output the given pixel value as a symbol. */
  output(dinfo, c);
  /* Issue Clear codes often enough to keep the reader from ratcheting up
   * its symbol size.
   */
  if (dinfo->code_counter < dinfo->maxcode) {
    dinfo->code_counter++;
  } else {
    output(dinfo, dinfo->ClearCode);
    dinfo->code_counter = dinfo->ClearCode + 2;	/* reset the counter */
  }
}


LOCAL(void)
compress_term (gif_dest_ptr dinfo)
/* Clean up at end */
{
  /* Send an EOF code */
  output(dinfo, dinfo->EOFCode);
  /* Flush the bit-packing buffer */
  if (dinfo->cur_bits > 0) {
    CHAR_OUT(dinfo, dinfo->cur_accum & 0xFF);
  }
  /* Flush the packet buffer */
  flush_packet(dinfo);
}


/* GIF header construction */


LOCAL(void)
put_word (gif_dest_ptr dinfo, unsigned int w)
/* Emit a 16-bit word, LSB first */
{
  putc(w & 0xFF, dinfo->pub.output_file);
  putc((w >> 8) & 0xFF, dinfo->pub.output_file);
}


LOCAL(void)
put_3bytes (gif_dest_ptr dinfo, int val)
/* Emit 3 copies of same byte value --- handy subr for colormap construction */
{
  putc(val, dinfo->pub.output_file);
  putc(val, dinfo->pub.output_file);
  putc(val, dinfo->pub.output_file);
}


LOCAL(void)
emit_header (gif_dest_ptr dinfo, int num_colors, JSAMPARRAY colormap)
/* Output the GIF file header, including color map */
/* If colormap==NULL, synthesize a gray-scale colormap */
{
  int BitsPerPixel, ColorMapSize, InitCodeSize, FlagByte;
  int cshift = dinfo->cinfo->data_precision - 8;
  int i;

  if (num_colors > 256)
    ERREXIT1(dinfo->cinfo, JERR_TOO_MANY_COLORS, num_colors);
  /* Compute bits/pixel and related values */
  BitsPerPixel = 1;
  while (num_colors > (1 << BitsPerPixel))
    BitsPerPixel++;
  ColorMapSize = 1 << BitsPerPixel;
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;
  /*
   * Write the GIF header.
   * Note that we generate a plain GIF87 header for maximum compatibility.
   */
  putc('G', dinfo->pub.output_file);
  putc('I', dinfo->pub.output_file);
  putc('F', dinfo->pub.output_file);
  putc('8', dinfo->pub.output_file);
  putc('7', dinfo->pub.output_file);
  putc('a', dinfo->pub.output_file);
  /* Write the Logical Screen Descriptor */
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_width);
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_height);
  FlagByte = 0x80;		/* Yes, there is a global color table */
  FlagByte |= (BitsPerPixel-1) << 4; /* color resolution */
  FlagByte |= (BitsPerPixel-1);	/* size of global color table */
  putc(FlagByte, dinfo->pub.output_file);
  putc(0, dinfo->pub.output_file); /* Background color index */
  putc(0, dinfo->pub.output_file); /* Reserved (aspect ratio in GIF89) */
  /* Write the Global Color Map */
  /* If the color map is more than 8 bits precision, */
  /* we reduce it to 8 bits by shifting */
  for (i=0; i < ColorMapSize; i++) {
    if (i < num_colors) {
      if (colormap != NULL) {
	if (dinfo->cinfo->out_color_space == JCS_RGB) {
	  /* Normal case: RGB color map */
	  putc(GETJSAMPLE(colormap[0][i]) >> cshift, dinfo->pub.output_file);
	  putc(GETJSAMPLE(colormap[1][i]) >> cshift, dinfo->pub.output_file);
	  putc(GETJSAMPLE(colormap[2][i]) >> cshift, dinfo->pub.output_file);
	} else {
	  /* Grayscale "color map": possible if quantizing grayscale image */
	  put_3bytes(dinfo, GETJSAMPLE(colormap[0][i]) >> cshift);
	}
      } else {
	/* Create a gray-scale map of num_colors values, range 0..255 */
	put_3bytes(dinfo, (i * 255 + (num_colors-1)/2) / (num_colors-1));
      }
    } else {
      /* fill out the map to a power of 2 */
      put_3bytes(dinfo, 0);
    }
  }
  /* Write image separator and Image Descriptor */
  putc(',', dinfo->pub.output_file); /* separator */
  put_word(dinfo, 0);		/* left/top offset */
  put_word(dinfo, 0);
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_width); /* image size */
  put_word(dinfo, (unsigned int) dinfo->cinfo->output_height);
  /* flag byte: not interlaced, no local color map */
  putc(0x00, dinfo->pub.output_file);
  /* Write Initial Code Size byte */
  putc(InitCodeSize, dinfo->pub.output_file);

  /* Initialize for "compression" of image data */
  compress_init(dinfo, InitCodeSize+1);
}


/*
 * Startup: write the file header.
 */

METHODDEF(void)
start_output_gif (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  gif_dest_ptr dest = (gif_dest_ptr) dinfo;

  if (cinfo->quantize_colors)
    emit_header(dest, cinfo->actual_number_of_colors, cinfo->colormap);
  else
    emit_header(dest, 256, (JSAMPARRAY) NULL);
}


/*
 * Write some pixel data.
 * In this module rows_supplied will always be 1.
 */

METHODDEF(void)
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		JDIMENSION rows_supplied)
{
  gif_dest_ptr dest = (gif_dest_ptr) dinfo;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  for (col = cinfo->output_width; col > 0; col--) {
    compress_pixel(dest, GETJSAMPLE(*ptr++));
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish_output_gif (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  gif_dest_ptr dest = (gif_dest_ptr) dinfo;

  /* Flush "compression" mechanism */
  compress_term(dest);
  /* Write a zero-length data block to end the series */
  putc(0, dest->pub.output_file);
  /* Write the GIF terminator mark */
  putc(';', dest->pub.output_file);
  /* Make sure we wrote the output file OK */
  fflush(dest->pub.output_file);
  if (ferror(dest->pub.output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
 * The module selection routine for GIF format output.
 */

GLOBAL(djpeg_dest_ptr)
jinit_write_gif (j_decompress_ptr cinfo)
{
  gif_dest_ptr dest;

  /* Create module interface object, fill in method pointers */
  dest = (gif_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(gif_dest_struct));
  dest->cinfo = cinfo;		/* make back link for subroutines */
  dest->pub.start_output = start_output_gif;
  dest->pub.put_pixel_rows = put_pixel_rows;
  dest->pub.finish_output = finish_output_gif;

  if (cinfo->out_color_space != JCS_GRAYSCALE &&
      cinfo->out_color_space != JCS_RGB)
    ERREXIT(cinfo, JERR_GIF_COLORSPACE);

  /* Force quantization if color or if > 8 bits input */
  if (cinfo->out_color_space != JCS_GRAYSCALE || cinfo->data_precision > 8) {
    /* Force quantization to at most 256 colors */
    cinfo->quantize_colors = TRUE;
    if (cinfo->desired_number_of_colors > 256)
      cinfo->desired_number_of_colors = 256;
  }

  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  if (cinfo->output_components != 1) /* safety check: just one component? */
    ERREXIT(cinfo, JERR_GIF_BUG);

  /* Create decompressor output buffer. */
  dest->pub.buffer = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, cinfo->output_width, (JDIMENSION) 1);
  dest->pub.buffer_height = 1;

  return (djpeg_dest_ptr) dest;
}

#endif /* GIF_SUPPORTED */
