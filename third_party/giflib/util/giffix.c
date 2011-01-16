/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to attempt and fix broken GIF images. Currently fix the following: *
* 1. EOF terminates before end of image size (adds black in the end).        *
* Options:								     *
* -q : quiet printing mode.						     *
* -h : on-line help							     *
******************************************************************************
* History:								     *
* 5 May 91 - Version 1.0 by Gershon Elber.				     *
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

#define PROGRAM_NAME	"GifFix"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifFix q%- h%- GifFile!*s";
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
	" q%- h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static int
    ImageNum = 0;

static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, Error, NumFiles, ExtCode, Row, Col, Width, Height,
	DarkestColor = 0, ColorIntens = 10000, HelpFlag = FALSE;
    GifRecordType RecordType;
    GifByteType *Extension;
    char **FileName = NULL;
    GifRowType LineBuffer;
    ColorMapObject *ColorMap;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint, &HelpFlag,
		&NumFiles, &FileName)) != FALSE ||
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

    /* Dump out exactly same screen information: */
    if (EGifPutScreenDesc(GifFileOut,
	GifFileIn->SWidth, GifFileIn->SHeight,
	GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
	GifFileIn->SColorMap) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);

    if ((LineBuffer = (GifRowType) malloc(GifFileIn->SWidth)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    /* Scan the content of the GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);

	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		if (GifFileIn->Image.Interlace)
		    GIF_EXIT("Cannt fix interlaced images.");

		Row = GifFileIn->Image.Top; /* Image Position relative to Screen. */
		Col = GifFileIn->Image.Left;
		Width = GifFileIn->Image.Width;
		Height = GifFileIn->Image.Height;
		GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ",
		    PROGRAM_NAME, ++ImageNum, Col, Row, Width, Height);

		/* Put the image descriptor to out file: */
		if (EGifPutImageDesc(GifFileOut, Col, Row, Width, Height,
		    FALSE, GifFileIn->Image.ColorMap) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);

		/* Find the darkest color in color map to use as a filler. */
		ColorMap = (GifFileIn->Image.ColorMap ? GifFileIn->Image.ColorMap :
						     GifFileIn->SColorMap);
		for (i = 0; i < ColorMap->ColorCount; i++) {
		    j = ((int) ColorMap->Colors[i].Red) * 30 +
			((int) ColorMap->Colors[i].Green) * 59 +
			((int) ColorMap->Colors[i].Blue) * 11;
		    if (j < ColorIntens) {
			ColorIntens = j;
			DarkestColor = i;
		    }
		}

		/* Load the image, and dump it. */
		for (i = 0; i < Height; i++) {
		    GifQprintf("\b\b\b\b%-4d", i);
		    if (DGifGetLine(GifFileIn, LineBuffer, Width)
			== GIF_ERROR) break;
		    if (EGifPutLine(GifFileOut, LineBuffer, Width)
			== GIF_ERROR) QuitGifError(GifFileIn, GifFileOut);
		}

		if (i < Height) {
		    fprintf(stderr, "\nFollowing error occured (and ignored):");
		    PrintGifError();

		    /* Fill in with the darkest color in color map. */
		    for (j = 0; j < Width; j++)
			LineBuffer[j] = DarkestColor;
		    for (; i < Height; i++)
			if (EGifPutLine(GifFileOut, LineBuffer, Width)
			    == GIF_ERROR) QuitGifError(GifFileIn, GifFileOut);
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
    fprintf(stderr, "\nFollowing unrecoverable error occured:");
    PrintGifError();
    if (GifFileIn != NULL) DGifCloseFile(GifFileIn);
    if (GifFileOut != NULL) EGifCloseFile(GifFileOut);
    exit(EXIT_FAILURE);
}
