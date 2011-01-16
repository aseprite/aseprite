/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to rotate image 90 degrees to the right/left or flip the image     *
* horizintally/vertically (mirror).					     *
* Options:								     *
* -q : quiet printing mode.						     *
* -r : rotate 90 degrees to the right (default).			     *
* -l : rotate 90 degrees to the left.					     *
* -x : Mirror the image horizontally (first line switch places with last).   *
* -y : Mirror the image vertically (first column switch places with last).   *
* -h : on-line help							     *
******************************************************************************
* History:								     *
* 10 Jul 89 - Version 1.0 by Gershon Elber.				     *
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

#define PROGRAM_NAME	"GifFlip"

#define FLIP_NONE	0
#define FLIP_RIGHT	1
#define FLIP_LEFT	2
#define FLIP_HORIZ	3
#define FLIP_VERT	4

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifFlip q%- r%- l%- x%- y%- h%- GifFile!*s";
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
	" q%- r%- l%- x%- y%- h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static int
    ImageNum = 0;

static int LoadImage(GifFileType *GifFile, GifRowType **ImageBuffer);
static int DumpImage(GifFileType *GifFile, GifRowType *ImageBuffer,
				int Width, int Height, int FlipDirection);
static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, Error, NumFiles, ExtCode, FlipDirection = FLIP_RIGHT,
	RightFlag = FALSE, LeftFlag = FALSE,
	HorizFlag = FALSE, VertFlag = FALSE, HelpFlag = FALSE;
    GifRecordType RecordType;
    GifByteType *Extension;
    char **FileName = NULL;
    GifRowType *ImageBuffer;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint,
		&RightFlag, &LeftFlag, &HorizFlag, &VertFlag, &HelpFlag,
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

    if ((i = (RightFlag != 0) + (LeftFlag != 0) +
	     (HorizFlag != 0) + (VertFlag != 0)) > 1)
	GIF_EXIT("Only one of -r, -l, -x, -y please.");
    if (i == 0)
	FlipDirection = FLIP_RIGHT;		     /* Make it the default. */
    else {
	if (RightFlag) FlipDirection = FLIP_RIGHT;
	if (LeftFlag) FlipDirection = FLIP_LEFT;
	if (HorizFlag) FlipDirection = FLIP_HORIZ;
	if (VertFlag) FlipDirection = FLIP_VERT;
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

    if (RightFlag || LeftFlag) {
	/* Dump out same screen information, but flip Screen Width/Height: */
	if (EGifPutScreenDesc(GifFileOut,
	    GifFileIn->SHeight, GifFileIn->SWidth,
	    GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
	    GifFileIn->SColorMap) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);
    }
    else {
	/* Dump out exactly same screen information: */
	if (EGifPutScreenDesc(GifFileOut,
	    GifFileIn->SWidth, GifFileIn->SHeight,
	    GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
	    GifFileIn->SColorMap) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);
    }

    /* Scan the content of the GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);

	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		if (GifFileIn->Image.Interlace)
		    GIF_EXIT("Cannt flip interlaced images - use GifInter first.");

		/* Put the image descriptor to out file: */
		if (RightFlag) {
		    /* Rotate to the right: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->SHeight - GifFileIn->Image.Height -
						GifFileIn->Image.Top,
			GifFileIn->Image.Left,
			GifFileIn->Image.Height, GifFileIn->Image.Width,
			FALSE, GifFileIn->Image.ColorMap) == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);
		}
		else if (LeftFlag) {
		    /* Rotate to the left: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Top,
			GifFileIn->SWidth - GifFileIn->Image.Width -
						GifFileIn->Image.Left,
			GifFileIn->Image.Height, GifFileIn->Image.Width,
			FALSE, GifFileIn->Image.ColorMap) == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);
		}
		else {
		    /* No rotation - only flipping vert. or horiz.: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			FALSE, GifFileIn->Image.ColorMap) == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);
		}

		/* Load the image (either Interlaced or not), and dump it    */
		/* fliped as requrested by Flags:			     */
		if (LoadImage(GifFileIn, &ImageBuffer) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		if (DumpImage(GifFileOut, ImageBuffer, GifFileIn->Image.Width,
				GifFileIn->Image.Height, FlipDirection) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
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
* Routine to read Image out. The image can be Non interlaced only.	      *
* The memory required to hold the image is allocate by the routine itself.    *
* Return GIF_OK if succesful, GIF_ERROR otherwise.			      *
******************************************************************************/
static int LoadImage(GifFileType *GifFile, GifRowType **ImageBufferPtr)
{
    int Size, i;
    GifRowType *ImageBuffer;

    /* Allocate the image as vector of column of rows. We cannt allocate     */
    /* the all screen at once, as this broken minded CPU can allocate up to  */
    /* 64k at a time and our image can be bigger than that:		     */
    if ((ImageBuffer = (GifRowType *)
	malloc(GifFile->Image.Height * sizeof(GifRowType *))) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");

    Size = GifFile->Image.Width * sizeof(GifPixelType);/* One row size in bytes.*/
    for (i = 0; i < GifFile->Image.Height; i++) {
	/* Allocate the rows: */
	if ((ImageBuffer[i] = (GifRowType) malloc(Size)) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");
    }

    *ImageBufferPtr = ImageBuffer;

    GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ",
	PROGRAM_NAME, ++ImageNum, GifFile->Image.Left, GifFile->Image.Top,
				 GifFile->Image.Width, GifFile->Image.Height);

    for (i = 0; i < GifFile->Image.Height; i++) {
	GifQprintf("\b\b\b\b%-4d", i);
	if (DGifGetLine(GifFile, ImageBuffer[i], GifFile->Image.Width)
		== GIF_ERROR) return GIF_ERROR;
    }

    return GIF_OK;
}

/******************************************************************************
* Routine to dump image out. The given Image buffer should always hold the    *
* image sequencially, and Width & Height hold image dimensions BEFORE flip.   *
* Image will be dumped according to FlipDirection.			      *
* Once dumped, the memory holding the image is freed.			      *
* Return GIF_OK if succesful, GIF_ERROR otherwise.			      *
******************************************************************************/
static int DumpImage(GifFileType *GifFile, GifRowType *ImageBuffer,
				int Width, int Height, int FlipDirection)
{
    int i, j, Count;
    GifRowType Line;			   /* New scan line is copied to it. */

    /* Allocate scan line that will fit both image width and height: */
    if ((Line = (GifRowType) malloc((Width > Height ? Width : Height)
						* sizeof(GifPixelType))) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    switch (FlipDirection) {
	case FLIP_RIGHT:
	    for (Count = Width, i = 0; i < Width; i++) {
		GifQprintf("\b\b\b\b%-4d", Count--);
		for (j = 0; j < Height; j++)
		    Line[j] = ImageBuffer[Height - j - 1][i];
		if (EGifPutLine(GifFile, Line, Height) == GIF_ERROR)
	    	    return GIF_ERROR;
	    }
	    break;
	case FLIP_LEFT:
	    for (i = Width - 1; i >= 0; i--) {
		GifQprintf("\b\b\b\b%-4d", i + 1);
		for (j = 0; j < Height; j++)
		    Line[j] = ImageBuffer[j][i];
		if (EGifPutLine(GifFile, Line, Height) == GIF_ERROR)
	    	    return GIF_ERROR;
	    }
	    break;
	case FLIP_HORIZ:
	    for (i = Height - 1; i >= 0; i--) {
		GifQprintf("\b\b\b\b%-4d", i);
		if (EGifPutLine(GifFile, ImageBuffer[i], Width) == GIF_ERROR)
		    return GIF_ERROR;
	    }
	    break;
	case FLIP_VERT:
	    for (Count = Height, i = 0; i < Height; i++) {
		GifQprintf("\b\b\b\b%-4d", Count--);
		for (j = 0; j < Width; j++)
		    Line[j] = ImageBuffer[i][Width - j - 1];
		if (EGifPutLine(GifFile, Line, Width) == GIF_ERROR)
	    	    return GIF_ERROR;
	    }
	    break;
    }

    /* Free the memory used for this image, and the temporary scan line: */
    for (i = 0; i < Height; i++) free((char *) ImageBuffer[i]);
    free((char *) ImageBuffer);

    free((char *) Line);

    return GIF_OK;
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
