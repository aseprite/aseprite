
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
int __gba_multiboot;
#include <gba_input.h>
#include <gba_video.h>
#endif

#define RGB8(r,g,b)	( (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3) )

#ifndef __MSDOS__
#include <stdlib.h>
#endif
#include <ctype.h>
#include <string.h>
#include "../lib/gif_lib.h"

#include "res/cover.c"
#include "res/porsche-240x160.c"
#include "res/x-trans.c"

const short InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
const short InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */

int readFunc(GifFileType* GifFile, GifByteType* buf, int count)
{
    char* ptr = GifFile->UserData;
    memcpy(buf, ptr, count);
    GifFile->UserData = ptr + count;
    return count;
}

void CopyLine(void* dst, void* src, int count)
{
    do
    {
	*(short*) dst = *(short*) src;
	src = (u8*)src + 2;
	dst = (u8*)dst + 2;
	count -= 2;
    }
    while (count >= 0);
}

int DGifGetLineByte(GifFileType *GifFile, GifPixelType *Line, int LineLen)
{
    GifPixelType LineBuf[240];
    CopyLine(LineBuf, Line, LineLen);
    int result = DGifGetLine(GifFile, LineBuf, LineLen);
    CopyLine(Line, LineBuf, LineLen);
    return result;
}

#define GAMMA(x)	(x)

#ifdef _NO_FILEIO
#define PrintGifError()
#endif

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
#ifdef _WIN32
int DecodeGif(const u8 *userData, u8 ScreenBuff[160][240], BITMAPINFO* pBMI)
#else
int DecodeGif(const u8 *userData, u8 ScreenBuff[160][240], u16* Palette)
#endif
{
    int	i, j, Row, Col, Width, Height, ExtCode, Count;
    GifRecordType RecordType;
    GifByteType *Extension;
    GifFileType *GifFile;
    ColorMapObject *ColorMap;
    
    if ((GifFile = DGifOpen(userData, readFunc)) == NULL) {
	PrintGifError();
	return EXIT_FAILURE;
    }
    
    for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
	ScreenBuff[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) {
	memcpy(ScreenBuff[i], ScreenBuff[0], GifFile->SWidth);
    }
    
    /* Scan the content of the GIF file and load the image(s) in: */
    do {
	if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
	    PrintGifError();
	    return EXIT_FAILURE;
	}
	switch (RecordType) {
	case IMAGE_DESC_RECORD_TYPE:
	    if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
		PrintGifError();
		return EXIT_FAILURE;
	    }
	    Row = GifFile->Image.Top; /* Image Position relative to Screen. */
	    Col = GifFile->Image.Left;
	    Width = GifFile->Image.Width;
	    Height = GifFile->Image.Height;
	    
	    // Update Color map
	    ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);
	    
#ifdef _WIN32
	    ZeroMemory(pBMI, sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD));
	    pBMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	    pBMI->bmiHeader.biWidth = Width;
	    pBMI->bmiHeader.biHeight = -Height;     // negative for top-down bitmap
	    pBMI->bmiHeader.biPlanes = 1;
	    pBMI->bmiHeader.biBitCount = 8;
	    pBMI->bmiHeader.biClrUsed = 256;
	    pBMI->bmiHeader.biClrImportant = 256;
	    i = ColorMap->ColorCount;
	    while (--i >= 0)
	    {
		RGBQUAD rgb;
		GifColorType* color = &ColorMap->Colors[i];
		rgb.rgbRed = color->Red;
		rgb.rgbGreen = color->Green;
		rgb.rgbBlue = color->Blue;
		rgb.rgbReserved = 0;
		pBMI->bmiColors[i] = rgb;
	    }
#else
	    i = ColorMap->ColorCount;
	    while (--i >= 0)
	    {
		GifColorType* pColor = &ColorMap->Colors[i];
		Palette[i] = RGB8(GAMMA(pColor->Red), GAMMA(pColor->Green), GAMMA(pColor->Blue));
	    }
#endif
	    if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
		GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
		return EXIT_FAILURE;
	    }
	    if (GifFile->Image.Interlace) {
		/* Need to perform 4 passes on the images: */
		for (Count = i = 0; i < 4; i++)
		    for (j = Row + InterlacedOffset[i]; j < Row + Height;
		    j += InterlacedJumps[i]) {
			if (DGifGetLineByte(GifFile, &ScreenBuff[j][Col],
			    Width) == GIF_ERROR) {
			    PrintGifError();
			    return EXIT_FAILURE;
			}
		    }
	    }
	    else {
		for (i = 0; i < Height; i++) {
		    if (DGifGetLineByte(GifFile, &ScreenBuff[Row++][Col],
			Width) == GIF_ERROR) {
			PrintGifError();
			return EXIT_FAILURE;
		    }
		}
	    }
	    break;
	case EXTENSION_RECORD_TYPE:
	    /* Skip any extension blocks in file: */
	    if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
		PrintGifError();
		return EXIT_FAILURE;
	    }
	    while (Extension != NULL) {
		if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
		    PrintGifError();
		    return EXIT_FAILURE;
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
    
    /* Close file when done */
    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError();
	return EXIT_FAILURE;
    }
    
    return 0;
}

u16* paletteMem = (u16*)0x5000000;

short x = 120;
short y = 60;
short n = 0;
short v = 0;

int WaitInput(void)
{
    volatile int keys;
    do
    {
        keys = ~REG_KEYINPUT;
	
	if (keys & KEY_B)
	{
	    REG_BG2PA = 0x0080;
	    REG_BG2PD = 0x0080;
	    
	    if (keys & KEY_RIGHT)
	    {
		if (x < 240)
		    ++x;
	    }
	    else if (keys & KEY_LEFT)
	    {
		if (x > 0)
		    --x;
	    }
	    if (keys & KEY_DOWN)
	    {
		if (y < 160)
		    ++y;
	    }
	    else if (keys & KEY_UP)
	    {
		if (y > 0)
		    --y;
	    }
	    REG_BG2X = x << 7;
	    REG_BG2Y = y << 7;
	}
	else
	{
	    // Normal: no pan offset
	    REG_BG2PA = 0x0100;
	    REG_BG2PD = 0x0100;
	    REG_BG2X = 0;
	    REG_BG2Y = 0;
	}
	while (REG_VCOUNT != 160);
    }
    while ((keys & (KEY_A|KEY_L|KEY_R)) == 0);
    
    if (keys & KEY_L)
	if (n > 0) --n; else n = 2;
    else
	if (n < 2) ++n; else n = 0;
    
    return keys;
}

int main(void)
{
    SetMode(4 | BG2_ENABLE); /* Enable mode 4 and turn on background 2. */ 
    do
    {
	memset(MODE3_FB, 0, 240*160*sizeof(short));
	const u8* pict;
	switch (n)
	{
	case 1:
	    pict = cover;
	    break;
	case 2:
	    pict = x_trans;
	    break;
	default:
	    pict = porsche_240x160;
	    break;
	}
	/* Convert GIF to 256-color picture */
        DecodeGif(pict, (void*) MODE3_FB, paletteMem);
    }
    while ((WaitInput() & (KEY_START|KEY_SELECT)) == 0);
	return 0;
}
