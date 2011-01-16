/*
 * Skeleton file for generic GIF `filter' program --- sequentially read GIF
 * records from stdin, process them, send them out.  Most of the junk above
 * `int main' isn't needed for the skeleton, but is likely to be for what
 * you'll do with it.
 *
 * If you compile this, it will turn into an expensive GIF copying routine;
 * stdin to stdout with no changes and minimal validation.  Well, it's a
 * decent test of the low-level routines, anyway.
 *
 * Note: due to the vicissitudes of Lempel-Ziv compression, the output of this
 * copier may not be bitwise identical to its input.  This can happen if you
 * copy an image from a much more (or much *less*) memory-limited system; your
 * compression may use more (or fewer) bits.  The uncompressed rasters should,
 * however, be identical (you can check this with icon2gif -d).
 */

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

#define PROGRAM_NAME	"giffiltr"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */


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

/******************************************************************************
* Main sequence								      *
******************************************************************************/
int main(int argc, char **argv)
{
    GifFileType *GifFileIn = NULL, *GifFileOut = NULL;
    GifRecordType RecordType;
    int CodeSize, ExtCode;
    GifByteType *CodeBlock, *Extension;

    /*
     * Command-line processing goes here.
     */

    /* Use the stdin as input (note this also read screen descriptor in: */
    if ((GifFileIn = DGifOpenFileHandle(0)) == NULL)
	QuitGifError(GifFileIn, GifFileOut);

    /* Use the stdout as output: */
    if ((GifFileOut = EGifOpenFileHandle(1)) == NULL)
	QuitGifError(GifFileIn, GifFileOut);

    /* And dump out its screen information: */
    if (EGifPutScreenDesc(GifFileOut,
	GifFileIn->SWidth, GifFileIn->SHeight,
	GifFileIn->SColorResolution, GifFileIn->SBackGroundColor,
	GifFileIn->SColorMap) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);

    /* Scan the content of the input GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
	    QuitGifError(GifFileIn, GifFileOut);

	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		/* Put image descriptor to out file: */
		if (EGifPutImageDesc(GifFileOut,
		    GifFileIn->Image.Left, GifFileIn->Image.Top,
		    GifFileIn->Image.Width, GifFileIn->Image.Height,
		    GifFileIn->Image.Interlace,
		    GifFileIn->Image.ColorMap) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);

		/* Now read image itself in decoded form as we dont really   */
		/* care what we have there, and this is much faster.	     */
		if (DGifGetCode(GifFileIn, &CodeSize, &CodeBlock) == GIF_ERROR ||
		    EGifPutCode(GifFileOut, CodeSize, CodeBlock) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		while (CodeBlock != NULL) {
		    if (DGifGetCodeNext(GifFileIn, &CodeBlock) == GIF_ERROR ||
			EGifPutCodeNext(GifFileOut, CodeBlock) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		}
		break;
	    case EXTENSION_RECORD_TYPE:
		/* Skip any extension blocks in file: */
		if (DGifGetExtension(GifFileIn, &ExtCode, &Extension) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);
		if (EGifPutExtension(GifFileOut, ExtCode, Extension[0],
							Extension + 1) == GIF_ERROR)
		    QuitGifError(GifFileIn, GifFileOut);

		/* No support to more than one extension blocks, so discard: */
		while (Extension != NULL) {
		    if (DGifGetExtensionNext(GifFileIn, &Extension) == GIF_ERROR)
			QuitGifError(GifFileIn, GifFileOut);
		}
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:	     /* Should be trapped by DGifGetRecordType */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE);

    if (DGifCloseFile(GifFileIn) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);
    if (EGifCloseFile(GifFileOut) == GIF_ERROR)
	QuitGifError(GifFileIn, GifFileOut);

    return 0;
}
