/*
 * transupp.c
 *
 * Copyright (C) 1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains image transformation routines and other utility code
 * used by the jpegtran sample application.  These are NOT part of the core
 * JPEG library.  But we keep these routines separate from jpegtran.c to
 * ease the task of maintaining jpegtran-like programs that have other user
 * interfaces.
 */

/* Although this file really shouldn't have access to the library internals,
 * it's helpful to let it call jround_up() and jcopy_block_row().
 */
#define JPEG_INTERNALS

#include "jinclude.h"
#include "jpeglib.h"
#include "transupp.h"		/* My own external interface */


#if TRANSFORMS_SUPPORTED

/*
 * Lossless image transformation routines.  These routines work on DCT
 * coefficient arrays and thus do not require any lossy decompression
 * or recompression of the image.
 * Thanks to Guido Vollbeding for the initial design and code of this feature.
 *
 * Horizontal flipping is done in-place, using a single top-to-bottom
 * pass through the virtual source array.  It will thus be much the
 * fastest option for images larger than main memory.
 *
 * The other routines require a set of destination virtual arrays, so they
 * need twice as much memory as jpegtran normally does.  The destination
 * arrays are always written in normal scan order (top to bottom) because
 * the virtual array manager expects this.  The source arrays will be scanned
 * in the corresponding order, which means multiple passes through the source
 * arrays for most of the transforms.  That could result in much thrashing
 * if the image is larger than main memory.
 *
 * Some notes about the operating environment of the individual transform
 * routines:
 * 1. Both the source and destination virtual arrays are allocated from the
 *    source JPEG object, and therefore should be manipulated by calling the
 *    source's memory manager.
 * 2. The destination's component count should be used.  It may be smaller
 *    than the source's when forcing to grayscale.
 * 3. Likewise the destination's sampling factors should be used.  When
 *    forcing to grayscale the destination's sampling factors will be all 1,
 *    and we may as well take that as the effective iMCU size.
 * 4. When "trim" is in effect, the destination's dimensions will be the
 *    trimmed values but the source's will be untrimmed.
 * 5. All the routines assume that the source and destination buffers are
 *    padded out to a full iMCU boundary.  This is true, although for the
 *    source buffer it is an undocumented property of jdcoefct.c.
 * Notes 2,3,4 boil down to this: generally we should use the destination's
 * dimensions and ignore the source's.
 */


LOCAL(void)
do_flip_h (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	   jvirt_barray_ptr *src_coef_arrays)
/* Horizontal flip; done in-place, so no separate dest array is required */
{
  JDIMENSION MCU_cols, comp_width, blk_x, blk_y;
  int ci, k, offset_y;
  JBLOCKARRAY buffer;
  JCOEFPTR ptr1, ptr2;
  JCOEF temp1, temp2;
  jpeg_component_info *compptr;

  /* Horizontal mirroring of DCT blocks is accomplished by swapping
   * pairs of blocks in-place.  Within a DCT block, we perform horizontal
   * mirroring by changing the signs of odd-numbered columns.
   * Partial iMCUs at the right edge are left untouched.
   */
  MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    for (blk_y = 0; blk_y < compptr->height_in_blocks;
	 blk_y += compptr->v_samp_factor) {
      buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, src_coef_arrays[ci], blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	for (blk_x = 0; blk_x * 2 < comp_width; blk_x++) {
	  ptr1 = buffer[offset_y][blk_x];
	  ptr2 = buffer[offset_y][comp_width - blk_x - 1];
	  /* this unrolled loop doesn't need to know which row it's on... */
	  for (k = 0; k < DCTSIZE2; k += 2) {
	    temp1 = *ptr1;	/* swap even column */
	    temp2 = *ptr2;
	    *ptr1++ = temp2;
	    *ptr2++ = temp1;
	    temp1 = *ptr1;	/* swap odd column with sign change */
	    temp2 = *ptr2;
	    *ptr1++ = -temp2;
	    *ptr2++ = -temp1;
	  }
	}
      }
    }
  }
}


LOCAL(void)
do_flip_v (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	   jvirt_barray_ptr *src_coef_arrays,
	   jvirt_barray_ptr *dst_coef_arrays)
