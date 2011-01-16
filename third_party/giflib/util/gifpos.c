/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to reposition GIF file, by changing screen size and/or image(s)    *
* position. Neither image(s) not colormap(s) are changed by any mean.	     *
* Options:								     *
* -q : quiet printing mode.						     *
* -s w h : set new screen size - width & height.			     *
* -i t l : set new image position - top & left (for first image).	     *
* -n n w h : set new image position - top & left (for image number n).	     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 6 Jul 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <alloc.h>
#endif /* __MSDOS__ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"GifPos"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifPos q%- s%-Width|Height!d!d i%-Left|Top!d!d n%-n|Left|Top!d!d!d h%- GifFile!*s";
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
	" q%- s%-Width|Height!d!d i%-Left|Top!d!d n%-n|Left|Top!d!d!d h%- GifFile!*s";
#endif /* SYSV */

static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	Error, NumFiles, ExtCode, CodeSize, ImageNum = 0,
	ScreenFlag = FALSE, ScreenWidth, ScreenHeight,
	ImageFlag = FALSE, ImageLeft, ImageTop,
	ImageNFlag = FALSE, ImageN, ImageNLeft, ImageNTop,
	HelpFlag = FALSE;
    GifRecordType RecordType;
    GifByteType *Extension, *CodeBlock;
    char **FileName = NULL;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint,
		&ScreenFlag, &ScreenWidth, &ScreenHeight,
		&ImageFlag, &ImageLeft, &ImageTop,
		&ImageNFlag, &ImageN, &ImageNLeft, &ImageNTop,
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

    /* And dump out its new possible repositioned screen information: */
    if (EGifPutScreenDesc(GifFileOut,
	ScreenFlag ? ScreenWidth : GifFileIn->SWidth,
	ScreenFlag ? ScreenHeight : GifFileIn->SHeight,
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
		/* Put possibly repositioned image descriptor to out file: */
		if (++ImageNum == 1 && ImageFlag) {
		    /* First image should be repositioned: */
		    if (EGifPutImageDesc(GifFileOut,
			ImageLeft, ImageTop,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			GifFileIn->Image.Interlace,
			GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		}
		else if (ImageNum == ImageN && ImageNFlag) {
		    /* Image N should be repositioned: */
		    if (EGifPutImageDesc(GifFileOut,
			ImageNLeft, ImageNTop,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			GifFileIn->Image.Interlace,
			GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		}
		else {
		    /* No repositioning for this image: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			GifFileIn->Image.Interlace,
			GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		}

		/* Now read image itself in decoded form as we dont really   */
		/* care what we have there, and this is much faster.	     */
		if (DGifGetCode(GifFileIn, &CodeSize, &CodeBlock) == GIF_ERROR ||
		    EGifPutCode(GifFileOut, CodeSize, CodeBlock) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		while (CodeBlock != NULL) {
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
