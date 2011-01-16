/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to combine 2 GIF images into single one, using optional mask GIF   *
* file. Result colormap will be the union of the two images colormaps.	     *
* Both images should have exactly the same size, although they may be mapped *
* differently on screen. Only First GIF screen descriptor info. is used.     *
* Options:								     *
* -q : quiet printing mode.						     *
* -m mask : optional boolean image, defines where second GIF should be used. *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 12 Jul 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <stdlib.h>
#include <alloc.h>
#endif /* _MSDOS__ */

#ifndef __MSDOS__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"GifComb"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifComb q%- m%-MaskGIFFile!s h%- GifFile!*s";
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
	" q%- m%-MaskGIFFile!s h%- GifFile!*s";
#endif /* SYSV */

static int ReadUntilImage(GifFileType *GifFile);
static void QuitGifError(GifFileType *GifFileIn1, GifFileType *GifFileIn2,
			 GifFileType *GifMaskFile, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, Error, NumFiles, Size,
	MaskFlag = FALSE, HelpFlag = FALSE;
    char **FileName = NULL, *MaskFileName;
    GifPixelType ColorTransIn2[256];
    GifRowType LineIn1 = NULL, LineIn2 = NULL, LineMask = NULL, LineOut = NULL;
    ColorMapObject *ColorUnion;
    ColorMapObject *ColorIn1 = NULL, *ColorIn2 = NULL;
    GifFileType *GifFileIn1 = NULL, *GifFileIn2 = NULL, *GifMaskFile = NULL,
	*GifFileOut = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &MaskFlag, &MaskFileName,
		&HelpFlag, &NumFiles, &FileName)) != FALSE ||
		(NumFiles != 2 && !HelpFlag)) {
	if (Error)
	    GAPrintErrMsg(Error);
	else if (NumFiles != 2)
	    GIF_MESSAGE("Error in command line parsing - two GIF file please.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (HelpFlag) {
	fprintf(stderr, VersionStr);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_SUCCESS);
    }

    /* Open all input files (two GIF to combine, and optional mask): */
    if ((GifFileIn1 = DGifOpenFileName(FileName[0])) == NULL ||
	(GifFileIn2 = DGifOpenFileName(FileName[1])) == NULL ||
	(MaskFlag && (GifMaskFile = DGifOpenFileName(MaskFileName)) == NULL))
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);

    if (ReadUntilImage(GifFileIn1) == GIF_ERROR ||
	ReadUntilImage(GifFileIn2) == GIF_ERROR ||
	(MaskFlag && ReadUntilImage(GifMaskFile) == GIF_ERROR))
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);

    if (GifFileIn1->Image.Width != GifFileIn2->Image.Width ||
	GifFileIn2->Image.Height != GifFileIn2->Image.Height ||
	(MaskFlag && (GifFileIn1->Image.Width != GifMaskFile->Image.Width ||
		      GifFileIn1->Image.Height != GifMaskFile->Image.Height)))
	GIF_EXIT("Given GIF files have different image dimensions.");

    /* Open stdout for the output file: */
    if ((GifFileOut = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);

    Size = sizeof(GifPixelType) * GifFileIn1->Image.Width;
    if ((LineIn1 = (GifRowType) malloc(Size)) == NULL ||
	(LineIn2 = (GifRowType) malloc(Size)) == NULL ||
	(MaskFlag && (LineMask = (GifRowType) malloc(Size)) == NULL) ||
	(LineOut = (GifRowType) malloc(Size)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    if (GifFileIn1->Image.ColorMap) {
	ColorIn1 = GifFileIn1->Image.ColorMap;
    }
    else if (GifFileIn1->SColorMap) {
	ColorIn1 = GifFileIn1->SColorMap;
    }
    else
	GIF_EXIT("Neither Screen nor Image color map exists - GIF file 1.");

    if (GifFileIn2->Image.ColorMap) {
	ColorIn2 = GifFileIn2->Image.ColorMap;
    }
    else if (GifFileIn2->SColorMap) {
	ColorIn2 = GifFileIn2->SColorMap;
    }
    else
	GIF_EXIT("Neither Screen nor Image color map exists - GIF file 2.");

    /* Create union of the two given color maps. ColorIn1 will be copied as  */
    /* is while ColorIn2 will be mapped using ColorTransIn2 table.	     */
    /* ColorUnion is allocated by the procedure itself.			     */
    if ((ColorUnion = UnionColorMap(ColorIn1, ColorIn2, ColorTransIn2))==NULL)
	GIF_EXIT("Unioned color map is too big (>256 colors).");

    /* Dump out new image and screen descriptors: */
    if (EGifPutScreenDesc(GifFileOut,
	GifFileIn1->SWidth, GifFileIn1->SHeight,
	ColorUnion->BitsPerPixel, GifFileIn1->SBackGroundColor,
	ColorUnion) == GIF_ERROR)
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);

    if (EGifPutImageDesc(GifFileOut,
	GifFileIn1->Image.Left, GifFileIn1->Image.Top,
	GifFileIn1->Image.Width, GifFileIn1->Image.Height,
	GifFileIn1->Image.Interlace, NULL) == GIF_ERROR)
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);


    /* Time to do it: read 2 scan lines from 2 files (and optionally from    */
    /* the mask file, merge them and them result out. Do it Height times:    */
    GifQprintf("\n%s: Image 1 at (%d, %d) [%dx%d]:     ",
	PROGRAM_NAME, GifFileOut->Image.Left, GifFileOut->Image.Top,
				GifFileOut->Image.Width, GifFileOut->Image.Height);
    for (i = 0; i < GifFileIn1->Image.Height; i++) {
	if (DGifGetLine(GifFileIn1, LineIn1, GifFileIn1->Image.Width) == GIF_ERROR ||
	    DGifGetLine(GifFileIn2, LineIn2, GifFileIn2->Image.Width) == GIF_ERROR ||
	    (MaskFlag &&
	     DGifGetLine(GifMaskFile, LineMask, GifMaskFile->Image.Width)
								== GIF_ERROR))
	    QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);
	if (MaskFlag) {
	    /* Every time Mask has non background color, use LineIn1 pixel,  */
	    /* otherwise use LineIn2 pixel instead.			     */
	    for (j = 0; j < GifFileIn1->Image.Width; j++) {
		if (LineMask[j] != GifMaskFile->SBackGroundColor)
		    LineOut[j] = LineIn1[j];
		else
		    LineOut[j] = ColorTransIn2[LineIn2[j]];
	    }
	}
	else {
	    /* Every time Color of Image 1 is equal to background - take it  */
	    /* From Image 2 instead of the background.			     */
	    for (j = 0; j < GifFileIn1->Image.Width; j++) {
		if (LineIn1[j] != GifFileIn1->SBackGroundColor)
		    LineOut[j] = LineIn1[j];
		else
		    LineOut[j] = ColorTransIn2[LineIn2[j]];
	    }
	}
	if (EGifPutLine(GifFileOut, LineOut, GifFileOut->Image.Width)
								== GIF_ERROR)
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);
	GifQprintf("\b\b\b\b%-4d", i);
    }

    FreeMapObject(ColorUnion);		    /* We dont need this any more... */
    ColorUnion = NULL;

    if (DGifCloseFile(GifFileIn1) == GIF_ERROR ||
	DGifCloseFile(GifFileIn2) == GIF_ERROR ||
	EGifCloseFile(GifFileOut) == GIF_ERROR ||
	(MaskFlag && DGifCloseFile(GifMaskFile) == GIF_ERROR))
	QuitGifError(GifFileIn1, GifFileIn2, GifMaskFile, GifFileOut);

    return 0;
}

