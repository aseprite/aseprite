/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to convert GIF file RLE format (utah raster toolkit).		     *
* Options:								     *
* -q : quiet printing mode.						     *
* -a : add alpha channel with full coverage.				     *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 5 Jan 90 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <graphics.h>
#include <alloc.h>
#include <io.h>
#include <dos.h>
#include <bios.h>
#endif /* __MSDOS__ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "gif_lib.h"
#include "getarg.h"

#include "rle.h"			   /* The rle tool kit header files. */

#define PROGRAM_NAME	"Gif2Rle"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "Gif2Rle q%- a%- h%- GifFile!*s";
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
	" q%- a%- h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static int
    ImageNum = 0,
    BackGround = 0,
    AlphaFlag = FALSE,
    HelpFlag = FALSE,
    ColorMapSize = 0,
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
static GifColorType
    *ColorMap;

static void DumpScreen2Rle(GifRowType *ScreenBuffer,
					int ScreenWidth, int ScreenHeight);

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

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint,
		&AlphaFlag, &HelpFlag, &NumFiles, &FileName)) != FALSE ||
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
		    fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n", ImageNum);
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
    ColorMap = (GifFile->Image.ColorMap ?
                    GifFile->Image.ColorMap->Colors :
                    GifFile->SColorMap->Colors);
    if (ColorMap == NULL) {
        fprintf(stderr, "Gif Image does not have a colormap\n");
        exit(EXIT_FAILURE);
    }
    ColorMapSize = 1 << ColorMap->BitsPerPixel;
    DumpScreen2Rle(ScreenBuffer, GifFile->SWidth, GifFile->SHeight);

    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError();
	exit(EXIT_FAILURE);
    }

    return 0;
}

/******************************************************************************
* The real dumping routine.						      *
******************************************************************************/
static void DumpScreen2Rle(GifRowType *ScreenBuffer,
					int ScreenWidth, int ScreenHeight)
{
    int i, j;
    char Comment[80];
    rle_pixel *rows[4];
    GifRowType GifRow;
    static GifColorType
	*ColorMapEntry;

    if (AlphaFlag) RLE_SET_BIT(rle_dflt_hdr, RLE_ALPHA);
    rle_dflt_hdr.alpha = AlphaFlag != 0;
    rle_dflt_hdr.rle_file = stdout;
    rle_dflt_hdr.xmin = 0;
    rle_dflt_hdr.ymin = 0;
    rle_dflt_hdr.xmax = ScreenWidth - 1;
    rle_dflt_hdr.ymax = ScreenHeight - 1;
    sprintf(Comment, "origin=GIF format, %d colors.", ColorMapSize);
    rle_putcom(Comment, &rle_dflt_hdr);
    rle_put_setup(&rle_dflt_hdr);

    for (i = 0; i < 4; i++)
	if ((rows[i] = (rle_pixel *) malloc(sizeof(rle_pixel) * ScreenWidth))
								     == NULL)
	    GIF_EXIT("Failed to allocated memory required, aborted.");

    if (AlphaFlag) {
	/* Initial the alpha channel to full coverage: */
	for (i = 0; i < ScreenWidth; i++) rows[0][i] = 255;
    }

    for (i = 0; i < ScreenHeight; i++) {
	/* Flip the image vertically as rle files start at the bollom... */
	GifRow = ScreenBuffer[ScreenHeight - i - 1];
	GifQprintf("\b\b\b\b%-4d", ScreenHeight - i);
	for (j = 0; j < ScreenWidth; j++) {
	    ColorMapEntry = &ColorMap[GifRow[j]];
	    rows[1][j] = ColorMapEntry->Red;
	    rows[2][j] = ColorMapEntry->Green;
	    rows[3][j] = ColorMapEntry->Blue;
	}
	rle_putrow( &rows[1], ScreenWidth, &rle_dflt_hdr );
    }

    rle_puteof( &rle_dflt_hdr );
}