/* Vertical flip */
{
  JDIMENSION MCU_rows, comp_height, dst_blk_x, dst_blk_y;
  int ci, i, j, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JBLOCKROW src_row_ptr, dst_row_ptr;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  /* We output into a separate array because we can't touch different
   * rows of the source virtual array simultaneously.  Otherwise, this
   * is a pretty straightforward analog of horizontal flip.
   * Within a DCT block, vertical mirroring is done by changing the signs
   * of odd-numbered rows.
   * Partial iMCUs at the bottom edge are copied verbatim.
   */
  MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_height = MCU_rows * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
	 dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      if (dst_blk_y < comp_height) {
	/* Row is within the mirrorable area. */
	src_buffer = (*srcinfo->mem->access_virt_barray)
	  ((j_common_ptr) srcinfo, src_coef_arrays[ci],
	   comp_height - dst_blk_y - (JDIMENSION) compptr->v_samp_factor,
	   (JDIMENSION) compptr->v_samp_factor, FALSE);
      } else {
	/* Bottom-edge blocks will be copied verbatim. */
	src_buffer = (*srcinfo->mem->access_virt_barray)
	  ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_y,
	   (JDIMENSION) compptr->v_samp_factor, FALSE);
      }
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	if (dst_blk_y < comp_height) {
	  /* Row is within the mirrorable area. */
	  dst_row_ptr = dst_buffer[offset_y];
	  src_row_ptr = src_buffer[compptr->v_samp_factor - offset_y - 1];
	  for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
	       dst_blk_x++) {
	    dst_ptr = dst_row_ptr[dst_blk_x];
	    src_ptr = src_row_ptr[dst_blk_x];
	    for (i = 0; i < DCTSIZE; i += 2) {
	      /* copy even row */
	      for (j = 0; j < DCTSIZE; j++)
		*dst_ptr++ = *src_ptr++;
	      /* copy odd row with sign change */
	      for (j = 0; j < DCTSIZE; j++)
		*dst_ptr++ = - *src_ptr++;
	    }
	  }
	} else {
	  /* Just copy row verbatim. */
	  jcopy_block_row(src_buffer[offset_y], dst_buffer[offset_y],
			  compptr->width_in_blocks);
	}
      }
    }
  }
}


LOCAL(void)
do_transpose (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	      jvirt_barray_ptr *src_coef_arrays,
	      jvirt_barray_ptr *dst_coef_arrays)
/* Transpose source into destination */
{
  JDIMENSION dst_blk_x, dst_blk_y;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  /* Transposing pixels within a block just requires transposing the
   * DCT coefficients.
   * Partial iMCUs at the edges require no special treatment; we simply
   * process all the available DCT blocks for every component.
   */
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
	 dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
	     dst_blk_x += compptr->h_samp_factor) {
	  src_buffer = (*srcinfo->mem->access_virt_barray)
	    ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_x,
	     (JDIMENSION) compptr->h_samp_factor, FALSE);
	  for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
	    src_ptr = src_buffer[offset_x][dst_blk_y + offset_y];
	    dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
	    for (i = 0; i < DCTSIZE; i++)
	      for (j = 0; j < DCTSIZE; j++)
		dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
	  }
	}
      }
    }
  }
}


LOCAL(void)
do_rot_90 (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	   jvirt_barray_ptr *src_coef_arrays,
	   jvirt_barray_ptr *dst_coef_arrays)
