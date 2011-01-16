/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to dump GIF file into PostScript type printers		     *
* Options:								     *
* -q : quiet printing mode.						     *
* -x : force image to be horizontal.					     *
* -y : force image to be vertical.					     *
* -s x y : force image to be of given size.				     *
* -p x y : force image to be positioned at given position in page.	     *
* -i : invert the image.						     *
* -n n : number of copies.						     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 22 Dec 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <graphics.h>
#include <stdlib.h>
#include <alloc.h>
#include <io.h>
#include <dos.h>
#include <bios.h>
#endif /* __MSDOS__ */

#ifndef __MSDOS__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"Gif2PS"

#define PAGE_WIDTH      7.5		    /* All dimensions are in inches. */
#define PAGE_HEIGHT     9.0
#define FULL_PAGE_WIDTH 8.5
#define FULL_PAGE_HEIGHT 11.0

#define UNKNOWN_ORIENT		0
#define HORIZONTAL_ORIENT	1 
#define VERTICAL_ORIENT		2

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "Gif2PS q%- x%- y%- s%-SizeX|SizeY!F!F p%-PosX|PosY!F!F i%- n%-#Copies!d h%- GifFile!*s";
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
	" q%- x%- y%- s%-SizeX|SizeY!F!F p%-PosX|PosY!F!F i%- n%-#Copies!d h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static int
    ImageNum = 0,
    BackGround = 0,
    ForceXFlag = FALSE,
    ForceYFlag = FALSE,
    SizeFlag = FALSE,
    PosFlag = FALSE,
    InvertFlag = FALSE,
    NumCopiesFlag = FALSE,
    HelpFlag = FALSE,
    NumOfCopies = 1,
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
    InterlacedJumps[] = { 8, 8, 4, 2 },    /* be read - offsets and jumps... */
    PSOrientation;
static double PSSizeX, PSSizeY, PSPosX, PSPosY;
static GifColorType
    *ColorMap;

static void DumpScreen2PS(GifRowType *ScreenBuffer,
					int ScreenWidth, int ScreenHeight);
