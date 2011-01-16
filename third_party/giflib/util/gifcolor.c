/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to generate a test pattern from a given color map		     *
******************************************************************************
* Options:								     *
* -q : quiet printing mode.						     *
* -b : set background color.
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 21 Sep 92 - Version 1.0 by Eric S. Raymond.				     *
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

#define PROGRAM_NAME	"GifColor"

#define LINE_LEN		40
#define IMAGEWIDTH		LINE_LEN*GIF_FONT_WIDTH

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif library module	\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifColor q%- b%-Background!d h%-";
#else
static char
    *VersionStr =
	PROGRAM_NAME
	GIF_LIB_VERSION
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = PROGRAM_NAME " q%- b%-Background!d h%-";
#endif /* SYSV */

static int BackGround = 0;
static void QuitGifError(GifFileType *GifFile);
static void GenRasterTextLine(GifRowType *RasterBuffer, char *TextLine,
					int BufferWidth, int ForeGroundIndex);

/******************************************************************************
* Interpret the command line and generate the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, l, Error, GifQuietPrint, ColorMapSize,
	BackGroundFlag = FALSE, HelpFlag = FALSE;
    char Line[LINE_LEN];
    GifRowType RasterBuffer[GIF_FONT_HEIGHT];
    ColorMapObject *ColorMap;
    GifFileType *GifFile;
    GifColorType	ScratchMap[256];
    int red, green, blue;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
			   &GifQuietPrint,
			   &BackGroundFlag, &BackGround,
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

    /* Allocate the raster buffer for GIF_FONT_HEIGHT scan lines. */
    for (i = 0; i < GIF_FONT_HEIGHT; i++)
    {
	if ((RasterBuffer[i] = (GifRowType) malloc(sizeof(GifPixelType) *
							IMAGEWIDTH)) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");
    }

    /* Open stdout for the output file: */
    if ((GifFile = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFile);

    /* Read the color map in ColorFile into this color map: */
    ColorMapSize = 0;
    while (fscanf(stdin,
		  "%*3d %3d %3d %3d\n",
		  &red, &green, &blue) == 3) {
	    ScratchMap[ColorMapSize].Red = red;
	    ScratchMap[ColorMapSize].Green = green;
	    ScratchMap[ColorMapSize].Blue = blue;
	    ColorMapSize++;
	}

    if ((ColorMap = MakeMapObject(1 << BitSize(ColorMapSize), ScratchMap)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    if (EGifPutScreenDesc(GifFile,
			  IMAGEWIDTH, ColorMapSize * GIF_FONT_HEIGHT,
			  BitSize(ColorMapSize),
			  BackGround, ColorMap) == GIF_ERROR)
	QuitGifError(GifFile);

    /* Dump out the image descriptor: */
    if (EGifPutImageDesc(GifFile,
	0, 0, IMAGEWIDTH, ColorMapSize * GIF_FONT_HEIGHT, FALSE, NULL) == GIF_ERROR)
	QuitGifError(GifFile);

    GifQprintf("\n%s: Image 1 at (%d, %d) [%dx%d]:     ",
		    PROGRAM_NAME, GifFile->Image.Left, GifFile->Image.Top,
		    GifFile->Image.Width, GifFile->Image.Height);

    for (i = l = 0; i < ColorMap->ColorCount; i++) {
	(void)sprintf(Line, "Color %-3d: [%-3d, %-3d, %-3d] ", i,
		      ColorMap->Colors[i].Red,
		      ColorMap->Colors[i].Green,
		      ColorMap->Colors[i].Blue);
	GenRasterTextLine(RasterBuffer, Line, IMAGEWIDTH, i);
	for (j = 0; j < GIF_FONT_HEIGHT; j++) {
	    if (EGifPutLine(GifFile, RasterBuffer[j], IMAGEWIDTH) == GIF_ERROR)
		QuitGifError(GifFile);
	    GifQprintf("\b\b\b\b%-4d", l++);
	}
    }

    if (EGifCloseFile(GifFile) == GIF_ERROR)
	QuitGifError(GifFile);

    return 0;
}

/******************************************************************************
* Close output file (if open), and exit.				      *
******************************************************************************/
static void GenRasterTextLine(GifRowType *RasterBuffer, char *TextLine,
					int BufferWidth, int ForeGroundIndex)
{
    unsigned char c;
    unsigned char Byte, Mask;
    int i, j, k, CharPosX, Len = strlen(TextLine);

    for (i = 0; i < BufferWidth; i++)
        for (j = 0; j < GIF_FONT_HEIGHT; j++) RasterBuffer[j][i] = BackGround;

    for (i = CharPosX = 0; i < Len; i++, CharPosX += GIF_FONT_WIDTH) {
	c = TextLine[i];
	for (j = 0; j < GIF_FONT_HEIGHT; j++) {
	    Byte = AsciiTable[(unsigned short)c][j];
	    for (k = 0, Mask = 128; k < GIF_FONT_WIDTH; k++, Mask >>= 1)
		if (Byte & Mask)
		    RasterBuffer[j][CharPosX + k] = ForeGroundIndex;
	}
    }
}

/******************************************************************************
* Close output file (if open), and exit.				      *
******************************************************************************/
static void QuitGifError(GifFileType *GifFile)
{
    PrintGifError();
    if (GifFile != NULL) EGifCloseFile(GifFile);
    exit(EXIT_FAILURE);
}
