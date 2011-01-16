/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jun. 1989   *
******************************************************************************
* Program to dump GIF file content as TEXT information			     *
* Options:								     *
* -q : quiet printing mode.						     *
* -c : include the color maps as well.					     *
* -e : include encoded information packed as bytes as well.		     *
* -z : include encoded information (12bits) codes as result from the zl alg. *
* -p : dump pixel information instead of encoded information.		     *
* -r : same as -p but dump one pixel as one byte in binary form with no      *
*      other information. This will create a file of size Width by Height.   *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 28 Jun 89 - Version 1.0 by Gershon Elber.				     *
* 21 Dec 89 - Fix segmentation fault problem in PrintCodeBlock (Version 1.1) *
* 25 Dec 89 - Add the -r flag for raw output.                                *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <stdlib.h>
#include <alloc.h>
#include <conio.h>
#include <io.h>
#endif /* __MSDOS__ */

#ifndef __MSDOS__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"GifText"

#define MAKE_PRINTABLE(c)  (isprint(c) ? (c) : ' ')

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifText q%- c%- e%- z%- p%- r%- h%- GifFile!*s";
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
	" q%- c%- e%- z%- p%- r%- h%- GifFile!*s";
#endif /* SYSV */

static void PrintCodeBlock(GifFileType *GifFile, GifByteType *CodeBlock, int Reset);
static void PrintPixelBlock(GifByteType *PixelBlock, int Len, int Reset);
static void PrintExtBlock(GifByteType *Extension, int Reset);
static void PrintLZCodes(GifFileType *GifFile);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int i, j, ExtCode, CodeSize, Error, NumFiles, Len,
	ColorMapFlag = FALSE, EncodedFlag = FALSE, LZCodesFlag = FALSE,
	PixelFlag = FALSE, HelpFlag = FALSE, RawFlag = FALSE, ImageNum = 1;
    char *GifFileName, **FileName = NULL;
    GifPixelType *Line;
    GifRecordType RecordType;
    GifByteType *CodeBlock, *Extension;
    GifFileType *GifFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &ColorMapFlag, &EncodedFlag,
		&LZCodesFlag, &PixelFlag, &RawFlag, &HelpFlag,
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
	GifFileName = *FileName;
	if ((GifFile = DGifOpenFileName(*FileName)) == NULL) {
	    PrintGifError();
	    exit(EXIT_FAILURE);
	}
    }
    else {
	/* Use the stdin instead: */
	GifFileName = "Stdin";
	if ((GifFile = DGifOpenFileHandle(0)) == NULL) {
	    PrintGifError();
	    exit(EXIT_FAILURE);
	}
    }

    /* Because we write binary data - make sure no text will be written. */
    if (RawFlag) {
	ColorMapFlag = EncodedFlag = LZCodesFlag = PixelFlag = FALSE;
#ifdef __MSDOS__
	setmode(1, O_BINARY);             /* Make sure it is in binary mode. */
#endif /* __MSDOS__ */
    }
    else {
	printf("\n%s:\n\n\tScreen Size - Width = %d, Height = %d.\n",
	       GifFileName, GifFile->SWidth, GifFile->SHeight);
	printf("\tColorResolution = %d, BitsPerPixel = %d, BackGround = %d.\n",
	       GifFile->SColorResolution,
	       GifFile->SColorMap ? GifFile->SColorMap->BitsPerPixel : 0,
	       GifFile->SBackGroundColor);
	if (GifFile->SColorMap)
	    printf("\tHas Global Color Map.\n\n");
	else
	    printf("\tNo Global Color Map.\n\n");
	if (ColorMapFlag && GifFile->SColorMap) {
	    printf("\tGlobal Color Map:\n");
	    Len = GifFile->SColorMap->ColorCount;
	    for (i = 0; i < Len; i+=4) {
		for (j = 0; j < 4 && j < Len; j++) {
		    printf("%3d: %02xh %02xh %02xh   ", i + j,
			   GifFile->SColorMap->Colors[i + j].Red,
			   GifFile->SColorMap->Colors[i + j].Green,
			   GifFile->SColorMap->Colors[i + j].Blue);
		}
		printf("\n");
	    }
	}
    }

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
		if (!RawFlag) {
		    printf("\nImage #%d:\n\n\tImage Size - Left = %d, Top = %d, Width = %d, Height = %d.\n",
			   ImageNum++, GifFile->Image.Left, GifFile->Image.Top,
			   GifFile->Image.Width, GifFile->Image.Height);
		    printf("\tImage is %s",
			   GifFile->Image.Interlace ? "Interlaced" :
						    "Non Interlaced");
		    if (GifFile->Image.ColorMap != NULL)
			printf(", BitsPerPixel = %d.\n",
				GifFile->Image.ColorMap->BitsPerPixel);
		    else
			printf(".\n");
		    if (GifFile->Image.ColorMap)
			printf("\tImage Has Color Map.\n");
		    else
			printf("\tNo Image Color Map.\n");
		    if (ColorMapFlag && GifFile->Image.ColorMap) {
			Len = 1 << GifFile->Image.ColorMap->BitsPerPixel;
			for (i = 0; i < Len; i+=4) {
			    for (j = 0; j < 4 && j < Len; j++) {
				printf("%3d: %02xh %02xh %02xh   ", i + j,
				       GifFile->Image.ColorMap->Colors[i + j].Red,
				       GifFile->Image.ColorMap->Colors[i + j].Green,
				       GifFile->Image.ColorMap->Colors[i + j].Blue);
			    }
			    printf("\n");
			}
		    }
		}

		if (EncodedFlag) {
		    if (DGifGetCode(GifFile, &CodeSize, &CodeBlock) == GIF_ERROR) {
			PrintGifError();
			exit(EXIT_FAILURE);
		    }
		    printf("\nImage LZ compressed Codes (Code Size = %d):\n",
			   CodeSize);
		    PrintCodeBlock(GifFile, CodeBlock, TRUE);
		    while (CodeBlock != NULL) {
			if (DGifGetCodeNext(GifFile, &CodeBlock) == GIF_ERROR) {
			    PrintGifError();
			    exit(EXIT_FAILURE);
			}
			PrintCodeBlock(GifFile, CodeBlock, FALSE);
		    }
		}
		else if (LZCodesFlag) {
		    PrintLZCodes(GifFile);
		}
		else if (PixelFlag) {
		    Line = (GifPixelType *) malloc(GifFile->Image.Width *
						sizeof(GifPixelType));
		    for (i = 0; i < GifFile->Image.Height; i++) {
			if (DGifGetLine(GifFile, Line, GifFile->Image.Width)
			    == GIF_ERROR) {
			    PrintGifError();
			    exit(EXIT_FAILURE);
			}
			PrintPixelBlock(Line, GifFile->Image.Width, i == 0);
		    }
		    PrintPixelBlock(NULL, GifFile->Image.Width, FALSE);
		    free((char *) Line);
		}
		else if (RawFlag) {
		    Line = (GifPixelType *) malloc(GifFile->Image.Width *
						sizeof(GifPixelType));
		    for (i = 0; i < GifFile->Image.Height; i++) {
			if (DGifGetLine(GifFile, Line, GifFile->Image.Width)
			    == GIF_ERROR) {
			    PrintGifError();
			    exit(EXIT_FAILURE);
			}
			fwrite(Line, 1, GifFile->Image.Width, stdout);
		    }
		    free((char *) Line);
		}
		else {
		    /* Skip the image: */
		    if (DGifGetCode(GifFile, &CodeSize, &CodeBlock) == GIF_ERROR) {
			PrintGifError();
			exit(EXIT_FAILURE);
		    }
		    while (CodeBlock != NULL) {
			if (DGifGetCodeNext(GifFile, &CodeBlock) == GIF_ERROR) {
			    PrintGifError();
			    exit(EXIT_FAILURE);
			}
		    }

		}
		break;
	    case EXTENSION_RECORD_TYPE:
		if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
		    PrintGifError();
		    exit(EXIT_FAILURE);
		}
		if (!RawFlag) {
		    putchar('\n');
		    switch (ExtCode)
		    {
		    case COMMENT_EXT_FUNC_CODE:
			printf("GIF89 comment");
			break;
		    case GRAPHICS_EXT_FUNC_CODE:
			printf("GIF89 graphics control");
			break;
		    case PLAINTEXT_EXT_FUNC_CODE:
			printf("GIF89 plaintext");
			break;
		    case APPLICATION_EXT_FUNC_CODE:
			printf("GIF89 application block");
			break;
		    default:
			printf("Extension record of unknown type");
			break;
		    }
		    printf(" (Ext Code = %d [%c]):\n",
			   ExtCode, MAKE_PRINTABLE(ExtCode));
		    PrintExtBlock(Extension, TRUE);
		}
		for (;;) {
		    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
			PrintGifError();
			exit(EXIT_FAILURE);
		    }
		    if (Extension == NULL)
			break;
		    PrintExtBlock(Extension, FALSE);
		}
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		     /* Should be traps by DGifGetRecordType */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE);

    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError();
	exit(EXIT_FAILURE);
    }

    if (!RawFlag) printf("\nGif file terminated normally.\n");

    return 0;
}

