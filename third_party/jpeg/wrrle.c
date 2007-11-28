/*
 * wrrle.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in RLE format.
 * The Utah Raster Toolkit library is required (version 3.1 or later).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 *
 * Based on code contributed by Mike Lijewski,
 * with updates from Robert Hutchinson.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef RLE_SUPPORTED

/* rle.h is provided by the Utah Raster Toolkit. */

#include <rle.h>

/*
 * We assume that JSAMPLE has the same representation as rle_pixel,
 * to wit, "unsigned char".  Hence we can't cope with 12- or 16-bit samples.
 */

#if BITS_IN_JSAMPLE != 8
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif


/*
 * Since RLE stores scanlines bottom-to-top, we have to invert the image
 * from JPEG's top-to-bottom order.  To do this, we save the outgoing data
 * in a virtual array during put_pixel_row calls, then actually emit the
 * RLE file during finish_output.
 */


/*
 * For now, if we emit an RLE color map then it is always 256 entries long,
 * though not all of the entries need be used.
 */

#define CMAPBITS	8
#define CMAPLENGTH	(1<<(CMAPBITS))

typedef struct {
  struct djpeg_dest_struct pub; /* public fields */

  jvirt_sarray_ptr image;	/* virtual array to store the output image */
  rle_map *colormap;	 	/* RLE-style color map, or NULL if none */
  rle_pixel **rle_row;		/* To pass rows to rle_putrow() */

} rle_dest_struct;

typedef rle_dest_struct * rle_dest_ptr;

/* Forward declarations */
METHODDEF(void) rle_put_pixel_rows
    JPP((j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
	 JDIMENSION rows_supplied));


/*
 * Write the file header.
 *
 * In this module it's easier to wait till finish_output to write anything.
 */

METHODDEF(void)
start_output_rle (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  rle_dest_ptr dest = (rle_dest_ptr) dinfo;
  size_t cmapsize;
  int i, ci;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  /*
   * Make sure the image can be stored in RLE format.
   *
   * - RLE stores image dimensions as *signed* 16 bit integers.  JPEG
   *   uses unsigned, so we have to check the width.
   *
   * - Colorspace is expected to be grayscale or RGB.
   *
   * - The number of channels (components) is expected to be 1 (grayscale/
   *   pseudocolor) or 3 (truecolor/directcolor).
   *   (could be 2 or 4 if using an alpha channel, but we aren't)
   */

  if (cinfo->output_width > 32767 || cinfo->output_height > 32767)
    ERREXIT2(cinfo, JERR_RLE_DIMENSIONS, cinfo->output_width, 
	     cinfo->output_height);

  if (cinfo->out_color_space != JCS_GRAYSCALE &&
      cinfo->out_color_space != JCS_RGB)
    ERREXIT(cinfo, JERR_RLE_COLORSPACE);

  if (cinfo->output_components != 1 && cinfo->output_components != 3)
    ERREXIT1(cinfo, JERR_RLE_TOOMANYCHANNELS, cinfo->num_components);

  /* Convert colormap, if any, to RLE format. */

  dest->colormap = NULL;

  if (cinfo->quantize_colors) {
    /* Allocate storage for RLE-style cmap, zero any extra entries */
    cmapsize = cinfo->out_color_components * CMAPLENGTH * SIZEOF(rle_map);
    dest->colormap = (rle_map *) (*cinfo->mem->alloc_small)
      ((j_common_ptr) cinfo, JPOOL_IMAGE, cmapsize);
    MEMZERO(dest->colormap, cmapsize);

    /* Save away data in RLE format --- note 8-bit left shift! */
    /* Shifting would need adjustment for JSAMPLEs wider than 8 bits. */
    for (ci = 0; ci < cinfo->out_color_components; ci++) {
      for (i = 0; i < cinfo->actual_number_of_colors; i++) {
        dest->colormap[ci * CMAPLENGTH + i] =
          GETJSAMPLE(cinfo->colormap[ci][i]) << 8;
      }
    }
  }

  /* Set the output buffer to the first row */
  dest->pub.buffer = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, dest->image, (JDIMENSION) 0, (JDIMENSION) 1, TRUE);
  dest->pub.buffer_height = 1;

  dest->pub.put_pixel_rows = rle_put_pixel_rows;

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    progress->total_extra_passes++;  /* count file writing as separate pass */
  }
#endif
}


