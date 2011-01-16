/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to assemble/disassemble GIF files: disassembles multi image file   *
* into seperated files, or assembles few single image GIF files into one.    *
* Options:								     *
* -q : quiet printing mode.						     *
* -A : assemble few files into one as an animation gif.                 *
* -a : assemble few files into one (default)				     *
* -d name : disassmble given GIF file into seperate files derived from name. *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 7 Jul 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <alloc.h>
#endif /* __MSDOS__ */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <ctype.h>
#include <string.h>
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"GifAsm"
#define GIF_ASM_NAME   "NETSCAPE2.0"
#define COMMENT_GIF_ASM    "(c) Gershon Elber, GifLib"

#define SQR(x)     ((x) * (x))

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "GifAsm q%- A%-Delay!d a%- d%-OutFileName!s h%- GifFile(s)!*s";
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
    " q%- A%-Delay!d a%- d%-OutFileName!s h%- GifFile(s)!*s";
#endif /* SYSV */

static int
    AsmGifAnimFlag = FALSE,
    AsmGifAnimNumIters = 1,
    AsmGifAnimDelay = 0,
    AsmGifAnimUserWait = FALSE,
    AsmFlag = FALSE;

#if FALSE
 /* This is apparently not yet an implemented feature of the program */
static void DoAssemblyGifAnim(int NumFiles, char **FileNames);
#endif /* FALSE */
static void DoAssembly(int NumFiles, char **FileNames);
static void DoDisassembly(char *InFileName, char *OutFileName);
static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	Error, NumFiles, DisasmFlag = FALSE, HelpFlag = FALSE;
    char **FileNames = NULL, *OutFileName;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifQuietPrint,
              &AsmGifAnimFlag, &AsmGifAnimDelay,
              &AsmFlag, &DisasmFlag, &OutFileName,
              &HelpFlag, &NumFiles, &FileNames)) != FALSE) {
	GAPrintErrMsg(Error);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (HelpFlag) {
	fprintf(stderr, VersionStr);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_SUCCESS);
    }

    if (!AsmFlag && !AsmGifAnimFlag && !DisasmFlag)
        AsmFlag = TRUE; /* Make default - assemble. */
    if ((AsmFlag || AsmGifAnimFlag) && NumFiles < 2) {
	GIF_MESSAGE("At list two GIF files are required to assembly operation.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }
    if (!AsmFlag && !AsmGifAnimFlag && NumFiles > 1) {
	GIF_MESSAGE("Error in command line parsing - one GIF file please.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (AsmFlag || AsmGifAnimFlag)
        DoAssembly(NumFiles, FileNames);
    else
	DoDisassembly(NumFiles == 1 ? *FileNames : NULL, OutFileName);

    return 0;
}

/******************************************************************************
* Perform the assembly operation - take few input files into one output.      *
******************************************************************************/
static void DoAssembly(int NumFiles, char **FileNames)
{
    int    i, j, k, Len = 0, ExtCode, ReMapColor[256];
    ColorMapObject FirstColorMap;
    GifRecordType RecordType;
    GifByteType *Line = NULL, *Extension;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

    /* Open stdout for the output file: */
    if ((GifFileOut = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFileIn, GifFileOut);

    /* Scan the content of the GIF file and load the image(s) in: */
    for (i = 0; i < NumFiles; i++) {
	if ((GifFileIn = DGifOpenFileName(FileNames[i])) == NULL)
	    QuitGifError(GifFileIn, GifFileOut);

	/* And dump out screen descriptor iff its first image.	*/
	if (i == 0) {
	    if (EGifPutScreenDesc(GifFileOut,
		GifFileIn->SWidth, GifFileIn->SHeight,
		GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
		GifFileIn->SColorMap) == GIF_ERROR)
		QuitGifError(GifFileIn, GifFileOut);

        FirstColorMap = *GifFileIn->SColorMap;
        FirstColorMap.Colors =
        (GifColorType *) malloc(sizeof(GifColorType) *
                    FirstColorMap.ColorCount);
        memcpy(FirstColorMap.Colors, GifFileIn->SColorMap->Colors,
           sizeof(GifColorType) * FirstColorMap.ColorCount);
 
        if (AsmGifAnimFlag) {
        unsigned char ExtStr[3];
 
        ExtStr[0] = AsmGifAnimNumIters % 256;
        ExtStr[1] = AsmGifAnimNumIters / 256;
        ExtStr[2] = 0;
 
        /* Dump application+comment blocks. */
        EGifPutExtensionFirst(GifFileOut, APPLICATION_EXT_FUNC_CODE,
                      strlen(GIF_ASM_NAME), GIF_ASM_NAME);
        EGifPutExtensionLast(GifFileOut, APPLICATION_EXT_FUNC_CODE,
                     3, ExtStr);
        EGifPutExtension(GifFileOut, COMMENT_EXT_FUNC_CODE,
                 strlen(COMMENT_GIF_ASM), COMMENT_GIF_ASM);
        }
        for (j = 0; j < GifFileIn->SColorMap->ColorCount; j++)
        ReMapColor[j] = j;
    }
    else {
        /* Compute the remapping of this image's color map to first one. */
        for (j = 0; j < GifFileIn->SColorMap->ColorCount; j++) {
        int MinIndex = 0,
            MinDist = 3 * 256 * 256;
        GifColorType *CMap1 = FirstColorMap.Colors,
                     *c = GifFileIn->SColorMap->Colors;
 
        /* Find closest color in first color map to this color. */
        for (k = 0; k < FirstColorMap.ColorCount; k++) {
            int Dist = SQR(c[j].Red - CMap1[k].Red) +
                   SQR(c[j].Green - CMap1[k].Green) +
                   SQR(c[j].Blue - CMap1[k].Blue);
 
            if (MinDist > Dist) {
            MinDist = Dist;
            MinIndex = k;
            }
        }
        ReMapColor[j] = MinIndex;
        }
    }

	do {
        if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
		QuitGifError(GifFileIn, GifFileOut);

	    switch (RecordType) {
		case IMAGE_DESC_RECORD_TYPE:
		    if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
 
            if (AsmGifAnimFlag) {
            static unsigned char
                ExtStr[4] = { 0x04, 0x00, 0x00, 0xff };
 
            ExtStr[0] = AsmGifAnimUserWait ? 0x06 : 0x04;
            ExtStr[1] = AsmGifAnimDelay % 256;
            ExtStr[2] = AsmGifAnimDelay / 256;
 
            /* Dump graphics control block. */
            EGifPutExtension(GifFileOut, GRAPHICS_EXT_FUNC_CODE,
                     4, ExtStr);
            }
 
            if (i == 0) {
            Len = sizeof(GifPixelType) * GifFileIn->Image.Width;
            if ((Line = (GifRowType) malloc(Len)) == NULL)
                GIF_EXIT("Failed to allocate memory required, aborted.");
            }
 
		    /* Put image descriptor to out file: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			GifFileIn->Image.Interlace,
			GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);

            /* Now read image itself and remap to global color map.  */
            for (k = 0; k < GifFileIn->Image.Height; k++) {
            if (DGifGetLine(GifFileIn, Line, Len) != GIF_ERROR) {
                for (j = 0; j < Len; j++)
                Line[j] = ReMapColor[Line[j]];
 
                if (EGifPutLine(GifFileOut, Line, Len) == GIF_ERROR)
                QuitGifError(GifFileIn, GifFileOut);
            }
            else
			    QuitGifError(GifFileIn, GifFileOut);
            }
		    break;
		case EXTENSION_RECORD_TYPE:
		    /* Skip any extension blocks in file: */
		    if (DGifGetExtension(GifFileIn, &ExtCode, &Extension)
			== GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		    if (EGifPutExtension(GifFileOut, ExtCode, Extension[0],
						       Extension) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);

		    /* No support to more than one extension blocks, discard.*/
		    while (Extension != NULL)
			if (DGifGetExtensionNext(GifFileIn, &Extension)
			    == GIF_ERROR)
				QuitGifError(GifFileIn, GifFileOut);
		    break;
		case TERMINATE_RECORD_TYPE:
		    break;
		default:	    /* Should be traps by DGifGetRecordType. */
		    break;
	    }
	}
	while (RecordType != TERMINATE_RECORD_TYPE);

	if (DGifCloseFile(GifFileIn) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);
    }

    if (EGifCloseFile(GifFileOut) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);
}

/******************************************************************************
* Perform the disassembly operation - take one input files into few output.   *
******************************************************************************/
static void DoDisassembly(char *InFileName, char *OutFileName)
{
    int	ExtCode, CodeSize, FileNum = 0, FileEmpty;
    GifRecordType RecordType;
    char CrntFileName[80];
    GifByteType *Extension, *CodeBlock;
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;

#ifdef __MSDOS__
    int i;
    char *p;

    /* If name has type postfix, strip it out, and make sure name is less    */
    /* or equal to 6 chars, so we will have 2 chars in name for numbers.     */
    for (i = 0; i < (int)strlen(OutFileName);  i++)/* Make sure all is upper case.*/
	if (islower(OutFileName[i]))
	    OutFileName[i] = toupper(OutFileName[i]);

    if ((p = strrchr(OutFileName, '.')) != NULL && strlen(p) <= 4) p[0] = 0;
    if ((p = strrchr(OutFileName, '/')) != NULL ||
	(p = strrchr(OutFileName, '\\')) != NULL ||
	(p = strrchr(OutFileName, ':')) != NULL) {
	if (strlen(p) > 7) p[7] = 0;  /* p includes the '/', '\\', ':' char. */
    }
    else {
	/* Only name is given for current directory: */
	if (strlen(OutFileName) > 6) OutFileName[6] = 0;
    }
#endif /* __MSDOS__ */

    /* Open input file: */
    if (InFileName != NULL) {
	if ((GifFileIn = DGifOpenFileName(InFileName)) == NULL)
	    QuitGifError(GifFileIn, GifFileOut);
    }
    else {
	/* Use the stdin instead: */
	if ((GifFileIn = DGifOpenFileHandle(0)) == NULL)
	    QuitGifError(GifFileIn, GifFileOut);
    }

    /* Scan the content of GIF file and dump image(s) to seperate file(s): */
    do {
	sprintf(CrntFileName, "%s%02d.gif", OutFileName, FileNum++);
	if ((GifFileOut = EGifOpenFileName(CrntFileName, TRUE)) == NULL)
	    QuitGifError(GifFileIn, GifFileOut);
	FileEmpty = TRUE;

	/* And dump out its exactly same screen information: */
	if (EGifPutScreenDesc(GifFileOut,
	    GifFileIn->SWidth, GifFileIn->SHeight,
	    GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
	    GifFileIn->SColorMap) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);

	do {
	    if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
		QuitGifError(GifFileIn, GifFileOut);

	    switch (RecordType) {
		case IMAGE_DESC_RECORD_TYPE:
		    FileEmpty = FALSE;
		    if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		    /* Put same image descriptor to out file: */
		    if (EGifPutImageDesc(GifFileOut,
			GifFileIn->Image.Left, GifFileIn->Image.Top,
			GifFileIn->Image.Width, GifFileIn->Image.Height,
			GifFileIn->Image.Interlace,
			GifFileIn->Image.ColorMap) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);

		    /* Now read image itself in decoded form as we dont      */
		    /* really care what is there, and this is much faster.   */
		    if (DGifGetCode(GifFileIn, &CodeSize, &CodeBlock) == GIF_ERROR
		     || EGifPutCode(GifFileOut, CodeSize, CodeBlock) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		    while (CodeBlock != NULL)
			if (DGifGetCodeNext(GifFileIn, &CodeBlock) == GIF_ERROR ||
			    EGifPutCodeNext(GifFileOut, CodeBlock) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		    break;
		case EXTENSION_RECORD_TYPE:
		    FileEmpty = FALSE;
		    /* Skip any extension blocks in file: */
		    if (DGifGetExtension(GifFileIn, &ExtCode, &Extension)
			== GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		    if (EGifPutExtension(GifFileOut, ExtCode, Extension[0],
							Extension) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);

		    /* No support to more than one extension blocks, discard.*/
		    while (Extension != NULL)
			if (DGifGetExtensionNext(GifFileIn, &Extension)
			    == GIF_ERROR)
			    QuitGifError(GifFileIn, GifFileOut);
		    break;
		case TERMINATE_RECORD_TYPE:
		    break;
		default:	    /* Should be traps by DGifGetRecordType. */
		    break;
	    }
	}
	while (RecordType != IMAGE_DESC_RECORD_TYPE &&
	       RecordType != TERMINATE_RECORD_TYPE);

	if (EGifCloseFile(GifFileOut) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);
	if (FileEmpty) {
	    /* Might happen on last file - delete it if so: */
	    unlink(CrntFileName);
	}
   }
    while (RecordType != TERMINATE_RECORD_TYPE);

    if (DGifCloseFile(GifFileIn) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);
}

/******************************************************************************
* Close both input and output file (if open), and exit.			      *
******************************************************************************/
static void QuitGifError(GifFileType *GifFileIn, GifFileType *GifFileOut)
{
    PrintGifError();
    if (GifFileIn != NULL) DGifCloseFile(GifFileIn);
    if (GifFileOut != NULL) EGifCloseFile(GifFileOut);
    exit(EXIT_FAILURE);
}
