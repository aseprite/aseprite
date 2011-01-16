/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to generate GIF image page from a given text by drawing the chars  *
* using 8 by 8 fixed font.						     *
* Options:								     *
* -q : quiet printing mode.						     *
* -s ColorMapSize : in bits, i.e. 6 bits for 64 colors.			     *
* -f ForeGroundIndex : by default foreground is 1. Must be in range 0..255.  *
* -c R G B : set the foregound color values. By default it is white.	     *
* -t "Text" : Make a one-line GIF (8 pixels high) from the given Text.       *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 3 May 90 - Version 1.0 by Gershon Elber.				     *
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

#define PROGRAM_NAME	"Text2Gif"

#define MAX_NUM_TEXT_LINES	100	 /* Maximum number of lines in file. */

#define LINE_LEN		256	 /* Maximum length of one text line. */

#define DEFAULT_FG_INDEX	1		   /* Text foreground index. */

#define DEFAULT_COLOR_RED	255		   /* Text foreground color. */
#define DEFAULT_COLOR_GREEN	255
#define DEFAULT_COLOR_BLUE	255

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif library module	\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "Text2Gif q%- s%-ClrMapSize!d f%-FGClr!d c%-R|G|B!d!d!d t%-\"Text\"!s h%-";
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
	" q%- s%-ClrMapSize!d f%-FGClr!d c%-R|G|B!d!d!d t%-\"Text\"!s h%-";
#endif /* SYSV */

static unsigned int
    RedColor = DEFAULT_COLOR_RED,
    GreenColor = DEFAULT_COLOR_GREEN,
    BlueColor = DEFAULT_COLOR_BLUE;

static void QuitGifError(GifFileType *GifFile);
static void GenRasterTextLine(GifRowType *RasterBuffer, char *TextLine,
					int BufferWidth, int ForeGroundIndex);

/******************************************************************************
* Interpret the command line and generate the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, l, Error, ImageWidth, ImageHeight, NumOfLines, LogNumLevels,
	NumLevels, ClrMapSizeFlag = FALSE, ColorMapSize = 1, ColorFlag = FALSE,
	ForeGroundIndex = DEFAULT_FG_INDEX, ForeGroundFlag = FALSE,
	TextLineFlag = FALSE, HelpFlag = FALSE;
    char *TextLines[MAX_NUM_TEXT_LINES], Line[LINE_LEN];
    GifRowType RasterBuffer[GIF_FONT_HEIGHT];
    ColorMapObject *ColorMap;
    GifFileType *GifFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &ClrMapSizeFlag, &ColorMapSize,
		&ForeGroundFlag, &ForeGroundIndex,
		&ColorFlag, &RedColor, &GreenColor, &BlueColor,
		&TextLineFlag, &TextLines[0],
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

    if (ForeGroundIndex > 255 || ForeGroundIndex < 1)
	GIF_EXIT("Foregound (-f) should be in the range 1..255, aborted.");

    if (ColorMapSize > 8 || ColorMapSize < 1)
	GIF_EXIT("ColorMapSize (-s) should be in the range 1..8, aborted.");

    if (TextLineFlag) {
	NumOfLines = 1;
	ImageHeight = GIF_FONT_HEIGHT;
	ImageWidth = GIF_FONT_WIDTH * strlen(TextLines[0]);
    }
    else {
	NumOfLines = l = 0;
	while (fgets(Line, LINE_LEN - 1, stdin)) {
	    for (i = strlen(Line); i > 0 && Line[i-1] <= ' '; i--);
	    Line[i] = 0;
	    if (l < i) l = i;
	    TextLines[NumOfLines++] = strdup(Line);
	    if (NumOfLines == MAX_NUM_TEXT_LINES)
		GIF_EXIT("Input file has too many lines, aborted.");
	}
	if (NumOfLines == 0)
	    GIF_EXIT("No input text, aborted.");
	ImageHeight = GIF_FONT_HEIGHT * NumOfLines;
	ImageWidth = GIF_FONT_WIDTH * l;
    }

    /* Allocate the raster buffer for GIF_FONT_HEIGHT scan lines (one text line). */
    for (i = 0; i < GIF_FONT_HEIGHT; i++)
	if ((RasterBuffer[i] = (GifRowType) malloc(sizeof(GifPixelType) *
							ImageWidth)) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");

    /* Open stdout for the output file: */
    if ((GifFile = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFile);

    /* Dump out screen description with given size and generated color map: */
    for (LogNumLevels = 1, NumLevels = 2;
	 NumLevels < ForeGroundIndex;
	 LogNumLevels++, NumLevels <<= 1);
    if (NumLevels < (1 << ColorMapSize)) {
    	NumLevels = (1 << ColorMapSize);
	LogNumLevels = ColorMapSize;
    }

    if ((ColorMap = MakeMapObject(NumLevels, NULL)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < NumLevels; i++)
	ColorMap->Colors[i].Red = ColorMap->Colors[i].Green = ColorMap->Colors[i].Blue = 0;
    ColorMap->Colors[ForeGroundIndex].Red = RedColor;
    ColorMap->Colors[ForeGroundIndex].Green = GreenColor;
    ColorMap->Colors[ForeGroundIndex].Blue = BlueColor;

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

    for (i = l = 0; i < NumOfLines; i++) {
	GenRasterTextLine(RasterBuffer, TextLines[i], ImageWidth,
							ForeGroundIndex);
	for (j = 0; j < GIF_FONT_HEIGHT; j++) {
	    if (EGifPutLine(GifFile, RasterBuffer[j], ImageWidth) == GIF_ERROR)
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
        for (j = 0; j < GIF_FONT_HEIGHT; j++) RasterBuffer[j][i] = 0;

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
