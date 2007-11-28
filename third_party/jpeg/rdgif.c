/*
 * rdgif.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in GIF format.
 *
 *****************************************************************************
 * NOTE: to avoid entanglements with Unisys' patent on LZW compression,      *
 * the ability to read GIF files has been removed from the IJG distribution. *
 * Sorry about that.                                                         *
 *****************************************************************************
 *
 * We are required to state that
 *    "The Graphics Interchange Format(c) is the Copyright property of
 *    CompuServe Incorporated. GIF(sm) is a Service Mark property of
 *    CompuServe Incorporated."
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef GIF_SUPPORTED

/*
 * The module selection routine for GIF format input.
 */

GLOBAL(cjpeg_source_ptr)
jinit_read_gif (j_compress_ptr cinfo)
{
  fprintf(stderr, "GIF input is unsupported for legal reasons.  Sorry.\n");
  exit(EXIT_FAILURE);
  return NULL;			/* keep compiler happy */
}

#endif /* GIF_SUPPORTED */