/* 90 degree rotation is equivalent to
 *   1. Transposing the image;
 *   2. Horizontal mirroring.
 * These two steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU_cols, comp_width, dst_blk_x, dst_blk_y;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  /* Because of the horizontal mirror step, we can't process partial iMCUs
   * at the (output) right edge properly.  They just get transposed and
   * not mirrored.
   */
  MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
	 dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
	     dst_blk_x += compptr->h_samp_factor) {
	  src_buffer = (*srcinfo->mem->access_virt_barray)
	    ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_x,
	     (JDIMENSION) compptr->h_samp_factor, FALSE);
	  for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
	    src_ptr = src_buffer[offset_x][dst_blk_y + offset_y];
	    if (dst_blk_x < comp_width) {
	      /* Block is within the mirrorable area. */
	      dst_ptr = dst_buffer[offset_y]
		[comp_width - dst_blk_x - offset_x - 1];
	      for (i = 0; i < DCTSIZE; i++) {
		for (j = 0; j < DCTSIZE; j++)
		  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
		i++;
		for (j = 0; j < DCTSIZE; j++)
		  dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
	      }
	    } else {
	      /* Edge blocks are transposed but not mirrored. */
	      dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
	      for (i = 0; i < DCTSIZE; i++)
		for (j = 0; j < DCTSIZE; j++)
		  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
	    }
	  }
	}
      }
    }
  }
}


LOCAL(void)
do_rot_270 (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	    jvirt_barray_ptr *src_coef_arrays,
	    jvirt_barray_ptr *dst_coef_arrays)
/* 270 degree rotation is equivalent to
 *   1. Horizontal mirroring;
 *   2. Transposing the image.
 * These two steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU_rows, comp_height, dst_blk_x, dst_blk_y;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  /* Because of the horizontal mirror step, we can't process partial iMCUs
   * at the (output) bottom edge properly.  They just get transposed and
   * not mirrored.
   */
  MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_height = MCU_rows * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
	 dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
	     dst_blk_x += compptr->h_samp_factor) {
	  src_buffer = (*srcinfo->mem->access_virt_barray)
	    ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_x,
	     (JDIMENSION) compptr->h_samp_factor, FALSE);
	  for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
	    dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
	    if (dst_blk_y < comp_height) {
	      /* Block is within the mirrorable area. */
	      src_ptr = src_buffer[offset_x]
		[comp_height - dst_blk_y - offset_y - 1];
	      for (i = 0; i < DCTSIZE; i++) {
		for (j = 0; j < DCTSIZE; j++) {
		  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
		  j++;
		  dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
		}
	      }
	    } else {
	      /* Edge blocks are transposed but not mirrored. */
	      src_ptr = src_buffer[offset_x][dst_blk_y + offset_y];
	      for (i = 0; i < DCTSIZE; i++)
		for (j = 0; j < DCTSIZE; j++)
		  dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
	    }
	  }
	}
      }
    }
  }
}


LOCAL(void)
do_rot_180 (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	    jvirt_barray_ptr *src_coef_arrays,
	    jvirt_barray_ptr *dst_coef_arrays)
