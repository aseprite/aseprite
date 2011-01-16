/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to dump GIF file into EPSON type printers			     *
* Options:								     *
* -q : quiet printing mode.						     *
* -d factor : use dithering of matrix of size factor by factor.		     *
* -t level : set the threshold level of white in the result (0..100).	     *
* -m mapping : methods for mapping the 24bits colors into 1 BW bit.	     *
* -p printer : specify printer to print to (lpt1: by default).		     *
* -n : nice mode : uses double density to achieve better quality.	     *
* -i : invert the image.						     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 15 Jul 89 - Version 1.0 by Gershon Elber.				     *
* 22 Dec 89 - Fix problems with const strings been modified (Version 1.1).   *
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

#define PROGRAM_NAME	"Gif2Epsn"

#define	C2BW_BACK_GROUND	0 /*Methods to map 24bits Colors to 1 BW bit.*/
#define	C2BW_GREY_LEVELS	1
#define C2BW_DITHER		2
#define C2BW_NUM_METHODS	3	        /* Always hold # of methods. */

#define DEFAULT_THRESHOLD	5000	     /* Color->BW threshold level. */

#define DITHER_MIN_MATRIX	2
#define DITHER_MAX_MATRIX	4

/* The epson specific are defined here: */
#define EPSON_WIDTH		80		        /* 80 char per line. */
#define EPSON_PIXEL_2_CHAR	8      /* 8 pixels per char, in REG_DENSITY. */

#define EPSON_ESC		"\033"	    /* Actually regular escape char. */
#define EPSON_RESET		"\033@"		       /* Reset the printer. */
#define EPSON_VERTICAL_SPACE	"\033A\010"         /* 8/72 inch vertically. */
#define EPSON_REG_DENSITY	"\033K"	      /* 640 pixels per 7.5" (line). */
#define EPSON_DUAL_DENSITY	"\033L"	     /* 1280 pixels per 7.5" (line). */

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "Gif2Epsn q%- d%-DitherSize!d t%-BWThreshold!d m%-Mapping!d i%- n%- p%-PrinterName!s h%- GifFile!*s";
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
	" q%- d%-DitherSize!d t%-BWThreshold!d m%-Mapping!d i%- n%- p%-PrinterName!s h%- GifFile!*s";
#endif /* SYSV */

static char
    *PrinterName = NULL;
/* Make some variables global, so we could access them faster: */
static int
    ImageNum = 0,
    BackGround = 0,
    DitherSize = 2, DitherFlag = FALSE,
    BWThresholdFlag = FALSE, Threshold,
    BWThreshold = DEFAULT_THRESHOLD,	   /* Color->BW mapping threshold. */
    Mapping, MappingFlag = FALSE,
    InvertFlag = FALSE,
    NiceFlag = FALSE,
    PrinterFlag = FALSE,
    HelpFlag = FALSE,
    ColorToBWMapping = C2BW_BACK_GROUND,
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should  */
    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
static GifColorType
    *ColorMap;

static void EvalDitheredScanline(GifRowType *ScreenBuffer, int Row,
					int RowSize, GifRowType *DitherBuffer);
static void DumpScreen2Epsn(GifRowType *ScreenBuffer,
					int ScreenWidth, int ScreenHeight);
static void PutString(FILE *Prt, int DirectPrint, char *Str, int Len);
static void PutString2(FILE *Prt, int DirectPrint, char *Str, int Len);

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

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &DitherFlag, &DitherSize,
		&BWThresholdFlag, &Threshold,
		&MappingFlag, &Mapping,	&InvertFlag,
		&NiceFlag, &PrinterFlag, &PrinterName, &HelpFlag,
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

    if (!PrinterFlag) PrinterName = "";

    if (DitherFlag) {
	/* Make sure we are o.k.: */
	if (DitherSize > DITHER_MAX_MATRIX) DitherSize = DITHER_MAX_MATRIX;
	if (DitherSize < DITHER_MIN_MATRIX) DitherSize = DITHER_MAX_MATRIX;
    }

    /* As Threshold is in [0..100] range and BWThreshold is [0..25500]: */
    if (BWThresholdFlag) {
	if (Threshold > 100 || Threshold < 0)
	    GIF_EXIT("Threshold not in 0..100 percent.");
	BWThreshold = Threshold * 255;
	if (BWThreshold == 0) BWThreshold = 1;   /* Overcome divide by zero! */
    }

    /* No message is emitted, but mapping method is clipped to exists method.*/
    if (MappingFlag) ColorToBWMapping = Mapping % C2BW_NUM_METHODS;

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
	/* Allocate the other rows, andset their color to background too:   */
	if ((ScreenBuffer[i] = (GifRowType) malloc(Size)) == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.\n");

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
    ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap->Colors :
				       (GifFile->SColorMap ? GifFile->SColorMap->Colors :
                        NULL));
    if (ColorMap == NULL) {
        fprintf(stderr, "Gif Image does not have a colormap\n");
        exit(EXIT_FAILURE);
    }
    DumpScreen2Epsn(ScreenBuffer, GifFile->SWidth, GifFile->SHeight);

    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError();
	exit(EXIT_FAILURE);
    }

    return 0;
}

