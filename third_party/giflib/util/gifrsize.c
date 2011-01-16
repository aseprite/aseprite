/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to resize GIF by given factors horizontally and vertically.	     *
* Options:								     *
* -q : quiet printing mode.						     *
* -S x y : scale into size as specified by x and y.			     *
* -s r : resize both x & y direction by factor r.			     *
* -x r : resize the x direction (horizontally) by factor r.		     *
* -y r : resize the y direction (vertically) by factor r.		     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 4 Jul 89 - Version 1.0 by Gershon Elber.				     *
* 22 Dec 89 - Fix minor bag in discarding last line of input (Version 1.1).  *
* 3 Aug 91 - make it scale by an arbitrary size value.			     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <stdlib.h>
#include <alloc.h>
#endif /* __MSDOS__ */

#ifndef __MSDOS__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"GifRSize"

#define MAX_SCALE	16.0			  /* Maximum scaling factor. */

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifRSize q%- S%-X|Y!d!d s%-Scale!F x%-XScale!F y%-YScale!F h%- GifFile!*s";
#else
static char
    *VersionStr =
	PROGRAM_NAME
	GIF_LIB_VERSION
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr =
	PROGRAM_NAME
	" q%- S%-X|Y!d!d s%-Scale!F x%-XScale!F y%-YScale!F h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static GifPixelType
    BackGroundColor = 0;
static double
    XScale = 0.5,
    YScale = 0.5;
static int
    XSize = 0,
    YSize = 0;

static void ResizeLine(GifRowType LineIn, GifRowType LineOut,
		      int InLineLen, int OutLineLen);