/* 180 degree rotation is equivalent to
 *   1. Vertical mirroring;
 *   2. Horizontal mirroring.
 * These two steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU_cols, MCU_rows, comp_width, comp_height, dst_blk_x, dst_blk_y;
  int ci, i, j, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JBLOCKROW src_row_ptr, dst_row_ptr;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);
  MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    comp_height = MCU_rows * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
	 dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      if (dst_blk_y < comp_height) {
	/* Row is within the vertically mirrorable area. */
	src_buffer = (*srcinfo->mem->access_virt_barray)
	  ((j_common_ptr) srcinfo, src_coef_arrays[ci],
	   comp_height - dst_blk_y - (JDIMENSION) compptr->v_samp_factor,
	   (JDIMENSION) compptr->v_samp_factor, FALSE);
      } else {
	/* Bottom-edge rows are only mirrored horizontally. */
	src_buffer = (*srcinfo->mem->access_virt_barray)
	  ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_y,
	   (JDIMENSION) compptr->v_samp_factor, FALSE);
      }
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	if (dst_blk_y < comp_height) {
	  /* Row is within the mirrorable area. */
	  dst_row_ptr = dst_buffer[offset_y];
	  src_row_ptr = src_buffer[compptr->v_samp_factor - offset_y - 1];
	  /* Process the blocks that can be mirrored both ways. */
	  for (dst_blk_x = 0; dst_blk_x < comp_width; dst_blk_x++) {
	    dst_ptr = dst_row_ptr[dst_blk_x];
	    src_ptr = src_row_ptr[comp_width - dst_blk_x - 1];
	    for (i = 0; i < DCTSIZE; i += 2) {
	      /* For even row, negate every odd column. */
	      for (j = 0; j < DCTSIZE; j += 2) {
		*dst_ptr++ = *src_ptr++;
		*dst_ptr++ = - *src_ptr++;
	      }
	      /* For odd row, negate every even column. */
	      for (j = 0; j < DCTSIZE; j += 2) {
		*dst_ptr++ = - *src_ptr++;
		*dst_ptr++ = *src_ptr++;
	      }
	    }
	  }
	  /* Any remaining right-edge blocks are only mirrored vertically. */
	  for (; dst_blk_x < compptr->width_in_blocks; dst_blk_x++) {
	    dst_ptr = dst_row_ptr[dst_blk_x];
	    src_ptr = src_row_ptr[dst_blk_x];
	    for (i = 0; i < DCTSIZE; i += 2) {
	      for (j = 0; j < DCTSIZE; j++)
		*dst_ptr++ = *src_ptr++;
	      for (j = 0; j < DCTSIZE; j++)
		*dst_ptr++ = - *src_ptr++;
	    }
	  }
	} else {
	  /* Remaining rows are just mirrored horizontally. */
	  dst_row_ptr = dst_buffer[offset_y];
	  src_row_ptr = src_buffer[offset_y];
	  /* Process the blocks that can be mirrored. */
	  for (dst_blk_x = 0; dst_blk_x < comp_width; dst_blk_x++) {
	    dst_ptr = dst_row_ptr[dst_blk_x];
	    src_ptr = src_row_ptr[comp_width - dst_blk_x - 1];
	    for (i = 0; i < DCTSIZE2; i += 2) {
	      *dst_ptr++ = *src_ptr++;
	      *dst_ptr++ = - *src_ptr++;
	    }
	  }
	  /* Any remaining right-edge blocks are only copied. */
	  for (; dst_blk_x < compptr->width_in_blocks; dst_blk_x++) {
	    dst_ptr = dst_row_ptr[dst_blk_x];
	    src_ptr = src_row_ptr[dst_blk_x];
	    for (i = 0; i < DCTSIZE2; i++)
	      *dst_ptr++ = *src_ptr++;
	  }
	}
      }
    }
  }
}


LOCAL(void)
do_transverse (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
	       jvirt_barray_ptr *src_coef_arrays,
	       jvirt_barray_ptr *dst_coef_arrays)