static void PutString(unsigned char *Line, int Len);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, Error, NumFiles, Size, Row, Col, Width, Height, ExtCode, Count;
    GifRecordType RecordType;
    GifByteType *Extension;
    char **FileName = NULL;
    GifRowType *ScreenBuffer;
    GifFileType *GifFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,&GifQuietPrint,
		&ForceXFlag, &ForceYFlag, &SizeFlag, &PSSizeX, &PSSizeY,
		&PosFlag, &PSPosX, &PSPosY,
		&InvertFlag, &NumCopiesFlag, &NumOfCopies, &HelpFlag,
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

    if (ForceXFlag)
        PSOrientation = HORIZONTAL_ORIENT;
    else if (ForceYFlag)
        PSOrientation = VERTICAL_ORIENT;
    else
	PSOrientation = UNKNOWN_ORIENT;

    if (NumFiles == 1) {
	if ((GifFile = DGifOpenFileName(*FileName)) == NULL) {
	    PrintGifError();
	    exit(EXIT_FAILURE);
	}
    }
    else {
	/* Use the stdin instead: */

#ifdef __MSDOS__
	setmode(0, O_BINARY);
#endif /* __MSDOS__ */
	if ((GifFile = DGifOpenFileHandle(0)) == NULL) {
	    PrintGifError();
	    exit(EXIT_FAILURE);
	}
    }

    /* Allocate the screen as vector of column of rows. We cannt allocate    */
    /* the all screen at once, as this broken minded CPU can allocate up to  */
    /* 64k at a time and our image can be bigger than that:		     */
    /* Note this screen is device independent - its the screen as defined by */
    /* the GIF file parameters itself.					     */
    if ((ScreenBuffer = (GifRowType *)
	malloc(GifFile->SHeight * sizeof(GifRowType *))) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");

    Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/
    if ((ScreenBuffer[0] = (GifRowType) malloc(Size)) == NULL) /* First row. */
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < GifFile->SWidth; i++) /* Set its color to BackGround. */
	ScreenBuffer[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) {
	/* Allocate the other rows, and set their color to background too:  */
	if ((ScreenBuffer[i] = (GifRowType) malloc(Size)) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");

	memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
    }

    /* Scan the content of the GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
	    PrintGifError();
	    exit(EXIT_FAILURE);
	}
	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
		    PrintGifError();
		    exit(EXIT_FAILURE);
		}
		Row = GifFile->Image.Top; /* Image Position relative to Screen. */
		Col = GifFile->Image.Left;
		Width = GifFile->Image.Width;
		Height = GifFile->Image.Height;
		GifQprintf("\n%s: Image %d at (%d, %d) [%dx%d]:     ",
		    PROGRAM_NAME, ++ImageNum, Col, Row, Width, Height);
		if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
		   GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
		    fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n",ImageNum);
		    exit(EXIT_FAILURE);
		}
		if (GifFile->Image.Interlace) {
		    /* Need to perform 4 passes on the images: */
 		    for (Count = i = 0; i < 4; i++)
			for (j = Row + InterlacedOffset[i]; j < Row + Height;
						 j += InterlacedJumps[i]) {
			    GifQprintf("\b\b\b\b%-4d", Count++);
			    if (DGifGetLine(GifFile, &ScreenBuffer[j][Col],
				Width) == GIF_ERROR) {
				PrintGifError();
				exit(EXIT_FAILURE);
			    }
			}
		}
		else {
		    for (i = 0; i < Height; i++) {
			GifQprintf("\b\b\b\b%-4d", i);
			if (DGifGetLine(GifFile, &ScreenBuffer[Row++][Col],
				Width) == GIF_ERROR) {
			    PrintGifError();
			    exit(EXIT_FAILURE);
			}
		    }
		}
		break;
	    case EXTENSION_RECORD_TYPE:
		/* Skip any extension blocks in file: */
		if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
		    PrintGifError();
		    exit(EXIT_FAILURE);
		}
		while (Extension != NULL) {
		    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
			PrintGifError();
			exit(EXIT_FAILURE);
		    }
		}
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		    /* Should be traps by DGifGetRecordType. */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE);

    /* Lets display it - set the global variables required and do it: */
    BackGround = GifFile->SBackGroundColor;
    ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap->Colors
		: (GifFile->SColorMap ? GifFile->SColorMap->Colors : NULL));
    if (ColorMap == NULL) {
        fprintf(stderr, "Gif Image does not have a colormap\n");
        exit(EXIT_FAILURE);
    }
    DumpScreen2PS(ScreenBuffer, GifFile->SWidth, GifFile->SHeight);

    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError();
	exit(EXIT_FAILURE);
    }

    return 0;
}

