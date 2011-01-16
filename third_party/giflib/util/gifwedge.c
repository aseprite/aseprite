/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to create a test image of White and Red/Green/Blue levels for      *
* test purposes. The Primary (RGB) and Secondary (YCM) are also displayed.   *
* background.								     *
* Options:								     *
* -q : quiet printing mode.						     *
* -s Width Height : set image size.					     *
* -l levels : number of color levels.					     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 4 Jan 90 - Version 1.0 by Gershon Elber.				     *
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

#define PROGRAM_NAME	"GifWedge"

#define DEFAULT_WIDTH	640
#define DEFAULT_HEIGHT	350

#define DEFAULT_NUM_LEVELS	16     /* Number of colors to gen the image. */

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifWedge q%- l%-#Lvls!d s%-Width|Height!d!d h%-";
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
	" q%- l%-#Lvls!d s%-Width|Height!d!d h%-";
#endif /* SYSV */

static int
    NumLevels = DEFAULT_NUM_LEVELS,
    ImageWidth = DEFAULT_WIDTH,
    ImageHeight = DEFAULT_HEIGHT;

static void QuitGifError(GifFileType *GifFile);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, l, c, Error, LevelStep, LogNumLevels,
	Count = 0, LevelsFlag = FALSE, SizeFlag = FALSE, HelpFlag = FALSE;
    GifRowType Line;
    ColorMapObject *ColorMap;
    GifFileType *GifFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &LevelsFlag, &NumLevels,
		&SizeFlag, &ImageWidth, &ImageHeight,
		&HelpFlag)) != FALSE) {
	GAPrintErrMsg(Error);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (HelpFlag) {
	fprintf(stderr, VersionStr);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_SUCCESS);
    }

    /* Make sure the number of levels is power of 2 (up to 32 levels.). */
    for (i = 1; i < 6; i++) if (NumLevels == (1 << i)) break;
    if (i == 6) GIF_EXIT("#Lvls (-l option) is not power of 2 up to 32.");
    LogNumLevels = i + 3;		       /* Multiple by 8 (see below). */
    LevelStep = 256 / NumLevels;

    /* Make sure the image dimension is a multiple of NumLevels horizontally */
    /* and 7 (White, Red, Green, Blue and Yellow Cyan Magenta) vertically.   */
    ImageWidth = (ImageWidth / NumLevels) * NumLevels;
    ImageHeight = (ImageHeight / 7) * 7;

    /* Open stdout for the output file: */
    if ((GifFile = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFile);

    /* Dump out screen description with given size and generated color map:  */
    /* The color map has 7 NumLevels colors for White, Red, Green and then   */
    /* The secondary colors Yellow Cyan and magenta.			     */
    if ((ColorMap = MakeMapObject(8 * NumLevels, NULL)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < 8; i++)				   /* Set color map. */
	for (j = 0; j < NumLevels; j++) {
	    l = LevelStep * j;
	    c = i * NumLevels + j;
	    ColorMap->Colors[c].Red = (i == 0 || i == 1 || i == 4 || i == 6) * l;
	    ColorMap->Colors[c].Green =	(i == 0 || i == 2 || i == 4 || i == 5) * l;
	    ColorMap->Colors[c].Blue = (i == 0 || i == 3 || i == 5 || i == 6) * l;
	}

    if (EGifPutScreenDesc(GifFile,
	ImageWidth, ImageHeight, LogNumLevels, 0, ColorMap)
	== GIF_ERROR)
	QuitGifError(GifFile);

    /* Dump out the image descriptor: */
    if (EGifPutImageDesc(GifFile,
	0, 0, ImageWidth, ImageHeight, FALSE, NULL) == GIF_ERROR)
	QuitGifError(GifFile);

    GifQprintf("\n%s: Image 1 at (%d, %d) [%dx%d]:     ",
		    PROGRAM_NAME, GifFile->Image.Left, GifFile->Image.Top,
		    GifFile->Image.Width, GifFile->Image.Height);

    /* Allocate one scan line to be used for all image.			     */
    if ((Line = (GifRowType) malloc(sizeof(GifPixelType) * ImageWidth)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    /* Dump the pixels: */
    for (c = 0; c < 7; c++) {
	for (i = 0, l = 0; i < NumLevels; i++)
	    for (j = 0; j < ImageWidth / NumLevels; j++)
		Line[l++] = i + NumLevels * c;
	for (i = 0; i < ImageHeight / 7; i++) {
	    if (EGifPutLine(GifFile, Line, ImageWidth) == GIF_ERROR)
		QuitGifError(GifFile);
	    GifQprintf("\b\b\b\b%-4d", Count++);
	}
    }

    if (EGifCloseFile(GifFile) == GIF_ERROR)
	QuitGifError(GifFile);

    return 0;
}

/******************************************************************************
* Close output file (if open), and exit.				      *
******************************************************************************/
static void QuitGifError(GifFileType *GifFile)
{
    PrintGifError();
    if (GifFile != NULL) DGifCloseFile(GifFile);
    exit(EXIT_FAILURE);
}
