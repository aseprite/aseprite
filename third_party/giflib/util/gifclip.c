/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to clip an image and dump out only portion of it.		     *
* Options:								     *
* -q : quiet printing mode.						     *
* -i left top width bottom : clipping information for first image.	     *
* -n n left top width bottom : clipping information for nth image.	     *
* -c complement; remove the bands specified by -i or -n			     *
* -h : on-line help							     *
******************************************************************************
* History:								     *
* 8 Jul 89 - Version 1.0 by Gershon Elber.				     *
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

#define PROGRAM_NAME	"GifClip"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifClip q%- c%- i%-Xmin|Ymin|Xmax|Ymax!d!d!d!d n%-n|Xmin|Ymin|Xmax|Ymax!d!d!d!d!d h%- GifFile!*s";
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
	" q%- c%- i%-Xmin|Ymin|Xmax|Ymax!d!d!d!d n%-n|Xmin|Ymin|Xmax|Ymax!d!d!d!d!d h%- GifFile!*s";
#endif /* SYSV */

static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, Error, NumFiles, ExtCode, CodeSize, ImageNum = 0,
	ImageFlag = FALSE, ImageNFlag = FALSE, ImageN, ImageX1, ImageY1,
	ImageX2, ImageY2, ImageWidth, ImageDepth,
	Complement = FALSE, HelpFlag = FALSE;
    GifRecordType RecordType;
    GifByteType *Extension, *CodeBlock;
    char **FileName = NULL;
    GifRowType Line;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    /* Same image dimension vars for both Image & ImageN as only one allowed.*/
    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint, &Complement,
		&ImageFlag, &ImageX1, &ImageY1, &ImageX2, &ImageY2,
		&ImageNFlag, &ImageN, &ImageX1, &ImageY1, &ImageX2, &ImageY2,
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

    /* Test to make sure exactly one of ImageFlag & ImageNFlag is set: */
    if ((ImageFlag && ImageNFlag) || (!ImageFlag && !ImageNFlag)) {
	GIF_MESSAGE("Exactly one of [-i ...] && [-n ...] please.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }
    if (ImageFlag) ImageN = 1;		    /* Its first image we are after. */

    /* Make sure the first coordinates of clipping box are smaller: */
    if (ImageX1 > ImageX2) {
	i = ImageX1;
	ImageX1 = ImageX2;
	ImageX2 = i;
    }
    if (ImageY1 > ImageY2) {
	i = ImageX1;
	ImageY1 = ImageY2;
	ImageY2 = i;
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

    /* Open stdout for the output file: */
    if ((GifFileOut = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFileIn, GifFileOut);

    /* Width and depth of clipped image. */
    if (!Complement)
	ImageWidth = ImageX2 - ImageX1 + 1;
    else
	ImageWidth = GifFileIn->SWidth
	    - (ImageX2 != ImageX1) * (ImageX2 - ImageX1 + 1);
    if (!Complement)
	ImageDepth = ImageY2 - ImageY1 + 1;
    else
	ImageDepth = GifFileIn->SHeight
	    - (ImageY2 != ImageY1) * (ImageY2 - ImageY1 + 1);

    /* And dump out exactly same screen information: */
    if (EGifPutScreenDesc(GifFileOut,
	GifFileIn->SWidth, GifFileIn->SHeight,
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
		if (++ImageNum == ImageN) {
		    /* We can handle only non interlaced images here: */
		    if (GifFileIn->Image.Interlace)
			GIF_EXIT("Image to clip is interlaced - use GifInter first.");

		    /* This is the image we should clip - test sizes and     */
		    /* dump out new clipped screen descriptor if o.k.	     */
		    if (GifFileIn->Image.Width <= ImageX2 ||
			GifFileIn->Image.Height <= ImageY2)
			GIF_EXIT("Image is smaller than given clip dimensions.");

		    /* Put the image descriptor to out file: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			ImageWidth, ImageDepth,
			FALSE, GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);

		    /* o.k. - read the image and clip it: */
		    Line = (GifRowType) malloc(GifFileIn->Image.Width *
							sizeof(GifPixelType));
		    GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ",
			PROGRAM_NAME, ImageNum,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			GifFileIn->Image.Width, GifFileIn->Image.Height);

		    /* Skip lines below ImageY1: */
		    for (i = 0; i < ImageY1; i++) {
			if (DGifGetLine(GifFileIn, Line, GifFileIn->Image.Width)
			    == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);

			if (Complement) {
			    if (ImageX1 == ImageX2) {
				/* don't remove any vertical band */
				if (EGifPutLine(GifFileOut, Line,
						ImageWidth) == GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);
			    }
			    else
			    {
				if (EGifPutLine(GifFileOut, Line,
						ImageX1) == GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);

				if (EGifPutLine(GifFileOut,
						&Line[ImageX2 + 1],
						GifFileIn->SWidth - (ImageX2 + 1)
						) == GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);
			    }
			}

			GifQprintf("\b\b\b\b%-4d", i);
		    }

		    /* Clip the lines from ImageY1 to ImageY2 (to X1 - X2): */
		    for (i = ImageY1; i <= ImageY2; i++) {
			if (DGifGetLine(GifFileIn, Line, GifFileIn->Image.Width)
			    == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);

			if (!Complement)
			    if (EGifPutLine(GifFileOut, &Line[ImageX1],
					    ImageWidth) == GIF_ERROR)
				QuitGifError(GifFileIn, GifFileOut);

			GifQprintf("\b\b\b\b%-4d", i);
		    }

		    /* Skip lines above ImageY2: */
		    for (i = ImageY2 + 1; i < GifFileIn->Image.Height; i++) {
			if (DGifGetLine(GifFileIn, Line, GifFileIn->Image.Width)
			    == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);

			if (Complement) {
			    if (ImageX1 == ImageX2) {
				/* don't remove any vertical band */
				if (EGifPutLine(GifFileOut, Line,
						ImageWidth) == GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);
			    }
			    else
			    {
				if (EGifPutLine(GifFileOut, Line,
						ImageX1) == GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);

				if (EGifPutLine(GifFileOut,
						&Line[ImageX2 + 1],
						GifFileIn->SWidth - (ImageX2 + 1)
						) == GIF_ERROR)
				    QuitGifError(GifFileIn, GifFileOut);
			    }
			}

			GifQprintf("\b\b\b\b%-4d", i);
		    }

		    free((char *) Line);
		}
		else {
		    /* Copy the image as is (we dont modify this one): */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			GifFileIn->Image.Interlace,
			GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);

		    /* Now read image itself in decoded form as we dont      */
		    /* really care what is there, and this is much faster.   */
		    if (DGifGetCode(GifFileIn, &CodeSize, &CodeBlock) == GIF_ERROR
		     || EGifPutCode(GifFileOut, CodeSize, CodeBlock) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		    while (CodeBlock != NULL)
			if (DGifGetCodeNext(GifFileIn, &CodeBlock) == GIF_ERROR ||
			    EGifPutCodeNext(GifFileOut, CodeBlock) == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);
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

    return 0;
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

