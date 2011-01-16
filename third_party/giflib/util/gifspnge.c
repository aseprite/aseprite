/*
 * Skeleton file for generic GIF `sponge' program --- slurp a GIF into core,
 * operate on it, spew it out again.  Most of the junk above `int main' isn't
 * needed for the skeleton, but is likely to be for what you'll do with it.
 *
 * If you compile this, it will turn into an expensive GIF copying routine;
 * stdin to stdout with no changes and minimal validation.  Well, it's a
 * decent test of DGifSlurp() and EGifSpew(), anyway.
 *
 * Note: due to the vicissitudes of Lempel-Ziv compression, the output of this
 * copier may not be bitwise identical to its input.  This can happen if you
 * copy an image from a much more (or much *less*) memory-limited system; your
 * compression may use more (or fewer) bits.  The uncompressed rasters should,
 * however, be identical (you can check this with icon2gif -d).
 *
 *					Eric S. Raymond
 *					esr@snark.thyrsus.com
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

#define PROGRAM_NAME	"gifspnge"

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */

int main(int argc, char **argv)
{
    int	i;
    GifFileType *GifFileIn, *GifFileOut = (GifFileType *)NULL;

    if ((GifFileIn = DGifOpenFileHandle(0)) == NULL
	|| DGifSlurp(GifFileIn) == GIF_ERROR
	|| ((GifFileOut = EGifOpenFileHandle(1)) == (GifFileType *)NULL))
    {
	PrintGifError();
	exit(EXIT_FAILURE);
    }

    /*
     * Your operations on in-core structures go here.  
     * This code just copies the header and each image from the incoming file.
     */
    GifFileOut->SWidth = GifFileIn->SWidth;
    GifFileOut->SHeight = GifFileIn->SHeight;
    GifFileOut->SColorResolution = GifFileIn->SColorResolution;
    GifFileOut->SBackGroundColor = GifFileIn->SBackGroundColor;
    GifFileOut->SColorMap = MakeMapObject(
				 GifFileIn->SColorMap->ColorCount,
				 GifFileIn->SColorMap->Colors);

    for (i = 0; i < GifFileIn->ImageCount; i++)
	(void) MakeSavedImage(GifFileOut, &GifFileIn->SavedImages[i]);

    /*
     * Note: don't do DGifCloseFile early, as this will
     * deallocate all the memory containing the GIF data!
     *
     * Further note: EGifSpew() doesn't try to validity-check any of this
     * data; it's *your* responsibility to keep your changes consistent.
     * Caveat hacker!
     */
    if (EGifSpew(GifFileOut) == GIF_ERROR)
	PrintGifError();
    else if (DGifCloseFile(GifFileIn) == GIF_ERROR)
	PrintGifError();

    return 0;
}

/* gifspnge.c ends here */
