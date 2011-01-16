/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to generate back ground image that can be used to replace constant *
* background.								     *
* Options:								     *
* -q : quiet printing mode.						     *
* -d direction : set direction image should increase intensity.		     *
* -l levels : number of color levels.					     *
* -c r g b : colors of the back ground.					     *
* -m min : minimin intensity in percent.				     *
* -M max : maximum intensity in percent.				     *
* -s width height : size of image to create.				     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 9 Jul 89 - Version 1.0 by Gershon Elber.				     *
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

#define PROGRAM_NAME	"GifBG"

#define DEFAULT_WIDTH	640
#define DEFAULT_HEIGHT	350

#define DEFAULT_COLOR_RED	0
#define DEFAULT_COLOR_GREEN	0
#define DEFAULT_COLOR_BLUE	255

#define DEFAULT_MIN_INTENSITY	10			      /* In percent. */
#define DEFAULT_MAX_INTENSITY	100

#define DEFAULT_NUM_LEVELS	16     /* Number of colors to gen the image. */

#define DIR_NONE	0	     /* Direction the levels can be changed: */
#define DIR_TOP		1
#define DIR_TOP_RIGHT	2
#define DIR_RIGHT	3
#define DIR_BOT_RIGHT	4
#define DIR_BOT		5
#define DIR_BOT_LEFT	6
#define DIR_LEFT	7
#define DIR_TOP_LEFT	8

#define DEFAULT_DIR	"T"			   /* TOP (North) direction. */

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif library module	\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifBG q%- d%-Dir!s l%-#Lvls!d c%-R|G|B!d!d!d m%-MinI!d M%-MaxI!d s%-W|H!d!d h%-";
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
	" q%- d%-Dir!s l%-#Lvls!d c%-R|G|B!d!d!d m%-MinI!d M%-MaxI!d s%-W|H!d!d h%-";
#endif /* SYSV */

static int
    MaximumIntensity = DEFAULT_MAX_INTENSITY,		      /* In percent. */
    MinimumIntensity = DEFAULT_MIN_INTENSITY,
    NumLevels = DEFAULT_NUM_LEVELS,
    ImageWidth = DEFAULT_WIDTH,
    ImageHeight = DEFAULT_HEIGHT,
    Direction;
static unsigned int
    RedColor = DEFAULT_COLOR_RED,
    GreenColor = DEFAULT_COLOR_GREEN,
    BlueColor = DEFAULT_COLOR_BLUE;

