/*
 * rdrle.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in Utah RLE format.
 * The Utah Raster Toolkit library is required (version 3.1 or later).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; start_input may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed RLE format).
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
 * We support the following types of RLE files:
 *   
 *   GRAYSCALE   - 8 bits, no colormap
 *   MAPPEDGRAY  - 8 bits, 1 channel colomap
 *   PSEUDOCOLOR - 8 bits, 3 channel colormap
 *   TRUECOLOR   - 24 bits, 3 channel colormap
 *   DIRECTCOLOR - 24 bits, no colormap
 *
 * For now, we ignore any alpha channel in the image.
 */

typedef enum
  { GRAYSCALE, MAPPEDGRAY, PSEUDOCOLOR, TRUECOLOR, DIRECTCOLOR } rle_kind;


/*
 * Since RLE stores scanlines bottom-to-top, we have to invert the image
 * to conform to JPEG's top-to-bottom order.  To do this, we read the
 * incoming image into a virtual array on the first get_pixel_rows call,
 * then fetch the required row from the virtual array on subsequent calls.
 */

typedef struct _rle_source_struct * rle_source_ptr;

typedef struct _rle_source_struct {
  struct cjpeg_source_struct pub; /* public fields */

  rle_kind visual;              /* actual type of input file */
  jvirt_sarray_ptr image;       /* virtual array to hold the image */
  JDIMENSION row;		/* current row # in the virtual array */
  rle_hdr header;               /* Input file information */
  rle_pixel** rle_row;          /* holds a row returned by rle_getrow() */

} rle_source_struct;


/*
 * Read the file header; return image size and component count.
 */

METHODDEF(void)
start_input_rle (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;
  JDIMENSION width, height;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  /* Use RLE library routine to get the header info */
  source->header = *rle_hdr_init(NULL);
  source->header.rle_file = source->pub.input_file;
  switch (rle_get_setup(&(source->header))) {
  case RLE_SUCCESS:
    /* A-OK */
    break;
  case RLE_NOT_RLE:
    ERREXIT(cinfo, JERR_RLE_NOT);
    break;
  case RLE_NO_SPACE:
    ERREXIT(cinfo, JERR_RLE_MEM);
    break;
  case RLE_EMPTY:
    ERREXIT(cinfo, JERR_RLE_EMPTY);
    break;
  case RLE_EOF:
    ERREXIT(cinfo, JERR_RLE_EOF);
    break;
  default:
    ERREXIT(cinfo, JERR_RLE_BADERROR);
    break;
  }

  /* Figure out what we have, set private vars and return values accordingly */
  
  width  = source->header.xmax - source->header.xmin + 1;
  height = source->header.ymax - source->header.ymin + 1;
  source->header.xmin = 0;		/* realign horizontally */
  source->header.xmax = width-1;

  cinfo->image_width      = width;
  cinfo->image_height     = height;
  cinfo->data_precision   = 8;  /* we can only handle 8 bit data */

  if (source->header.ncolors == 1 && source->header.ncmap == 0) {
    source->visual     = GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_RLE_GRAY, width, height);
  } else if (source->header.ncolors == 1 && source->header.ncmap == 1) {
    source->visual     = MAPPEDGRAY;
    TRACEMS3(cinfo, 1, JTRC_RLE_MAPGRAY, width, height,
             1 << source->header.cmaplen);
  } else if (source->header.ncolors == 1 && source->header.ncmap == 3) {
    source->visual     = PSEUDOCOLOR;
    TRACEMS3(cinfo, 1, JTRC_RLE_MAPPED, width, height,
	     1 << source->header.cmaplen);
  } else if (source->header.ncolors == 3 && source->header.ncmap == 3) {
    source->visual     = TRUECOLOR;
    TRACEMS3(cinfo, 1, JTRC_RLE_FULLMAP, width, height,
	     1 << source->header.cmaplen);
  } else if (source->header.ncolors == 3 && source->header.ncmap == 0) {
    source->visual     = DIRECTCOLOR;
    TRACEMS2(cinfo, 1, JTRC_RLE, width, height);
  } else
    ERREXIT(cinfo, JERR_RLE_UNSUPPORTED);
  
  if (source->visual == GRAYSCALE || source->visual == MAPPEDGRAY) {
    cinfo->in_color_space   = JCS_GRAYSCALE;
    cinfo->input_components = 1;
  } else {
    cinfo->in_color_space   = JCS_RGB;
    cinfo->input_components = 3;
  }

  /*
   * A place to hold each scanline while it's converted.
   * (GRAYSCALE scanlines don't need converting)
   */
  if (source->visual != GRAYSCALE) {
    source->rle_row = (rle_pixel**) (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       (JDIMENSION) width, (JDIMENSION) cinfo->input_components);
  }

  /* request a virtual array to hold the image */
  source->image = (*cinfo->mem->request_virt_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
     (JDIMENSION) (width * source->header.ncolors),
     (JDIMENSION) height, (JDIMENSION) 1);

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    /* count file input as separate pass */
    progress->total_extra_passes++;
  }
#endif

  source->pub.buffer_height = 1;
}


/*
 * Read one row of pixels.
 * Called only after load_image has read the image into the virtual array.
 * Used for GRAYSCALE, MAPPEDGRAY, TRUECOLOR, and DIRECTCOLOR images.
 */