/*****************************************************************************
* Routine to evaluate dithered scanlines out of given ones, using Size	     *
* dithering matrix, starting from Row. The given scanlines are NOT modified. *
*****************************************************************************/
static void EvalDitheredScanline(GifRowType *ScreenBuffer, int Row,
					int RowSize, GifRowType *DitherBuffer)
{
    static char Dither2[2][2] = {	 /* See Foley & Van Dam pp. 597-601. */
	{ 1, 3 },
	{ 4, 2 }
    };
    static char Dither3[3][3] = {
	{ 7, 9, 5 },
	{ 2, 1, 4 },
	{ 6, 3, 8 }
    };
    static char Dither4[4][4] = {
	{ 1,  9,  3,  11 },
	{ 13, 5,  15, 7 },
	{ 4,  12, 2,  10 },
	{ 16, 8,  14, 6 }
    };
    int i, j, k, Level;
    long Intensity;
    GifColorType *ColorMapEntry;

    /* Scan the Rows (Size rows) evaluate intensity every Size pixel and use */
    /* the dither matrix to set the dithered result;			     */
    for (i = 0; i <= RowSize - DitherSize; i += DitherSize) {
	Intensity = 0;
	for (j = Row; j < Row + DitherSize; j++)
	    for (k = 0; k < DitherSize; k++) {
		ColorMapEntry = &ColorMap[ScreenBuffer[j][i+k]];
		Intensity += 30 * ((int) ColorMapEntry->Red) +
			     59 * ((int) ColorMapEntry->Green) +
			     11 * ((int) ColorMapEntry->Blue);
	    }

	/* Find the intensity level (between 0 and Size^2) of our matrix:    */
	/* Expression is "Intensity * BWThreshold / (25500 * DefThresh)"     */
	/* but to prevent from overflow in the long evaluation we do this:   */
	Level = ((Intensity / 2550) * ((long) DEFAULT_THRESHOLD) /
						(((long) BWThreshold) * 10));
	switch (DitherSize) {
	    case 2:     
		for (j = 0; j < DitherSize; j++)
		    for (k = 0; k < DitherSize; k++)
			DitherBuffer[j][i+k] = Dither2[j][k] <= Level;
		break;
	    case 3:
		for (j = 0; j < DitherSize; j++)
		    for (k = 0; k < DitherSize; k++)
			DitherBuffer[j][i+k] = Dither3[j][k] <= Level;
		break;
	    case 4:
		for (j = 0; j < DitherSize; j++)
		    for (k = 0; k < DitherSize; k++)
			DitherBuffer[j][i+k] = Dither4[j][k] <= Level;
		break;
	}
    }
}