static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, iy, last_iy, l, t, w, h, Error, NumFiles, ExtCode,
	ImageNum = 0,
	SizeFlag = FALSE,
	ScaleFlag = FALSE,
	XScaleFlag = FALSE,
	YScaleFlag = FALSE,
	HelpFlag = FALSE;
    double Scale, y;
    GifRecordType RecordType;
    char s[80];
    GifByteType *Extension;
    GifRowType LineIn, LineOut;
    char **FileName = NULL;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &SizeFlag, &XSize, &YSize, &ScaleFlag, &Scale,
		&XScaleFlag, &XScale, &YScaleFlag, &YScale,
		&HelpFlag, &NumFiles, &FileName)) != FALSE ||
		(NumFiles > 1 && !HelpFlag)) {
	if (Error)
	    GAPrintErrMsg(Error);
	else if (NumFiles > 1)
	    GIF_MESSAGE("Error in command line parsing - one GIF file please.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (HelpFlag) {
	fprintf(stderr, VersionStr);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_SUCCESS);
    }

    /* If specific direction was set, set other direction to 1: */
    if (!XScaleFlag && YScaleFlag) XScale = 1.0;
    if (!YScaleFlag && XScaleFlag) YScale = 1.0;

    /* If the specific direction was not set, but global one did use it: */
    if (!XScaleFlag && ScaleFlag) XScale = Scale;
    if (!YScaleFlag && ScaleFlag) YScale = Scale;

    if (XScale > MAX_SCALE) {
	sprintf(s, "XScale too big, maximum scale selected instead (%f).",
								MAX_SCALE);
	GIF_MESSAGE(s);
	XScale = MAX_SCALE;
    }
    if (YScale > MAX_SCALE) {
	sprintf(s, "YScale too big, maximum scale selected instead (%f).",
								MAX_SCALE);
	GIF_MESSAGE(s);
	YScale = MAX_SCALE;
    }

    if (NumFiles == 1) {
	if ((GifFileIn = DGifOpenFileName(*FileName)) == NULL)
	    QuitGifError(GifFileIn, GifFileOut);
    }
    else {
	/* Use the stdin instead: */
	if ((GifFileIn = DGifOpenFileHandle(0)) == NULL)
	    QuitGifError(GifFileIn, GifFileOut);
    }
    BackGroundColor = GifFileIn->SBackGroundColor;

    /* If size was specified, it is used to derive the scale: */
    if (SizeFlag) {
	XScale = XSize / ((double) GifFileIn->SWidth);
	YScale = YSize / ((double) GifFileIn->SHeight);
    }
    else
    {
	XSize = (int) (GifFileIn->SWidth * XScale + 0.5);
	YSize = (int) (GifFileIn->SHeight * YScale + 0.5);
    }

    /* As at this time we know the Screen size of the input gif file, and as */
    /* all image(s) in file must be less/equal to it, we can allocate the    */
    /* scan lines for the input file, and output file. The number of lines   */
    /* to allocate for each is set by ScaleDown & XScale & YScale:	     */
    LineOut = (GifRowType) malloc(XSize * sizeof(GifPixelType));
    LineIn = (GifRowType) malloc(GifFileIn->SWidth * sizeof(GifPixelType));

    /* Open stdout for the output file: */
    if ((GifFileOut = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFileIn, GifFileOut);

    /* And dump out its new scaled screen information: */
    if (EGifPutScreenDesc(GifFileOut, XSize, YSize,
	GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
	GifFileIn->SColorMap) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);

    /* Scan the content of the GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);

	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		/* Put the image descriptor to out file: */
		l = (int) (GifFileIn->Image.Left * XScale + 0.5);
		w = (int) (GifFileIn->Image.Width * XScale + 0.5);
		t = (int) (GifFileIn->Image.Top * YScale + 0.5);
		h = (int) (GifFileIn->Image.Height * YScale + 0.5);
		if (l < 0) l = 0;
		if (t < 0) t = 0;
		if (l + w > XSize) w = XSize - l;
		if (t + h > YSize) h = YSize - t;

		if (EGifPutImageDesc(GifFileOut, l, t, w, h,
		    GifFileIn->Image.Interlace,
		    GifFileIn->Image.ColorMap) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);

		if (GifFileIn->Image.Interlace) {
		    GIF_EXIT("Cannt resize interlaced images - use GifInter first.");
		}
		else {
		    GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ",
			PROGRAM_NAME, ++ImageNum,
			GifFileOut->Image.Left, GifFileOut->Image.Top,
			GifFileOut->Image.Width, GifFileOut->Image.Height);

		    for (i = GifFileIn->Image.Height, y = 0.0, last_iy = -1;
			 i-- > 0;
			 y += YScale) {
			if (DGifGetLine(GifFileIn, LineIn,
					GifFileIn->Image.Width) == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);

			iy = (int) y;
			if (last_iy < iy && last_iy < YSize) {
			    ResizeLine(LineIn, LineOut,
				       GifFileIn->Image.Width, GifFileOut->Image.Width);

			    for (;
				 last_iy < iy && last_iy < GifFileOut->Image.Height - 1;
				 last_iy++) {
				GifQprintf("\b\b\b\b%-4d", last_iy + 1);
				if (EGifPutLine(GifFileOut, LineOut,
						GifFileOut->Image.Width) ==
								    GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);
			    }
			}
		    }

		    /* If scale is not dividable - dump last lines: */
		    while (++last_iy < GifFileOut->Image.Height) {
			GifQprintf("\b\b\b\b%-4d", last_iy);
			if (EGifPutLine(GifFileOut, LineOut,
					GifFileOut->Image.Width) == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);
		    }
		}
		break;
	    case EXTENSION_RECORD_TYPE:
		/* Skip any extension blocks in file: */
		if (DGifGetExtension(GifFileIn, &ExtCode, &Extension) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		if (EGifPutExtension(GifFileOut, ExtCode, Extension[0],
							Extension) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);

		/* No support to more than one extension blocks, so discard: */
		while (Extension != NULL) {
		    if (DGifGetExtensionNext(GifFileIn, &Extension) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		}
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		    /* Should be traps by DGifGetRecordType. */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE);

    if (DGifCloseFile(GifFileIn) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);
    if (EGifCloseFile(GifFileOut) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);

    free(LineOut);
    free(LineIn);

    return 0;
}

/******************************************************************************
* Line resizing routine - scale given lines as follows:			      *
* Scale (by pixel duplication/elimination) from InLineLen to OutLineLen.      *
******************************************************************************/
static void ResizeLine(GifRowType LineIn, GifRowType LineOut,
		      int InLineLen, int OutLineLen)
{
    int i, ix, last_ix;
    double x;

    OutLineLen--;

    for (i = InLineLen, x = 0.0, last_ix = -1;
	 i-- > 0;
	 x += XScale, LineIn++)
    {
	ix = (int) x;
    	for (; last_ix < ix && last_ix < OutLineLen; last_ix++)
	    *LineOut++ = *LineIn;
    }

    /* Make sure the line is complete. */
    for (LineIn--; last_ix < OutLineLen; last_ix++)
	*LineOut++ = *LineIn;
}

/******************************************************************************
* Close both input and output file (if open), and exit.			      *
******************************************************************************/
static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut)
{
    PrintGifError();
    if (GifFileIn != NULL) DGifCloseFile(GifFileIn);
    if (GifFileOut != NULL) EGifCloseFile(GifFileOut);
    exit(EXIT_FAILURE);
}
