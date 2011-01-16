/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Takes a multi-image gif and yields the overlay of all the images           *
******************************************************************************
* History:								     *
* 6 May 94 - Version 1.0 by Eric Raymond.				     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <dos.h>
#include <alloc.h>
#include <graphics.h>
#include <io.h>
#endif /* __MSDOS__ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "getarg.h"
#include "gif_lib.h"

#define PROGRAM_NAME	"gifovly"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

#ifdef SYSV
static char *VersionStr =
        "Gif compiler,\t\tEric S. Raymond\n\
	(C) Copyright 1992 Eric S. Raymond, all rights reserved.\n";
static char
    *CtrlStr = "GifOvly t%-TransparentColor!d h%-";
#else
static char
    *VersionStr =
	PROGRAM_NAME
	GIF_LIB_VERSION
	"	Eric Raymond,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1992 Eric Raymond.\n";
static char
    *CtrlStr =
	PROGRAM_NAME
	" t%-TransparentColor!d h%-";
#endif /* SYSV */

int main(int argc, char **argv)
{
    int	k;
    GifFileType *GifFileIn, *GifFileOut = (GifFileType *)NULL;
    SavedImage *bp;
    int	Error, TransparentColorFlag = FALSE, TransparentColor = 0,
	HelpFlag = FALSE;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&TransparentColorFlag, &TransparentColor,
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

    if ((GifFileIn = DGifOpenFileHandle(0)) == NULL
	|| DGifSlurp(GifFileIn) == GIF_ERROR
	|| ((GifFileOut = EGifOpenFileHandle(1)) == (GifFileType *)NULL))
    {
	PrintGifError();
	exit(EXIT_FAILURE);
    }

    GifFileOut->SWidth = GifFileIn->SWidth;
    GifFileOut->SHeight = GifFileIn->SHeight;
    GifFileOut->SColorResolution = GifFileIn->SColorResolution;
    GifFileOut->SBackGroundColor = GifFileIn->SBackGroundColor;
    GifFileOut->SColorMap = MakeMapObject(
				 GifFileIn->SColorMap->ColorCount,
				 GifFileIn->SColorMap->Colors);


    /* The output file will have exactly one image */
    MakeSavedImage(GifFileOut, &GifFileIn->SavedImages[0]);
    bp = &GifFileOut->SavedImages[0];
    for (k = 1; k < GifFileIn->ImageCount; k++)
    {
	register int	i, j;
	register unsigned char	*sp, *tp;

	SavedImage *ovp = &GifFileIn->SavedImages[k];

	for (i = 0; i < ovp->ImageDesc.Height; i++)
	{
	    tp = bp->RasterBits + (ovp->ImageDesc.Top + i) * bp->ImageDesc.Width + ovp->ImageDesc.Left;
	    sp = ovp->RasterBits + i * ovp->ImageDesc.Width;

	    for (j = 0; j < ovp->ImageDesc.Width; j++)
		if (!TransparentColorFlag || sp[j] != TransparentColor)
		    tp[j] = sp[j];
	}
    }

    if (EGifSpew(GifFileOut) == GIF_ERROR)
	PrintGifError();
    else if (DGifCloseFile(GifFileIn) == GIF_ERROR)
	PrintGifError();

    return 0;
}

/* gifovly.c ends here */