/* Transverse transpose is equivalent to
 *   1. 180 degree rotation;
 *   2. Transposition;
 * or
 *   1. Horizontal mirroring;
 *   2. Transposition;
 *   3. Horizontal mirroring.
 * These steps are merged into a single processing routine.
 */
{
  JDIMENSION MCU_cols, MCU_rows, comp_width, comp_height, dst_blk_x, dst_blk_y;
  int ci, i, j, offset_x, offset_y;
  JBLOCKARRAY src_buffer, dst_buffer;
  JCOEFPTR src_ptr, dst_ptr;
  jpeg_component_info *compptr;

  MCU_cols = dstinfo->image_width / (dstinfo->max_h_samp_factor * DCTSIZE);
  MCU_rows = dstinfo->image_height / (dstinfo->max_v_samp_factor * DCTSIZE);

  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    comp_width = MCU_cols * compptr->h_samp_factor;
    comp_height = MCU_rows * compptr->v_samp_factor;
    for (dst_blk_y = 0; dst_blk_y < compptr->height_in_blocks;
	 dst_blk_y += compptr->v_samp_factor) {
      dst_buffer = (*srcinfo->mem->access_virt_barray)
	((j_common_ptr) srcinfo, dst_coef_arrays[ci], dst_blk_y,
	 (JDIMENSION) compptr->v_samp_factor, TRUE);
      for (offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
	for (dst_blk_x = 0; dst_blk_x < compptr->width_in_blocks;
	     dst_blk_x += compptr->h_samp_factor) {
	  src_buffer = (*srcinfo->mem->access_virt_barray)
	    ((j_common_ptr) srcinfo, src_coef_arrays[ci], dst_blk_x,
	     (JDIMENSION) compptr->h_samp_factor, FALSE);
	  for (offset_x = 0; offset_x < compptr->h_samp_factor; offset_x++) {
	    if (dst_blk_y < comp_height) {
	      src_ptr = src_buffer[offset_x]
		[comp_height - dst_blk_y - offset_y - 1];
	      if (dst_blk_x < comp_width) {
		/* Block is within the mirrorable area. */
		dst_ptr = dst_buffer[offset_y]
		  [comp_width - dst_blk_x - offset_x - 1];
		for (i = 0; i < DCTSIZE; i++) {
		  for (j = 0; j < DCTSIZE; j++) {
		    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
		    j++;
		    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
		  }
		  i++;
		  for (j = 0; j < DCTSIZE; j++) {
		    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
		    j++;
		    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
		  }
		}
	      } else {
		/* Right-edge blocks are mirrored in y only */
		dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
		for (i = 0; i < DCTSIZE; i++) {
		  for (j = 0; j < DCTSIZE; j++) {
		    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
		    j++;
		    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
		  }
		}
	      }
	    } else {
	      src_ptr = src_buffer[offset_x][dst_blk_y + offset_y];
	      if (dst_blk_x < comp_width) {
		/* Bottom-edge blocks are mirrored in x only */
		dst_ptr = dst_buffer[offset_y]
		  [comp_width - dst_blk_x - offset_x - 1];
		for (i = 0; i < DCTSIZE; i++) {
		  for (j = 0; j < DCTSIZE; j++)
		    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
		  i++;
		  for (j = 0; j < DCTSIZE; j++)
		    dst_ptr[j*DCTSIZE+i] = -src_ptr[i*DCTSIZE+j];
		}
	      } else {
		/* At lower right corner, just transpose, no mirroring */
		dst_ptr = dst_buffer[offset_y][dst_blk_x + offset_x];
		for (i = 0; i < DCTSIZE; i++)
		  for (j = 0; j < DCTSIZE; j++)
		    dst_ptr[j*DCTSIZE+i] = src_ptr[i*DCTSIZE+j];
	      }
	    }
	  }
	}
      }
    }
  }
}


/* Request any required workspace.
 *
 * We allocate the workspace virtual arrays from the source decompression
 * object, so that all the arrays (both the original data and the workspace)
 * will be taken into account while making memory management decisions.
 * Hence, this routine must be called after jpeg_read_header (which reads
 * the image dimensions) and before jpeg_read_coefficients (which realizes
 * the source's virtual arrays).
 */

GLOBAL(void)
jtransform_request_workspace (j_decompress_ptr srcinfo,
			      jpeg_transform_info *info)
{
  jvirt_barray_ptr *coef_arrays = NULL;
  jpeg_component_info *compptr;
  int ci;

  if (info->force_grayscale &&
      srcinfo->jpeg_color_space == JCS_YCbCr &&
      srcinfo->num_components == 3) {
    /* We'll only process the first component */
    info->num_components = 1;
  } else {
    /* Process all the components */
    info->num_components = srcinfo->num_components;
  }

  switch (info->transform) {
  case JXFORM_NONE:
  case JXFORM_FLIP_H:
    /* Don't need a workspace array */
    break;
  case JXFORM_FLIP_V:
  case JXFORM_ROT_180:
    /* Need workspace arrays having same dimensions as source image.
     * Note that we allocate arrays padded out to the next iMCU boundary,
     * so that transform routines need not worry about missing edge blocks.
     */
    coef_arrays = (jvirt_barray_ptr *)
      (*srcinfo->mem->alloc_small) ((j_common_ptr) srcinfo, JPOOL_IMAGE,
	SIZEOF(jvirt_barray_ptr) * info->num_components);
    for (ci = 0; ci < info->num_components; ci++) {
      compptr = srcinfo->comp_info + ci;
      coef_arrays[ci] = (*srcinfo->mem->request_virt_barray)
	((j_common_ptr) srcinfo, JPOOL_IMAGE, FALSE,
	 (JDIMENSION) jround_up((long) compptr->width_in_blocks,
				(long) compptr->h_samp_factor),
	 (JDIMENSION) jround_up((long) compptr->height_in_blocks,
				(long) compptr->v_samp_factor),
	 (JDIMENSION) compptr->v_samp_factor);
    }
    break;
  case JXFORM_TRANSPOSE:
  case JXFORM_TRANSVERSE:
  case JXFORM_ROT_90:
  case JXFORM_ROT_270:
    /* Need workspace arrays having transposed dimensions.
     * Note that we allocate arrays padded out to the next iMCU boundary,
     * so that transform routines need not worry about missing edge blocks.
     */
    coef_arrays = (jvirt_barray_ptr *)
      (*srcinfo->mem->alloc_small) ((j_common_ptr) srcinfo, JPOOL_IMAGE,
	SIZEOF(jvirt_barray_ptr) * info->num_components);
    for (ci = 0; ci < info->num_components; ci++) {
      compptr = srcinfo->comp_info + ci;
      coef_arrays[ci] = (*srcinfo->mem->request_virt_barray)
	((j_common_ptr) srcinfo, JPOOL_IMAGE, FALSE,
	 (JDIMENSION) jround_up((long) compptr->height_in_blocks,
				(long) compptr->v_samp_factor),
	 (JDIMENSION) jround_up((long) compptr->width_in_blocks,
				(long) compptr->h_samp_factor),
	 (JDIMENSION) compptr->h_samp_factor);
    }
    break;
  }
  info->workspace_coef_arrays = coef_arrays;
}