/******************************************************************************
* Read until first image in GIF file is detected and read its descriptor.     *
******************************************************************************/
static int ReadUntilImage(GifFileType *GifFile)
{
    int ExtCode;
    GifRecordType RecordType;
    GifByteType *Extension;

    /* Scan the content of the GIF file, until image descriptor is detected: */
    do {
	if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
	    return GIF_ERROR;

	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		return DGifGetImageDesc(GifFile);
	    case EXTENSION_RECORD_TYPE:
		/* Skip any extension blocks in file: */
		if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR)
		    return GIF_ERROR;

		while (Extension != NULL)
		    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR)
			return GIF_ERROR;
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		    /* Should be traps by DGifGetRecordType. */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE);

    return GIF_ERROR;		  /* We should be here - no image was found! */
}

/******************************************************************************
* Close both input and output file (if open), and exit.			      *
******************************************************************************/
static void QuitGifError(GifFileType *GifFileIn1, GifFileType *GifFileIn2,
			 GifFileType *GifMaskFile, GifFileType *GifFileOut)
{
    PrintGifError();
    if (GifFileIn1 != NULL) DGifCloseFile(GifFileIn1);
    if (GifFileIn2 != NULL) DGifCloseFile(GifFileIn2);
    if (GifMaskFile != NULL) DGifCloseFile(GifMaskFile);
    if (GifFileOut != NULL) EGifCloseFile(GifFileOut);
    exit(EXIT_FAILURE);
}
