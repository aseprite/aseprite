/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Aug. 1991   *
******************************************************************************
* Program to rotate a GIF image by an arbitrary angle.			     *
* Options:								     *
* -q : quiet printing mode.						     *
* -a Angle : angle to rotate with respect to the X axis.		     *
* -s Width Height : specifies size of output image.                          *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
*  2 Aug 91 - Version 1.0 by Gershon Elber.				     *
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
#include <math.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "gif_lib.h"
#include "getarg.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif /* M_PI */

#define PROGRAM_NAME	"GifRotat"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
	"Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifRotat a!-Angle!d q%- s%-Width|Height!d!d h%- GifFile!*s";
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
	" a!-Angle!d q%- s%-Width|Height!d!d h%- GifFile!*s";
#endif /* SYSV */

static int
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */

static void RotateGifImage(GifRowType *ScreenBuffer, GifFileType *SrcGifFile,
			   int Angle, ColorMapObject *ColorMap,
			   int DstWidth, int DstHeight);
static void RotateGifLine(GifRowType *ScreenBuffer, int BackGroundColor,
			  int SrcWidth, int SrcHeight,
			  int Angle, GifRowType DstLine,
			  int DstWidth, int DstHeight, int y);
static void QuitGifError(GifFileType *DstGifFile);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, Size, Error, NumFiles, Col, Row, Count, ExtCode,
	DstWidth, DstHeight, Width, Height,
	ImageNum = 0,
	DstSizeFlag = FALSE,
	AngleFlag = FALSE,
	Angle = 0,
	HelpFlag = FALSE;
    char **FileName = NULL;
    GifRecordType RecordType;
    GifByteType *Extension;
    GifFileType *GifFile;
    GifRowType *ScreenBuffer;
    ColorMapObject *ColorMap = NULL;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&AngleFlag, &Angle, &GifQuietPrint,
		&DstSizeFlag, &DstWidth, &DstHeight, &HelpFlag,
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

    for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
	ScreenBuffer[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) {
	/* Allocate the other rows, and set their color to background too: */
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

    ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap :
				       GifFile->SColorMap);

    if (!DstSizeFlag) {
	DstWidth = GifFile->SWidth;
	DstHeight = GifFile->SHeight;
    }

    /* Perform the actual rotation and dump the image: */
    RotateGifImage(ScreenBuffer, GifFile, Angle, ColorMap,
		   DstWidth, DstHeight);

    return 0;
}

/******************************************************************************
* Save the GIF resulting image.						      *
******************************************************************************/
static void RotateGifImage(GifRowType *ScreenBuffer, GifFileType *SrcGifFile,
			   int Angle, ColorMapObject *ColorMap,
			   int DstWidth, int DstHeight)
{
    int i,
	LineSize = DstWidth * sizeof(GifPixelType);
    GifFileType *DstGifFile;
    GifRowType LineBuffer;

    if ((LineBuffer = (GifRowType) malloc(LineSize)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    /* Open stdout for the output file: */
    if ((DstGifFile = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(DstGifFile);

    if (EGifPutScreenDesc(DstGifFile, DstWidth, DstHeight,
			  ColorMap->BitsPerPixel, 0, ColorMap) == GIF_ERROR ||
	EGifPutImageDesc(DstGifFile, 0, 0, DstWidth, DstHeight,
			 FALSE, NULL) == GIF_ERROR)
	QuitGifError(DstGifFile);

    for (i = 0; i < DstHeight; i++) {
	RotateGifLine(ScreenBuffer, SrcGifFile->SBackGroundColor,
		      SrcGifFile->SWidth, SrcGifFile->SHeight,
		      Angle, LineBuffer, DstWidth, DstHeight, i);
	if (EGifPutLine(DstGifFile, LineBuffer, DstWidth) == GIF_ERROR)
	    QuitGifError(DstGifFile);
	GifQprintf("\b\b\b\b%-4d", DstHeight - i - 1);
    }

    if (EGifCloseFile(DstGifFile) == GIF_ERROR)
	QuitGifError(DstGifFile);
}


/******************************************************************************
* Save the GIF resulting image.						      *
******************************************************************************/
static void RotateGifLine(GifRowType *ScreenBuffer, int BackGroundColor,
		          int SrcWidth, int SrcHeight,
			  int Angle, GifRowType DstLine,
			  int DstWidth, int DstHeight, int y)
{
    int x,
	TransSrcX = SrcWidth / 2,
	TransSrcY = SrcHeight / 2,
	TransDstX = DstWidth / 2,
	TransDstY = DstHeight / 2;
    double SinAngle = sin(Angle * M_PI / 180.0),
	   CosAngle = cos(Angle * M_PI / 180.0);

    for (x = 0; x < DstWidth; x++)
    {
	int xc = x - TransDstX,
	    yc = y - TransDstY,
	    SrcX = xc * CosAngle - yc * SinAngle + TransSrcX,
	    SrcY = xc * SinAngle + yc * CosAngle + TransSrcY;

	if (SrcX < 0 || SrcX >= SrcWidth ||
	    SrcY < 0 || SrcY >= SrcHeight)
	{
	    /* Out of the source image domain - set it to background color. */
	    *DstLine++ = BackGroundColor;
	}
	else
	    *DstLine++ = ScreenBuffer[SrcY][SrcX];
    }
}

/******************************************************************************
* Close output file (if open), and exit.				      *
******************************************************************************/
static void QuitGifError(GifFileType *DstGifFile)
{
    PrintGifError();
    if (DstGifFile != NULL) EGifCloseFile(DstGifFile);
    exit(EXIT_FAILURE);
}