/******************************************************************************
* The real dumping routine.						      *
******************************************************************************/
static void DumpScreen2PS(GifRowType *ScreenBuffer,
					int ScreenWidth, int ScreenHeight)
{
    int i, j;
    double Aspect;
    GifByteType *OutLine, Data;
    GifPixelType *Line;
    GifColorType *ColorMapEntry;

    /* If user did not enforce orientation, pick the best one. */
    if (PSOrientation == UNKNOWN_ORIENT) {
	if (ScreenWidth > ScreenHeight) {
	    PSOrientation = VERTICAL_ORIENT;
    } else {
	    PSOrientation = HORIZONTAL_ORIENT;
    }
    }
    Aspect = ((double) ScreenHeight) / ((double) ScreenWidth);

    if (!SizeFlag)
	switch (PSOrientation) {
            case HORIZONTAL_ORIENT:
	        if (Aspect > PAGE_HEIGHT / PAGE_WIDTH) {
		    PSSizeX = PAGE_HEIGHT / Aspect; 
		    PSSizeY = PAGE_HEIGHT;
		}
		else {
		    PSSizeX = PAGE_WIDTH;
		    PSSizeY = PAGE_WIDTH * Aspect;
		}
		break;
	    case VERTICAL_ORIENT:
	        if (1 / Aspect > PAGE_HEIGHT / PAGE_WIDTH) {
		    PSSizeX = PAGE_HEIGHT * Aspect; 
		    PSSizeY = PAGE_HEIGHT;
		}
		else {
		    PSSizeX = PAGE_WIDTH;
		    PSSizeY = PAGE_WIDTH / Aspect;
		}
		break;
	}
    else {
	if (PAGE_WIDTH < PSSizeX) {
	    GIF_MESSAGE("X Size specified is too big, page size selected.");
	    PSSizeX = PAGE_WIDTH;
	}
	if (PAGE_HEIGHT < PSSizeY) {
	    GIF_MESSAGE("Y Size specified is too big, page size selected.");
	    PSSizeX = PAGE_HEIGHT;
	}
    }

    if (!PosFlag) {
	PSPosX = (FULL_PAGE_WIDTH - PSSizeX) / 2;
	PSPosY = (FULL_PAGE_HEIGHT - PSSizeY) / 2;
    }
    else {
	if (PSPosX + PSSizeX > PAGE_WIDTH || PSPosY + PSSizeY > PAGE_HEIGHT)
	    GIF_EXIT("Requested position will put image out of page, aborted.");
    }

    /* Time to dump out the PostScript header: */
    printf("%%!\n");
    printf("%%%%Creator: %s\n", PROGRAM_NAME);
    printf("/#copies %d def\n", NumOfCopies);
    printf("gsave\n");
    printf("72 72 scale\t\t\t\t%% Lets talk inches.\n");
    printf("/oneline %d string def\t\t\t%% Allocate one scan line.\n",
	   ScreenWidth);
    printf("/drawimage {\n");
    printf("\t%d %d 8 [%d 0 0 %d 0 %d]\n", ScreenWidth, ScreenHeight,
	   ScreenWidth, -ScreenHeight, ScreenHeight);
    printf("\t{ currentfile oneline readhexstring pop } image\n");
    printf("} def\n");
    switch (PSOrientation) {
        case HORIZONTAL_ORIENT:
	    printf("%f %f translate\n", PSPosX, PSPosY);
	    printf("%f %f scale\n", PSSizeX, PSSizeY);
	    break;
	case VERTICAL_ORIENT:
	    printf("%f %f translate\n", PSPosX + PSSizeX, PSPosY);
	    printf("90 rotate\n");
	    printf("%f %f scale\n", PSSizeY, PSSizeX);
	    break;
    }
    printf("drawimage\n");

    if ((OutLine = (GifByteType *) malloc(sizeof(GifByteType) * ScreenWidth))
								== NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < ScreenHeight; i++) {
	GifQprintf("\b\b\b\b%-4d", ScreenHeight - i);

	Line = ScreenBuffer[i];
	for (j = 0; j < ScreenWidth; j++) {
	    ColorMapEntry = &ColorMap[Line[j]];
	    Data = (30 * ((unsigned int) ColorMapEntry->Red) +
		    59 * ((unsigned int) ColorMapEntry->Green) +
		    11 * ((unsigned int) ColorMapEntry->Blue)) / 100;
	    OutLine[j] = InvertFlag ? 255 - Data : Data;
	}

	PutString(OutLine, ScreenWidth);
    }
    free(OutLine);

    printf("\nshowpage\n");
    printf("grestore\n");
}

/******************************************************************************
* Dumps the string of given length as 2 hexdigits per byte 39 bytes per line. *
******************************************************************************/
static void PutString(unsigned char *Line, int Len)
{
    int i;
    static int Counter = 0;
    static char *Hex = "0123456789ABCDEF";

    for (i = 0; i < Len; i++) {
	if (++Counter % 40 == 0) {
	    putchar('\n');
	    Counter = 1;
	}
	putchar(Hex[Line[i] >> 4]);
	putchar(Hex[Line[i] & 0x0f]);
    }
}