/* Transpose destination image parameters */

LOCAL(void)
transpose_critical_parameters (j_compress_ptr dstinfo)
{
  int tblno, i, j, ci, itemp;
  jpeg_component_info *compptr;
  JQUANT_TBL *qtblptr;
  JDIMENSION dtemp;
  UINT16 qtemp;

  /* Transpose basic image dimensions */
  dtemp = dstinfo->image_width;
  dstinfo->image_width = dstinfo->image_height;
  dstinfo->image_height = dtemp;

  /* Transpose sampling factors */
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    compptr = dstinfo->comp_info + ci;
    itemp = compptr->h_samp_factor;
    compptr->h_samp_factor = compptr->v_samp_factor;
    compptr->v_samp_factor = itemp;
  }

  /* Transpose quantization tables */
  for (tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) {
    qtblptr = dstinfo->quant_tbl_ptrs[tblno];
    if (qtblptr != NULL) {
      for (i = 0; i < DCTSIZE; i++) {
	for (j = 0; j < i; j++) {
	  qtemp = qtblptr->quantval[i*DCTSIZE+j];
	  qtblptr->quantval[i*DCTSIZE+j] = qtblptr->quantval[j*DCTSIZE+i];
	  qtblptr->quantval[j*DCTSIZE+i] = qtemp;
	}
      }
    }
  }
}


/* Trim off any partial iMCUs on the indicated destination edge */

LOCAL(void)
trim_right_edge (j_compress_ptr dstinfo)
{
  int ci, max_h_samp_factor;
  JDIMENSION MCU_cols;

  /* We have to compute max_h_samp_factor ourselves,
   * because it hasn't been set yet in the destination
   * (and we don't want to use the source's value).
   */
  max_h_samp_factor = 1;
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    int h_samp_factor = dstinfo->comp_info[ci].h_samp_factor;
    max_h_samp_factor = MAX(max_h_samp_factor, h_samp_factor);
  }
  MCU_cols = dstinfo->image_width / (max_h_samp_factor * DCTSIZE);
  if (MCU_cols > 0)		/* can't trim to 0 pixels */
    dstinfo->image_width = MCU_cols * (max_h_samp_factor * DCTSIZE);
}

LOCAL(void)
trim_bottom_edge (j_compress_ptr dstinfo)
{
  int ci, max_v_samp_factor;
  JDIMENSION MCU_rows;

  /* We have to compute max_v_samp_factor ourselves,
   * because it hasn't been set yet in the destination
   * (and we don't want to use the source's value).
   */
  max_v_samp_factor = 1;
  for (ci = 0; ci < dstinfo->num_components; ci++) {
    int v_samp_factor = dstinfo->comp_info[ci].v_samp_factor;
    max_v_samp_factor = MAX(max_v_samp_factor, v_samp_factor);
  }
  MCU_rows = dstinfo->image_height / (max_v_samp_factor * DCTSIZE);
  if (MCU_rows > 0)		/* can't trim to 0 pixels */
    dstinfo->image_height = MCU_rows * (max_v_samp_factor * DCTSIZE);
}