static void QuitGifError(GifFileType *GifFile);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    unsigned int Ratio;
    int	i, j, l, LevelHeight, LevelWidth, Error, LogNumLevels, FlipDir,
	Accumulator, StartX, StepX, Count = 0, DoAllMaximum = FALSE,
	DirectionFlag = FALSE, LevelsFlag = FALSE, ColorFlag = FALSE,
	MinFlag = FALSE, MaxFlag = FALSE, SizeFlag = FALSE, HelpFlag = FALSE;
    GifPixelType Color;
    char *DirectionStr = DEFAULT_DIR;
    GifRowType Line;
    ColorMapObject *ColorMap;
    GifFileType *GifFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint,
		&DirectionFlag, &DirectionStr, &LevelsFlag, &NumLevels,
		&ColorFlag, &RedColor, &GreenColor, &BlueColor,
		&MinFlag, &MinimumIntensity, &MaxFlag, &MaximumIntensity,
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

    /* Make sure intensities are in the right range: */
    if (MinimumIntensity < 0 || MinimumIntensity > 100 ||
	MaximumIntensity < 0 || MaximumIntensity > 100)
	GIF_EXIT("Intensities (-m or -M options) are not in [0..100] range (percent).");

    /* Convert DirectionStr to our local representation: */
    Direction = DIR_NONE;
    FlipDir = FALSE;
    for (i = 0; i < (int)strlen(DirectionStr);  i++) /* Make sure its upper case. */
	if (islower(DirectionStr[i]))
	    DirectionStr[i] = toupper(DirectionStr[i]);

    switch(DirectionStr[0]) {
	case 'T': /* Top or North */
	case 'N':
	    if (strlen(DirectionStr) < 2)
		Direction = DIR_TOP;
	    else
		switch(DirectionStr[1]) {
		    case 'R':
		    case 'E':
			Direction = DIR_TOP_RIGHT;
			break;
		    case 'L':
		    case 'W':
			Direction = DIR_TOP_LEFT;
			FlipDir = TRUE;
			break;
		}
	    break;
	case 'R': /* Right or East */
	case 'E':
	    Direction = DIR_RIGHT;
	    break;
	case 'B': /* Bottom or South */
	case 'S':
	    if (strlen(DirectionStr) < 2) {
		Direction = DIR_BOT;
		FlipDir = TRUE;
	    }
	    else
		switch(DirectionStr[1]) {
		    case 'R':
		    case 'E':
			Direction = DIR_BOT_RIGHT;
			break;
		    case 'L':
		    case 'W':
			Direction = DIR_BOT_LEFT;
			FlipDir = TRUE;
			break;
		}
	    break;
	case 'L': /* Left or West */
	case 'W':
	    Direction = DIR_LEFT;
	    FlipDir = TRUE;
	    break;
    }
    if (Direction == DIR_NONE)
	GIF_EXIT("Direction requested (-d option) is wierd!");

    /* We are going to handle only TOP, TOP_RIGHT, RIGHT, BOT_RIGHT  so flip */
    /* the complement cases (TOP <-> BOT for example) by flipping the	     */
    /* Color i with color (NumLevels - i - 1).				     */
    if (FlipDir) {
	switch (Direction) {
	    case DIR_BOT:
		Direction = DIR_TOP;
		break;
	    case DIR_BOT_LEFT:
		Direction = DIR_TOP_RIGHT;
		break;
	    case DIR_LEFT:
		Direction = DIR_RIGHT;
		break;
	    case DIR_TOP_LEFT:
		Direction = DIR_BOT_RIGHT;
		break;
	}
    }

    /* If binary mask is requested (special case): */
    if (MinimumIntensity == 100 && MaximumIntensity == 100 && NumLevels == 2) {
	MinimumIntensity = 0;
	DoAllMaximum = TRUE;
	Direction = DIR_RIGHT;
    }

    /* Make sure colors are in the right range: */
    if (RedColor > 255 || GreenColor > 255 || BlueColor > 255)
	GIF_EXIT("Colors are not in the ragne [0..255].");

    /* Make sure number of levels is power of 2 (up to 8 bits per pixel).    */
    for (i = 1; i < 8; i++) if (NumLevels == (1 << i)) break;
    if (i == 8) GIF_EXIT("#Lvls (-l option) is not power of 2.");
    LogNumLevels = i;

    /* Open stdout for the output file: */
    if ((GifFile = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFile);

    /* Dump out screen description with given size and generated color map:  */
    if ((ColorMap = MakeMapObject(NumLevels, NULL)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 1; i <= NumLevels; i++) {
	/* Ratio will be in the range of 0..100 for required intensity: */
	Ratio = (MaximumIntensity * (i * (256 / NumLevels)) +
		 MinimumIntensity * ((NumLevels - i) * (256 / NumLevels))) /
		 256;
	ColorMap->Colors[i-1].Red   = (RedColor * Ratio) / 100;
	ColorMap->Colors[i-1].Green = (GreenColor * Ratio) / 100;
	ColorMap->Colors[i-1].Blue  = (BlueColor * Ratio) / 100;
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

    /* Allocate one scan line twice as big as image is as we are going to    */
    /* shift along it, while we dump the scan lines:			     */
    if ((Line = (GifRowType) malloc(sizeof(GifPixelType) * ImageWidth * 2)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    if (Direction == DIR_TOP) {
	/* We must evaluate the line each time level is changing: */
	LevelHeight = ImageHeight / NumLevels;
	for (Color = NumLevels, i = l = 0; i < ImageHeight; i++) {
	    if (i == l) {
		/* Time to update the line to next color level: */
		if (Color != 0) Color--;
		for (j = 0; j < ImageWidth; j++)
		    Line[j] = (FlipDir ? NumLevels - Color - 1 : Color);
		l += LevelHeight;
	    }
	    if (EGifPutLine(GifFile, Line, ImageWidth) == GIF_ERROR)
		QuitGifError(GifFile);
	    GifQprintf("\b\b\b\b%-4d", Count++);
	}
    }
    else if (Direction == DIR_RIGHT) {
	/* We pre-prepare the scan lines as going from color zero to maximum */
	/* color and dump the same scan line Height times:		     */
	/* Note this case should handle the Boolean Mask special case.	     */
	LevelWidth = ImageWidth / NumLevels;
	if (DoAllMaximum) {
	    /* Special case - do all in maximum color: */
	    for (i = 0; i < ImageWidth; i++) Line[i] = 1;
	}
	else {
	    for (Color = i = 0, l = LevelWidth; i < ImageWidth; i++, l--) {
		if (l == 0) {
		    l = LevelWidth;
		    if (Color < NumLevels - 1) Color++;
		}
		Line[i] = (FlipDir ? NumLevels - Color - 1 : Color);
	    }
	}

	for (i = 0; i < ImageHeight; i++) {
	    if (EGifPutLine(GifFile, Line, ImageWidth) == GIF_ERROR)
		QuitGifError(GifFile);
	    GifQprintf("\b\b\b\b%-4d", Count++);
	}
    }
    else {
	/* We are in one of the TOP_RIGHT, BOT_RIGHT cases: we will          */
	/* initialize the Line with its double ImageWidth length from the    */
	/* minimum intensity to the maximum intensity and shift along it     */
	/* while we go along the image height.				     */
	LevelWidth = ImageWidth * 2 / NumLevels;
	for (Color = i = 0, l = LevelWidth; i < ImageWidth * 2; i++, l--) {
	    if (l == 0) {
		l = LevelWidth;
		if (Color < NumLevels - 1) Color++;
	    }
	    Line[i] = (FlipDir ? NumLevels - Color - 1 : Color);
	}
	/* We need to implement a DDA to know how much to shift Line while   */
	/* we go down along image height. we set the parameters for it now:  */
	Accumulator = 0;
	switch(Direction) {
	    case DIR_TOP_RIGHT:
		StartX = ImageWidth;
		StepX = -1;
		break;
	    case DIR_BOT_RIGHT:
	    default:
		StartX = 0;
		StepX = 1;
		break;
	}

	/* Time to dump information out: */
	for (i = 0; i < ImageHeight; i++) {
	    if (EGifPutLine(GifFile, &Line[StartX], ImageWidth) == GIF_ERROR)
		QuitGifError(GifFile);
	    GifQprintf("\b\b\b\b%-4d", Count++);
	    if ((Accumulator += ImageWidth) > ImageHeight) {
		while (Accumulator > ImageHeight) {
		    Accumulator -= ImageHeight;
		    StartX += StepX;
		}
		if (Direction < 0) Direction = 0;
		if (Direction > ImageWidth) Direction = ImageWidth;
	    }
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
    if (GifFile != NULL) EGifCloseFile(GifFile);
    exit(EXIT_FAILURE);
}