/******************************************************************************
* The real dumping routine. Few things are taken into account:		      *
* 1. The Nice flag. If TRUE each pixel is printed twice in double density.    *
* 2. The Invert flag. If TRUE each pixel before drawn is inverted.	      *
* 3. The rendering mode and dither matrix flag if dithering is selected.      *
*   The image is drawn from ScreenBuffer ScreenTop/Left in the bottom/right   *
* directions.							     	      *
*   Unfortunatelly, there is a BUG in DOS that does not handle ctrl-Z	      *
* correctly if we open lptx: device in binary mode (should treat it as any    *
* other char). Therefore I had to write to it directly using biosprint. I     *
* dont like it either, and if you have better way to do it, let me know.      *
******************************************************************************/
static void DumpScreen2Epsn(GifRowType *ScreenBuffer,
					int ScreenWidth, int ScreenHeight)
{
    int i, j, p, Size, LeftCWidth, Len, DirectPrint = 0,
	DitheredLinesLeft = 0, DitheredLinesCount = 0, MapInvert[2];
    char LinePrefixLen[2];			     /* Length of scan line. */
    GifByteType *EpsonBuffer;
    GifPixelType *Line;
    GifRowType *DitherBuffer;
    GifColorType *ColorMapEntry;
    FILE *Prt = NULL;

#ifdef __MSDOS__
    for (i = 0;	 i < strlen(PrinterName); i++)
	if (islower(PrinterName[i]))
	    PrinterName[i] = toupper(PrinterName[i]);

    if (strcmp(PrinterName, "LPT1") == 0 ||
	strcmp(PrinterName, "PRN") == 0)
	DirectPrint = 1;
    else if (strcmp(PrinterName, "LPT2") == 0)
        DirectPrint = 2;
    else if (strcmp(PrinterName, "LPT3") == 0)
        DirectPrint = 3;
#endif /* __MSDOS__ */

    if (!DirectPrint) {
	if (strlen(PrinterName) == 0) {
#ifdef __MSDOS__
	    setmode(1, O_BINARY);	  /* Make sure it is in binary mode. */
#endif
	    Prt = stdout;
	}
	else
        {
                Prt = fopen(PrinterName, "wb");
                if (Prt == NULL)
                {
                    GIF_EXIT("Failed to open output (printer) file.");
                }

#ifdef __MSDOS__
                if (setvbuf(Prt, NULL, _IOFBF, GIF_FILE_BUFFER_SIZE))
                {
                    GIF_EXIT("Failed to open output (printer) file.");
                }
#endif
        }
    }

    if ((EpsonBuffer = (GifByteType *) malloc(ScreenWidth)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    /* Allocate the buffer to save the dithered information. */
    if (ColorToBWMapping == C2BW_DITHER) {
	if ((DitherBuffer = (GifRowType *)
	    malloc(DITHER_MAX_MATRIX * sizeof(GifRowType *))) != NULL) {
	    Size = ScreenWidth * sizeof(GifPixelType);	 /* Size of one row. */
	    for (i = 0; i < DITHER_MAX_MATRIX; i++) {
		if ((DitherBuffer[i] = (GifRowType) malloc(Size)) == NULL) {
		    DitherBuffer = NULL;
		    break;
		}
	    }
	}
	if (DitherBuffer == NULL)
	    GIF_EXIT("Failed to allocate memory required, aborted.");
    }
    else
	DitherBuffer = NULL;

    /* Reset the printer, and make sure no space between adjacent lines: */
    PutString(Prt, DirectPrint, EPSON_RESET, 2);
    PutString(Prt, DirectPrint, EPSON_VERTICAL_SPACE, 3);

    /* Prepar left spacing to begin with, so image will be in the middle.    */
    LeftCWidth = (EPSON_WIDTH - (ScreenWidth / EPSON_PIXEL_2_CHAR)) / 2;

    if (InvertFlag) {		   /* Make the inversion as fast a possible. */
	MapInvert[0] = 1;
	MapInvert[1] = 0;
    }
    else {
	MapInvert[0] = 0;
	MapInvert[1] = 1;
    }

    for (i = 0, p = 0; i < ScreenHeight; i++, p++) {
	GifQprintf("\b\b\b\b%-4d", ScreenHeight - i);
	Line = ScreenBuffer[i];

	/* If 8 lines were accumulated in printer buffer - dump them out.    */
	if (p == 8) {
	    for (Len = ScreenWidth-1; Len >= 0; Len--)
	        if (EpsonBuffer[Len]) break;

	    /* Only in case this line is not empty: */
	    if (Len++ >= 0) {
		/* Make the left space, so image will be centered: */
		for (j = 0; j < LeftCWidth; j++)
		    PutString(Prt, DirectPrint,  " ", 1);

		/* Full printer line is ready to be dumped - send it out: */
		if (NiceFlag) {
		    PutString(Prt, DirectPrint, EPSON_DUAL_DENSITY, 2);
		    LinePrefixLen[0] = (Len * 2) % 256;
		    LinePrefixLen[1] = (Len * 2) / 256;
		    PutString(Prt, DirectPrint, LinePrefixLen, 2);
		    PutString2(Prt, DirectPrint, (char *) EpsonBuffer, Len);
		}
		else {
		    PutString(Prt, DirectPrint, EPSON_REG_DENSITY, 2);
		    LinePrefixLen[0] = Len % 256;
		    LinePrefixLen[1] = Len / 256;
		    PutString(Prt, DirectPrint, LinePrefixLen, 2);
		    PutString(Prt, DirectPrint, (char *) EpsonBuffer, Len);
		}
	    }
	    PutString(Prt, DirectPrint, "\015\012", 2);
	    p = 0;
	}

	/* We decide right here what method to map Colors to BW so the inner */
	/* loop will be independent of it (and therefore faster):	     */
	switch(ColorToBWMapping) {
	    case C2BW_BACK_GROUND:
		for (j = 0; j < ScreenWidth; j++)
		    EpsonBuffer[j] = (EpsonBuffer[j] << 1) +
					MapInvert[Line[j] != BackGround];
		break;
	    case C2BW_GREY_LEVELS:
		for (j = 0; j < ScreenWidth; j++) {
		    ColorMapEntry = &ColorMap[Line[j]];
		    /* For the transformation from RGB to BW, see Folley &   */
		    /* Van Dam pp 613: The Y channel is the BW we need:	     */
		    /* As colors are 255 maximum, the result can be up to    */
		    /* 25500 which is still in range of our 16 bits integers */
		    EpsonBuffer[j] = (EpsonBuffer[j] << 1) +
			MapInvert[(30 * (int) ColorMapEntry->Red) +
				   59 * ((int) ColorMapEntry->Green) +
				   11 * ((int) ColorMapEntry->Blue) >
					BWThreshold];
		}
		break;
	    case C2BW_DITHER:
		if (DitheredLinesLeft-- == 0) {
		    EvalDitheredScanline(ScreenBuffer,
			(i < ScreenHeight - DitherSize ? i :
						 ScreenHeight - DitherSize),
			ScreenWidth, DitherBuffer);
		    DitheredLinesLeft = DitherSize - 1;
		    DitheredLinesCount = 0;
		}
		Line = DitherBuffer[DitheredLinesCount++];
		for (j = 0; j < ScreenWidth; j++)
		    EpsonBuffer[j] = (EpsonBuffer[j] << 1) +
							MapInvert[Line[j]];
		break;
	}
    }

    /* If buffer in incomplete - complete it and dump it out: */
    if (p != 0) {
	for (Len = ScreenWidth - 1; Len >= 0; Len--)
	    if (EpsonBuffer[Len]) break;
	if (Len++ >= 0) {
	    i = 8 - p;					 /* Amount to shift. */
	    for (j = 0; j < ScreenWidth; j++) EpsonBuffer[j] <<= i;

	    /* Make the left space, so image will be centered: */
	    for (j = 0; j < LeftCWidth; j++)
		PutString(Prt, DirectPrint, " ", 1);

	    if (NiceFlag) {
		PutString(Prt, DirectPrint, EPSON_DUAL_DENSITY, 2);
		LinePrefixLen[0] = (Len * 2) % 256;
		LinePrefixLen[1] = (Len * 2) / 256;
		PutString(Prt, DirectPrint, LinePrefixLen, 2);
		PutString2(Prt, DirectPrint, (char *) EpsonBuffer, Len);
	    }
	    else {
		PutString(Prt, DirectPrint, EPSON_REG_DENSITY, 2);
		LinePrefixLen[0] = Len % 256;
		LinePrefixLen[1] = Len / 256;
		PutString(Prt, DirectPrint, LinePrefixLen, 2);
		PutString(Prt, DirectPrint, (char *) EpsonBuffer, Len);
	    }
	}
	PutString(Prt, DirectPrint, "\015\012", 2);
    }

    fclose(Prt);
}

/******************************************************************************
* Dumps the string of given length, to Prt. No char in Str has special	      *
* meaning, and even zero (NULL) chars are dumped.			      *
* If however DirectPrint is non zero, string is dumped to specifed lpt port.  *
******************************************************************************/
static void PutString(FILE *Prt, int DirectPrint, char *Str, int Len)
{
    int i;

    if (DirectPrint) {
#ifdef __MSDOS__
	for (i = 0; i < Len; i++) biosprint(0, Str[i], DirectPrint - 1);
#else
	GIF_EXIT("Can not print directly to a printer if not MSDOS.");
#endif /* __MSDOS__ */
    }
    else
	for (i = 0; i < Len; i++) fputc(Str[i], Prt);
}

/******************************************************************************
* Dumps the string of given length, to Prt. No char in Str has special	      *
* meaning, and even zero (NULL) chars are dumped. Every char is dumped twice. *
* If however DirectPrint is non zero, string is dumped to specifed lpt port.  *
******************************************************************************/
static void PutString2(FILE *Prt, int DirectPrint, char *Str, int Len)
{
    int i;

    if (DirectPrint) {
#ifdef __MSDOS__
	for (i = 0; i < Len; i++) {
	    biosprint(0, Str[i], DirectPrint - 1);
	    biosprint(0, Str[i], DirectPrint - 1);
	}
#else
	GIF_EXIT("Can not print directly to a printer if not MSDOS.");
#endif /* __MSDOS__ */
    }
    else
	for (i = 0; i < Len; i++) {
	    fputc(Str[i], Prt);
	    fputc(Str[i], Prt);
	}
}