/* Adjust output image parameters as needed.
 *
 * This must be called after jpeg_copy_critical_parameters()
 * and before jpeg_write_coefficients().
 *
 * The return value is the set of virtual coefficient arrays to be written
 * (either the ones allocated by jtransform_request_workspace, or the
 * original source data arrays).  The caller will need to pass this value
 * to jpeg_write_coefficients().
 */

GLOBAL(jvirt_barray_ptr *)
jtransform_adjust_parameters (j_decompress_ptr srcinfo,
			      j_compress_ptr dstinfo,
			      jvirt_barray_ptr *src_coef_arrays,
			      jpeg_transform_info *info)
{
  /* If force-to-grayscale is requested, adjust destination parameters */
  if (info->force_grayscale) {
    /* We use jpeg_set_colorspace to make sure subsidiary settings get fixed
     * properly.  Among other things, the target h_samp_factor & v_samp_factor
     * will get set to 1, which typically won't match the source.
     * In fact we do this even if the source is already grayscale; that
     * provides an easy way of coercing a grayscale JPEG with funny sampling
     * factors to the customary 1,1.  (Some decoders fail on other factors.)
     */
    if ((dstinfo->jpeg_color_space == JCS_YCbCr &&
	 dstinfo->num_components == 3) ||
	(dstinfo->jpeg_color_space == JCS_GRAYSCALE &&
	 dstinfo->num_components == 1)) {
      /* We have to preserve the source's quantization table number. */
      int sv_quant_tbl_no = dstinfo->comp_info[0].quant_tbl_no;
      jpeg_set_colorspace(dstinfo, JCS_GRAYSCALE);
      dstinfo->comp_info[0].quant_tbl_no = sv_quant_tbl_no;
    } else {
      /* Sorry, can't do it */
      ERREXIT(dstinfo, JERR_CONVERSION_NOTIMPL);
    }
  }

  /* Correct the destination's image dimensions etc if necessary */
  switch (info->transform) {
  case JXFORM_NONE:
    /* Nothing to do */
    break;
  case JXFORM_FLIP_H:
    if (info->trim)
      trim_right_edge(dstinfo);
    break;
  case JXFORM_FLIP_V:
    if (info->trim)
      trim_bottom_edge(dstinfo);
    break;
  case JXFORM_TRANSPOSE:
    transpose_critical_parameters(dstinfo);
    /* transpose does NOT have to trim anything */
    break;
  case JXFORM_TRANSVERSE:
    transpose_critical_parameters(dstinfo);
    if (info->trim) {
      trim_right_edge(dstinfo);
      trim_bottom_edge(dstinfo);
    }
    break;
  case JXFORM_ROT_90:
    transpose_critical_parameters(dstinfo);
    if (info->trim)
      trim_right_edge(dstinfo);
    break;
  case JXFORM_ROT_180:
    if (info->trim) {
      trim_right_edge(dstinfo);
      trim_bottom_edge(dstinfo);
    }
    break;
  case JXFORM_ROT_270:
    transpose_critical_parameters(dstinfo);
    if (info->trim)
      trim_bottom_edge(dstinfo);
    break;
  }

  /* Return the appropriate output data set */
  if (info->workspace_coef_arrays != NULL)
    return info->workspace_coef_arrays;
  return src_coef_arrays;
}


/* Execute the actual transformation, if any.
 *
 * This must be called *after* jpeg_write_coefficients, because it depends
 * on jpeg_write_coefficients to have computed subsidiary values such as
 * the per-component width and height fields in the destination object.
 *
 * Note that some transformations will modify the source data arrays!
 */

