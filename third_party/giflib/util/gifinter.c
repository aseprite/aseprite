/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to flip interlaced and non interlaced images in GIF files.	     *
* Options:								     *
* -q : quiet printing mode.						     *
* -i : Force all images to be intelaced.				     *
* -s : Force all images to be sequencial (non interlaced). This is default.  *
* -h : on-line help							     *
******************************************************************************
* History:								     *
* 5 Jul 89 - Version 1.0 by Gershon Elber.				     *
* 21 Dec 89 - Fix problems with -i and -s flags (Version 1.1).               *
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

#define PROGRAM_NAME	"GifInter"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifInter q%- i%- s%- h%- GifFile!*s";
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
	" q%- i%- s%- h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static int
    ImageNum = 0,
    SequencialFlag = FALSE,
    InterlacedFlag = FALSE,
    HelpFlag = FALSE,
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */

static int LoadImage(GifFileType *GifFile, GifRowType **ImageBuffer);
static int DumpImage(GifFileType *GifFile, GifRowType *ImageBuffer);
static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	Error, NumFiles, ExtCode;
    GifRecordType RecordType;
    GifByteType *Extension;
    char **FileName = NULL;
    GifRowType *ImageBuffer;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint,
		&InterlacedFlag, &SequencialFlag, &HelpFlag,
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

		/* Put the image descriptor to out file: */
		if (EGifPutImageDesc(GifFileOut,
		    GifFileIn->Image.Left, GifFileIn->Image.Top,
		    GifFileIn->Image.Width, GifFileIn->Image.Height,
		    InterlacedFlag,
		    GifFileIn->Image.ColorMap) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);

		/* Load the image (either Interlaced or not), and dump it as */
		/* defined in GifFileOut->Image.Interlaced.		     */
		if (LoadImage(GifFileIn, &ImageBuffer) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		if (DumpImage(GifFileOut, ImageBuffer) == GIF_ERROR)
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
* Routine to read Image out. The image can be Interlaced or None interlaced.  *
* The memory required to hold the image is allocate by the routine itself.    *
* The image is always loaded sequencially into the buffer.		      *
* Return GIF_OK if succesful, GIF_ERROR otherwise.			      *
******************************************************************************/
static int LoadImage(GifFileType *GifFile, GifRowType **ImageBufferPtr)
{
    int Size, i, j, Count;
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
    if (GifFile->Image.Interlace) {
	/* Need to perform 4 passes on the images: */
	for (Count = i = 0; i < 4; i++)
	    for (j = InterlacedOffset[i]; j < GifFile->Image.Height;
						 j += InterlacedJumps[i]) {
		GifQprintf("\b\b\b\b%-4d", Count++);
		if (DGifGetLine(GifFile, ImageBuffer[j], GifFile->Image.Width)
		    == GIF_ERROR) return GIF_ERROR;
	    }
    }
    else {
	for (i = 0; i < GifFile->Image.Height; i++) {
	    GifQprintf("\b\b\b\b%-4d", i);
	    if (DGifGetLine(GifFile, ImageBuffer[i], GifFile->Image.Width)
		== GIF_ERROR) return GIF_ERROR;
	}
    }

    return GIF_OK;
}

/******************************************************************************
* Routine to dump image out. The given Image buffer should always hold the    *
* image sequencially. Image will be dumped according to IInterlaced flag in   *
* GifFile structure. Once dumped, the memory holding the image is freed.      *
* Return GIF_OK if succesful, GIF_ERROR otherwise.			      *
******************************************************************************/
static int DumpImage(GifFileType *GifFile, GifRowType *ImageBuffer)
{
    int i, j, Count;

    if (GifFile->Image.Interlace) {
	/* Need to perform 4 passes on the images: */
	for (Count = GifFile->Image.Height, i = 0; i < 4; i++)
	    for (j = InterlacedOffset[i]; j < GifFile->Image.Height;
						 j += InterlacedJumps[i]) {
		GifQprintf("\b\b\b\b%-4d", Count--);
		if (EGifPutLine(GifFile, ImageBuffer[j], GifFile->Image.Width)
		    == GIF_ERROR) return GIF_ERROR;
	    }
    }
    else {
	for (Count = GifFile->Image.Height, i = 0; i < GifFile->Image.Height; i++) {
	    GifQprintf("\b\b\b\b%-4d", Count--);
	    if (EGifPutLine(GifFile, ImageBuffer[i], GifFile->Image.Width)
		== GIF_ERROR) return GIF_ERROR;
	}
    }

    /* Free the m emory used for this image: */
    for (i = 0; i < GifFile->Image.Height; i++) free((char *) ImageBuffer[i]);
    free((char *) ImageBuffer);

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