/******************************************************************************
* Print the given CodeBlock - a string in pascal notation (size in first      *
* place). Save local information so printing can be performed continuously,   *
* or reset to start state if Reset. If CodeBlock is NULL, output is flushed   *
******************************************************************************/
static void PrintCodeBlock(GifFileType *GifFile, GifByteType *CodeBlock, int Reset)
{
    static int CrntPlace = 0, Print = TRUE;
    static long CodeCount = 0;
    int i, Percent, Len;
    long NumBytes;

    if (Reset || CodeBlock == NULL) {
	if (CodeBlock == NULL) {
	    if (CrntPlace > 0) {
		if (Print) printf("\n");
		CodeCount += CrntPlace - 16;
	    }
	    if (GifFile->Image.ColorMap)
		NumBytes = ((((long) GifFile->Image.Width) * GifFile->Image.Height)
				* GifFile->Image.ColorMap->BitsPerPixel) / 8;
	    else
		NumBytes = ((((long) GifFile->Image.Width) * GifFile->Image.Height)
				* GifFile->SColorMap->BitsPerPixel) / 8;
	    Percent = 100 * CodeCount / NumBytes;
	    printf("\nCompression ratio: %ld/%ld (%d%%).\n",
			CodeCount, NumBytes, Percent);
	    return;
	}
	CrntPlace = 0;
	CodeCount = 0;
	Print = TRUE;
    }

    Len = CodeBlock[0];
    for (i = 1; i <= Len; i++) {
	if (CrntPlace == 0) {
	    if (Print) printf("\n%05lxh:  ", CodeCount);
	    CodeCount += 16;
	}
#ifdef __MSDOS__
	if (kbhit() && ((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
#endif /* __MSDOS__ */
	if (Print) printf(" %02xh", CodeBlock[i]);
	if (++CrntPlace >= 16) CrntPlace = 0;
    }
}

/******************************************************************************
* Print the given Extension - a string in pascal notation (size in first      *
* place). Save local information so printing can be performed continuously,   *
* or reset to start state if Reset. If Extension is NULL, output is flushed   *
******************************************************************************/
static void PrintExtBlock(GifByteType *Extension, int Reset)
{
    static int CrntPlace = 0, Print = TRUE;
    static long ExtCount = 0;
    static char HexForm[49], AsciiForm[17];
    int i, Len;

    if (Reset || Extension == NULL) {
	if (Extension == NULL) {
	    if (CrntPlace > 0) {
		HexForm[CrntPlace * 3] = 0;
		AsciiForm[CrntPlace] = 0;
		if (Print) printf("\n%05lx: %-49s  %-17s\n",
				ExtCount, HexForm, AsciiForm);
		return;
	    }
	    else if (Print)
		printf("\n");
	}
	CrntPlace = 0;
	ExtCount = 0;
	Print = TRUE;
    }

    if (!Print) return;

    Len = Extension[0];
    for (i = 1; i <= Len; i++) {
	sprintf(&HexForm[CrntPlace * 3], " %02x", Extension[i]);
	sprintf(&AsciiForm[CrntPlace], "%c", MAKE_PRINTABLE(Extension[i]));
#ifdef __MSDOS__
	if (kbhit() && ((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
#endif /* __MSDOS__ */
	if (++CrntPlace == 16) {
	    HexForm[CrntPlace * 3] = 0;
	    AsciiForm[CrntPlace] = 0;
	    if (Print) printf("\n%05lx: %-49s  %-17s",
				ExtCount, HexForm, AsciiForm);
	    ExtCount += 16;
	    CrntPlace = 0;
	}
    }
}

/******************************************************************************
* Print the given PixelBlock of length Len.				      *
* Save local information so printing can be performed continuously,           *
* or reset to start state if Reset. If PixelBlock is NULL, output is flushed  *
******************************************************************************/
static void PrintPixelBlock(GifByteType *PixelBlock, int Len, int Reset)
{
    static int CrntPlace = 0, Print = TRUE;
    static long ExtCount = 0;
    static char HexForm[49], AsciiForm[17];
    int i;

    if (Reset || PixelBlock == NULL) {
	if (PixelBlock == NULL) {
	    if (CrntPlace > 0) {
		HexForm[CrntPlace * 3] = 0;
		AsciiForm[CrntPlace] = 0;
		if (Print) printf("\n%05lx: %-49s  %-17s\n",
				ExtCount, HexForm, AsciiForm);
	    }
	    else if (Print)
		printf("\n");
	}
	CrntPlace = 0;
	ExtCount = 0;
	Print = TRUE;
	if (PixelBlock == NULL) return;
    }

    if (!Print) return;

    for (i = 0; i < Len; i++) {
	sprintf(&HexForm[CrntPlace * 3], " %02x", PixelBlock[i]);
	sprintf(&AsciiForm[CrntPlace], "%c", MAKE_PRINTABLE(PixelBlock[i]));
#ifdef __MSDOS__
	if (kbhit() && ((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
#endif /* __MSDOS__ */
	if (++CrntPlace == 16) {
	    HexForm[CrntPlace * 3] = 0;
	    AsciiForm[CrntPlace] = 0;
	    if (Print) printf("\n%05lx: %-49s  %-17s",
				ExtCount, HexForm, AsciiForm);
	    ExtCount += 16;
	    CrntPlace = 0;
	}
    }
}

/******************************************************************************
* Print the image as LZ codes (each 12bits), until EOF marker is reached.     *
******************************************************************************/
static void PrintLZCodes(GifFileType *GifFile)
{
    int Code, Print = TRUE, CrntPlace = 0;
    long CodeCount = 0;

    do {
	if (Print && CrntPlace == 0) printf("\n%05lx:", CodeCount);
	if (DGifGetLZCodes(GifFile, &Code) == GIF_ERROR) {
	    PrintGifError();
	    exit(EXIT_FAILURE);
	}
	if (Print && Code >= 0)
	    printf(" %03x", Code);	      /* EOF Code is returned as -1. */
	CodeCount++;
	if (++CrntPlace >= 16) CrntPlace = 0;
#ifdef __MSDOS__
	if (kbhit() && ((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
#endif /* __MSDOS__ */
    }
    while (Code >= 0);
}