GLOBAL(void)
jtransform_execute_transformation (j_decompress_ptr srcinfo,
				   j_compress_ptr dstinfo,
				   jvirt_barray_ptr *src_coef_arrays,
				   jpeg_transform_info *info)
{
  jvirt_barray_ptr *dst_coef_arrays = info->workspace_coef_arrays;

  switch (info->transform) {
  case JXFORM_NONE:
    break;
  case JXFORM_FLIP_H:
    do_flip_h(srcinfo, dstinfo, src_coef_arrays);
    break;
  case JXFORM_FLIP_V:
    do_flip_v(srcinfo, dstinfo, src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_TRANSPOSE:
    do_transpose(srcinfo, dstinfo, src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_TRANSVERSE:
    do_transverse(srcinfo, dstinfo, src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_ROT_90:
    do_rot_90(srcinfo, dstinfo, src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_ROT_180:
    do_rot_180(srcinfo, dstinfo, src_coef_arrays, dst_coef_arrays);
    break;
  case JXFORM_ROT_270:
    do_rot_270(srcinfo, dstinfo, src_coef_arrays, dst_coef_arrays);
    break;
  }
}

#endif /* TRANSFORMS_SUPPORTED */


/* Setup decompression object to save desired markers in memory.
 * This must be called before jpeg_read_header() to have the desired effect.
 */

GLOBAL(void)
jcopy_markers_setup (j_decompress_ptr srcinfo, JCOPY_OPTION option)
{
#ifdef SAVE_MARKERS_SUPPORTED
  int m;

  /* Save comments except under NONE option */
  if (option != JCOPYOPT_NONE) {
    jpeg_save_markers(srcinfo, JPEG_COM, 0xFFFF);
  }
  /* Save all types of APPn markers iff ALL option */
  if (option == JCOPYOPT_ALL) {
    for (m = 0; m < 16; m++)
      jpeg_save_markers(srcinfo, JPEG_APP0 + m, 0xFFFF);
  }
#endif /* SAVE_MARKERS_SUPPORTED */
}

/* Copy markers saved in the given source object to the destination object.
 * This should be called just after jpeg_start_compress() or
 * jpeg_write_coefficients().
 * Note that those routines will have written the SOI, and also the
 * JFIF APP0 or Adobe APP14 markers if selected.
 */

GLOBAL(void)
jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
		       JCOPY_OPTION option)
{
  jpeg_saved_marker_ptr marker;

  /* In the current implementation, we don't actually need to examine the
   * option flag here; we just copy everything that got saved.
   * But to avoid confusion, we do not output JFIF and Adobe APP14 markers
   * if the encoder library already wrote one.
   */
  for (marker = srcinfo->marker_list; marker != NULL; marker = marker->next) {
    if (dstinfo->write_JFIF_header &&
	marker->marker == JPEG_APP0 &&
	marker->data_length >= 5 &&
	GETJOCTET(marker->data[0]) == 0x4A &&
	GETJOCTET(marker->data[1]) == 0x46 &&
	GETJOCTET(marker->data[2]) == 0x49 &&
	GETJOCTET(marker->data[3]) == 0x46 &&
	GETJOCTET(marker->data[4]) == 0)
      continue;			/* reject duplicate JFIF */
    if (dstinfo->write_Adobe_marker &&
	marker->marker == JPEG_APP0+14 &&
	marker->data_length >= 5 &&
	GETJOCTET(marker->data[0]) == 0x41 &&
	GETJOCTET(marker->data[1]) == 0x64 &&
	GETJOCTET(marker->data[2]) == 0x6F &&
	GETJOCTET(marker->data[3]) == 0x62 &&
	GETJOCTET(marker->data[4]) == 0x65)
      continue;			/* reject duplicate Adobe */
#ifdef NEED_FAR_POINTERS
    /* We could use jpeg_write_marker if the data weren't FAR... */
    {
      unsigned int i;
      jpeg_write_m_header(dstinfo, marker->marker, marker->data_length);
      for (i = 0; i < marker->data_length; i++)
	jpeg_write_m_byte(dstinfo, marker->data[i]);
    }
#else
    jpeg_write_marker(dstinfo, marker->marker,
		      marker->data, marker->data_length);
#endif
  }
}