METHODDEF(JDIMENSION)
get_rle_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;

  source->row--;
  source->pub.buffer = (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, source->image, source->row, (JDIMENSION) 1, FALSE);

  return 1;
}

/*
 * Read one row of pixels.
 * Called only after load_image has read the image into the virtual array.
 * Used for PSEUDOCOLOR images.
 */

METHODDEF(JDIMENSION)
get_pseudocolor_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;
  JSAMPROW src_row, dest_row;
  JDIMENSION col;
  rle_map *colormap;
  int val;

  colormap = source->header.cmap;
  dest_row = source->pub.buffer[0];
  source->row--;
  src_row = * (*cinfo->mem->access_virt_sarray)
    ((j_common_ptr) cinfo, source->image, source->row, (JDIMENSION) 1, FALSE);

  for (col = cinfo->image_width; col > 0; col--) {
    val = GETJSAMPLE(*src_row++);
    *dest_row++ = (JSAMPLE) (colormap[val      ] >> 8);
    *dest_row++ = (JSAMPLE) (colormap[val + 256] >> 8);
    *dest_row++ = (JSAMPLE) (colormap[val + 512] >> 8);
  }

  return 1;
}


/*
 * Load the image into a virtual array.  We have to do this because RLE
 * files start at the lower left while the JPEG standard has them starting
 * in the upper left.  This is called the first time we want to get a row
 * of input.  What we do is load the RLE data into the array and then call
 * the appropriate routine to read one row from the array.  Before returning,
 * we set source->pub.get_pixel_rows so that subsequent calls go straight to
 * the appropriate row-reading routine.
 */

METHODDEF(JDIMENSION)
load_image (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  rle_source_ptr source = (rle_source_ptr) sinfo;
  JDIMENSION row, col;
  JSAMPROW  scanline, red_ptr, green_ptr, blue_ptr;
  rle_pixel **rle_row;
  rle_map *colormap;
  char channel;
#ifdef PROGRESS_REPORT
  cd_progress_ptr progress = (cd_progress_ptr) cinfo->progress;
#endif

  colormap = source->header.cmap;
  rle_row = source->rle_row;

  /* Read the RLE data into our virtual array.
   * We assume here that (a) rle_pixel is represented the same as JSAMPLE,
   * and (b) we are not on a machine where FAR pointers differ from regular.
   */
  RLE_CLR_BIT(source->header, RLE_ALPHA); /* don't read the alpha channel */

#ifdef PROGRESS_REPORT
  if (progress != NULL) {
    progress->pub.pass_limit = cinfo->image_height;
    progress->pub.pass_counter = 0;
    (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
  }
#endif

  switch (source->visual) {

  case GRAYSCALE:
  case PSEUDOCOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      rle_row = (rle_pixel **) (*cinfo->mem->access_virt_sarray)
         ((j_common_ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle_getrow(&source->header, rle_row);
#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
    break;

  case MAPPEDGRAY:
  case TRUECOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      scanline = * (*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle_row = source->rle_row;
      rle_getrow(&source->header, rle_row);

      for (col = 0; col < cinfo->image_width; col++) {
        for (channel = 0; channel < source->header.ncolors; channel++) {
          *scanline++ = (JSAMPLE)
            (colormap[GETJSAMPLE(rle_row[channel][col]) + 256 * channel] >> 8);
        }
      }

#ifdef PROGRESS_REPORT
      if (progress != NULL) {
        progress->pub.pass_counter++;
        (*progress->pub.progress_monitor) ((j_common_ptr) cinfo);
      }
#endif
    }
    break;

  case DIRECTCOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      scanline = * (*cinfo->mem->access_virt_sarray)
        ((j_common_ptr) cinfo, source->image, row, (JDIMENSION) 1, TRUE);
      rle_getrow(&source->header, rle_row);

      red_ptr   = rle_row[0];
      green_ptr = rle_row[1];
      blue_ptr  = rle_row[2];

      for (col = cinfo->image_width; col > 0; col--) {
        *scanline++ = *red_ptr++;
        *scanline++ = *green_ptr++;
        *scanline++ = *blue_ptr++;
      }

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

  /* Set up to call proper row-extraction routine in future */
  if (source->visual == PSEUDOCOLOR) {
    source->pub.buffer = source->rle_row;
    source->pub.get_pixel_rows = get_pseudocolor_row;
  } else {
    source->pub.get_pixel_rows = get_rle_row;
  }
  source->row = cinfo->image_height;

  /* And fetch the topmost (bottommost) row */
  return (*source->pub.get_pixel_rows) (cinfo, sinfo);   
}


/*
 * Finish up at the end of the file.
 */

METHODDEF(void)
finish_input_rle (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  /* no work */
}


/*
 * The module selection routine for RLE format input.
 */

GLOBAL(cjpeg_source_ptr)
jinit_read_rle (j_compress_ptr cinfo)
{
  rle_source_ptr source;

  /* Create module interface object */
  source = (rle_source_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                  SIZEOF(rle_source_struct));
  /* Fill in method ptrs */
  source->pub.start_input = start_input_rle;
  source->pub.finish_input = finish_input_rle;
  source->pub.get_pixel_rows = load_image;

  return (cjpeg_source_ptr) source;
}

#endif /* RLE_SUPPORTED */