/*
 * Write some pixel data.
 *
 * This routine just saves the data away in a virtual array.
 */

METHODDEF(void)
rle_put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		    JDIMENSION rows_supplied)
{
  rle_dest_ptr dest = (rle_dest_ptr) dinfo;

  if (cinfo->output_scanline < cinfo->output_height) {
    dest->pub.buffer = (*cinfo->mem->access_virt_sarray)
      ((j_common_ptr) cinfo, dest->image,
       cinfo->output_scanline, (JDIMENSION) 1, TRUE);
  }
}

/*
 * Finish up at the end of the file.
 *
 * Here is where we really output the RLE file.
 */

METHODDEF(void)
finish_output_rle (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  rle_dest_ptr dest = (rle_dest_ptr) dinfo;
  rle_hdr header;		/* Output file information */
  rle_pixel **rle_row, *red, *green, *blue;
  JSAMPROW output_row;
  char cmapcomment[80];
  int row, col;
  int ci;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  /* Initialize the header info */
  header = *rle_hdr_init(NULL);
  header.rle_file = dest->pub.output_file;
  header.xmin     = 0;
  header.xmax     = cinfo->output_width  - 1;
  header.ymin     = 0;
  header.ymax     = cinfo->output_height - 1;
  header.alpha    = 0;
  header.ncolors  = cinfo->output_components;
  for (ci = 0; ci < cinfo->output_components; ci++) {
    RLE_SET_BIT(header, ci);
  }
  if (cinfo->quantize_colors) {
    header.ncmap   = cinfo->out_color_components;
    header.cmaplen = CMAPBITS;
    header.cmap    = dest->colormap;
    /* Add a comment to the output image with the true colormap length. */
    sprintf(cmapcomment, "color_map_length=%d", cinfo->actual_number_of_colors);
    rle_putcom(cmapcomment, &header);
  }

  /* Emit the RLE header and color map (if any) */
  rle_put_setup(&header);

  /* Now output the RLE data from our virtual array.
   * We assume here that (a) rle_pixel is represented the same as JSAMPLE,
   * and (b) we are not on a machine where FAR pointers differ from regular.
   */

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    progress->pub.pass_limit = cinfo->output_height;
    progress->pub.pass_counter = 0;
    (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
  }
#endif

  if (cinfo->output_components == 1) {
    for (row = cinfo->output_height-1; row >= 0; row--) {
      rle_row = (rle_pixel **) (*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, dest->image,
	 (JDIMENSION) row, (JDIMENSION) 1, FALSE);
      rle_putrow(rle_row, (int) cinfo->output_width, &header);
#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
  } else {
    for (row = cinfo->output_height-1; row >= 0; row--) {
      rle_row = (rle_pixel **) dest->rle_row;
      output_row = * (*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, dest->image,
	 (JDIMENSION) row, (JDIMENSION) 1, FALSE);
      red = rle_row[0];
      green = rle_row[1];
      blue = rle_row[2];
      for (col = cinfo->output_width; col > 0; col--) {
        *red++ = GETJSAMPLE(*output_row++);
        *green++ = GETJSAMPLE(*output_row++);
        *blue++ = GETJSAMPLE(*output_row++);
      }
      rle_putrow(rle_row, (int) cinfo->output_width, &header);
#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
  }

#ifdef PROGRESS_REPORT
  if (progress != NULL)
    progress->completed_extra_passes++;
#endif

  /* Emit file trailer */
  rle_puteof(&header);
  fflush(dest->pub.output_file);
  if (ferror(dest->pub.output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
 * The module selection routine for RLE format output.
 */

GLOBAL(djpeg_dest_ptr)
jinit_write_rle (j_decompress_ptr cinfo)
{
  rle_dest_ptr dest;

  /* Create module interface object, fill in method pointers */
  dest = (rle_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  SIZEOF(rle_dest_struct));
  dest->pub.start_output = start_output_rle;
  dest->pub.finish_output = finish_output_rle;

  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  /* Allocate a work array for output to the RLE library. */
  dest->rle_row = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     cinfo->output_width, (JDIMENSION) cinfo->output_components);

  /* Allocate a virtual array to hold the image. */
  dest->image = (*cinfo->mem->request_virt_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
     (JDIMENSION) (cinfo->output_width * cinfo->output_components),
     cinfo->output_height, (JDIMENSION) 1);

  return (djpeg_dest_ptr) dest;
}

#endif /* RLE_SUPPORTED */
