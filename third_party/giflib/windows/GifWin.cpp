/*********************************************************
*
*	File:	GifWin.cpp
*	Title:	Graphics Interchange Format implementation
*
*	Author:	Lennie Araki
*	Date:	24-Nov-1999
*
*	This class is a thin wrapper around the open source
*	giflib-1.4.0 for opening, parsing and displaying
*	Compuserve GIF files on Windows.
*
*	The baseline code was derived from fragments extracted
*	from the sample programs gif2rgb.c and giftext.c.
*	Added support for local/global palettes, transparency
*	and "dispose" methods to improve display compliance
*	with GIF89a.
*
*	Copyright (c) 1999 CallWave, Inc.
*		CallWave, Inc.
*		136 W. Canon Perdido Suite A
*		Santa Barbara, CA 93101
*
*	Licensed under the terms laid out in the libungif 
*	COPYING file.
*
*********************************************************/

#include "stdafx.h"
#include <windowsx.h>
#include "GifWin.h"
extern "C"
{
    #include "gif_lib.h"
}

#define LOCAL	static

//
//	Implements the GIF89a specification with the following omissions:
//
//	Section 18. Logical Screen Descriptor:
//		Pixel Aspect Ratio is ignored - square pixels assumed (1:1)
//	Section 23. Graphic Control Extension:
//		User Input Flag is ignored - could be added but not very useful
//	Section 25. Plain Text Extension
//		Not implemented - would require embedding fonts and text drawing
//		code to be added
//	Section 26. Application Extension
//		Not implemented.  Note: this includes Netscape 2.0 looping
//		extensions
//

//   _______________________________
//	|  reserved | disposal  |u_i| t |
//	|___|___|___|___|___|___|___|___|
//
#define GIF_TRANSPARENT		0x01
#define GIF_USER_INPUT		0x02
#define GIF_DISPOSE_MASK	0x07
#define GIF_DISPOSE_SHIFT	2

#define GIF_NOT_TRANSPARENT	-1

#define GIF_DISPOSE_NONE	0		// No disposal specified. The decoder is
									// not required to take any action.
#define GIF_DISPOSE_LEAVE	1		// Do not dispose. The graphic is to be left
									// in place.
#define GIF_DISPOSE_BACKGND	2		// Restore to background color. The area used by the
									// graphic must be restored to the background color.

#define GIF_DISPOSE_RESTORE	3		// Restore to previous. The decoder is required to
									// restore the area overwritten by the graphic with
									// what was there prior to rendering the graphic.


// Initialize BITMAPINFO 
LOCAL void InitBitmapInfo(LPBITMAPINFO pBMI, int cx, int cy)
{
    ::ZeroMemory(pBMI, sizeof(BITMAPINFOHEADER));
    pBMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBMI->bmiHeader.biWidth = cx;
	pBMI->bmiHeader.biHeight = -cy;     // negative for top-down bitmap
	pBMI->bmiHeader.biPlanes = 1;
	pBMI->bmiHeader.biBitCount = 24;
   	pBMI->bmiHeader.biClrUsed = 256;
}

// Copy GIF ColorMap into Windows BITMAPINFO
LOCAL void CopyColorMap(ColorMapObject* pColorMap, LPBITMAPINFO pBMI)
{
	int iLen = pColorMap->ColorCount;
	ASSERT( iLen <= 256 );
	int iCount = min(iLen, 256);
	for (int i = 0; i < iCount; i++)
	{
		BYTE red = pColorMap->Colors[i].Red;
		BYTE green = pColorMap->Colors[i].Green;
		BYTE blue = pColorMap->Colors[i].Blue;
		TRACE("%3d: %02xh %02xh %02xh   ", i, red, green, blue);
		pBMI->bmiColors[i].rgbRed = red;
		pBMI->bmiColors[i].rgbGreen = green;
		pBMI->bmiColors[i].rgbBlue = blue;
		pBMI->bmiColors[i].rgbReserved = 0;
		if (i % 4 == 3)
			TRACE("\n");
	}
	TRACE("\n");
}

#define DWORD_PAD(x)		(((x) + 3) & ~3)

// Copy bytes from source to destination  skipping transparent bytes
LOCAL void CopyGIF(LPBYTE pDst, LPBYTE pSrc, int width, const int transparent, GifColorType* pColorTable)
{
	ASSERT( pColorTable );
	if (width)
	{
		do
		{
			BYTE b = *pSrc++;
			if (b != transparent)
			{
				// Translate to 24-bit RGB value if not transparent
				const GifColorType* pColor = pColorTable + b;
				pDst[0] = pColor->Blue;
				pDst[1] = pColor->Green;
				pDst[2] = pColor->Red;
			}
			// Skip to next pixel
			pDst += 3;
		}
		while (--width);
	}
}

// Fix pixels in 24-bit GIF buffer
LOCAL void FillGIF(LPBYTE pDst, const COLORREF rgb, int width)
{
	if (width)
	{
		do
		{
			pDst[0] = GetBValue(rgb);
			pDst[1] = GetGValue(rgb);
			pDst[2] = GetRValue(rgb);
			pDst += 3;
		}
		while (--width);
	}
}

// Constructor/destructor
CGIFWin::CGIFWin()
{
	m_pGifFile = NULL;
	m_pBits = NULL;

	// Clear bitmap information
	::ZeroMemory(&m_bmiGlobal, sizeof(m_bmiGlobal));
	::ZeroMemory(&m_bmiDisplay, sizeof(m_bmiDisplay));

	//
	// Per Section 11 of GIF spec:
	// If no color table is available at all, the decoder is free to use a 
	// system color table or a table of its own. In that case, the decoder 
	// may use a color table with as many colors as its hardware is able 
	// to support; it is recommended that such a table have black and
	// white as its first two entries, so that monochrome images can be 
	// rendered adequately.
	//
	const RGBQUAD rgbWhite = { 255, 255, 255, 0 };
	const RGBQUAD rgbBlack = { 0, 0, 0, 0 };
	m_bmiGlobal.bmi.bmiColors[0] = rgbBlack;
	for (int i = 1; i < 256; ++i)
	{
		m_bmiGlobal.bmi.bmiColors[i] = rgbWhite;
	}
}

CGIFWin::~CGIFWin()
{
	TRACE("*** CGIFWin destructor called ***\n");
	Close();
}

// Open GIF file and allocate "screen" buffer
int CGIFWin::Open(LPCTSTR pszGifFileName, COLORREF rgbTransparent)
{
	m_rgbBackgnd = m_rgbTransparent = rgbTransparent;

	// First close and delete previous GIF (if open)
	Close();

    m_pGifFile = ::DGifOpenFileName(pszGifFileName);
	int iResult = -1;
    if (m_pGifFile)
    {
		const int cxScreen =  m_pGifFile->SWidth;
		const int cyScreen = m_pGifFile->SHeight;

		TRACE("\n%s:\n\n\tScreen Size - Width = %d, Height = %d.\n", pszGifFileName, cxScreen, cyScreen);
		TRACE("\tColorResolution = %d, BackGround = %d.\n", m_pGifFile->SColorResolution, m_pGifFile->SBackGroundColor);

		// Allocate buffer big enough for 2 screens + 1 line
		// Use 24-bit (3-bytes per pixel) to correctly handle local palettes
		const DWORD dwRowBytes = DWORD_PAD(cxScreen * 3);
		const DWORD dwScreen = dwRowBytes * cyScreen;
		m_pBits = (LPBYTE) GlobalAllocPtr(GHND, dwScreen * 2 + dwRowBytes);
		iResult = -2;
		if (m_pBits)
		{
			// Fill in current and next image with background color
			for (int y = 0; y < cyScreen * 2; ++y)
			{
				::FillGIF(m_pBits + y * dwRowBytes, rgbTransparent, cxScreen);
			}

			::InitBitmapInfo(&m_bmiGlobal.bmi, cxScreen, cyScreen);

			if (m_pGifFile->SColorMap)
			{
				TRACE("\tGlobal Color Map:\n");
				::CopyColorMap(m_pGifFile->SColorMap, &m_bmiGlobal.bmi);
				GifColorType* pColor = m_pGifFile->SColorMap->Colors + m_pGifFile->SBackGroundColor;
				m_rgbBackgnd = RGB(pColor->Red, pColor->Green, pColor->Blue);
			}
			iResult = 0;
			m_iImageNum = 0;
            m_uLoopCount = 0U;
		}
	}
	return iResult;
}

// Close the GIF file and free resources allocated by libgif
void CGIFWin::Close()
{
	// Close GIF file if opened
	if (m_pGifFile)
	{
		int iError = DGifCloseFile(m_pGifFile);
		if (iError == GIF_ERROR)
		{
			TRACE("DGifCloseFile error=%d\n", GifLastError());
		}
		m_pGifFile = NULL;
	}
	// Free memory if allocated
	if (m_pBits)
	{
		GlobalFreePtr(m_pBits);
		m_pBits = NULL;
	}
}

//
//  Draw entire GIF to a Windows Device Context
//      iFactor Percent Ratio
//      -3      25%     (1:4)
//      -2      33%     (1:3)
//      -1      50%     (1:2)
//       0      100%    (1:1)
//       1      200%    (2:1)
//       2      300%    (3:1)
//       3      400%    (4:1)
//
int CGIFWin::Draw(HDC hDC, LPCRECT pRect, int iFactor /*=0*/)
{
	int iResult = 0;
	if (m_pGifFile && m_pBits)
	{
		const int Width =  m_pGifFile->SWidth;
		const int Height = m_pGifFile->SHeight;
        int zoomWidth = Width;
        int zoomHeight = Height;
        if (iFactor < 0)
        {
            zoomWidth /= (1 - iFactor);
            zoomHeight /= (1 - iFactor);

        }
        else if (iFactor > 0)
        {
            zoomWidth *= (1 + iFactor);
            zoomHeight *= (1 + iFactor);
        }

		int x, y;
		if (pRect)
		{
			// Center image in rectangle
			x = (pRect->right - pRect->left - zoomWidth) / 2 + pRect->left;
			y = (pRect->bottom - pRect->top - zoomHeight) / 2 + pRect->top;
		}
		else
		{
			// Draw image at top-left
			x = y = 0;
		}

		if (Width && Height)
		{
            if (iFactor < 0)
            {
                HBITMAP hBitmap = CreateMappedBitmap(NULL, 0, 1 - iFactor);
                if (hBitmap)
                {
	                HDC hdcMem = ::CreateCompatibleDC(hDC);
                    if (hdcMem)
                    {
	                    HBITMAP hOldBm = (HBITMAP) ::SelectObject(hdcMem, hBitmap);

	                    // Blast bits from memory DC to target DC.
	                    iResult = ::BitBlt(hDC, x, y, zoomWidth, zoomHeight, hdcMem, 0, 0, SRCCOPY);

                        ::SelectObject(hdcMem, hOldBm);
                        ::DeleteDC(hdcMem);
                    }
                    ::DeleteObject(hBitmap);
                }
            }
            else // (iFactor >= 0)
            {
			    // Display bitmap on screen (-negative height to flip DIB upside down)
			    iResult = ::StretchDIBits(hDC, x, y, zoomWidth, zoomHeight, 0, 0, Width, Height, m_pBits, &m_bmiDisplay.bmi, DIB_RGB_COLORS, SRCCOPY);
            }
		}
	}
	return iResult;
}

// Compute least squared color difference
LOCAL COLORREF ColorDiff(COLORREF rgb1, COLORREF rgb2)
{
    // If matching color, replace with Windows color
    const int rDiff = GetRValue(rgb1) - GetRValue(rgb2);
    const int gDiff = GetGValue(rgb1) - GetGValue(rgb2);
    const int bDiff = GetBValue(rgb1) - GetBValue(rgb2);
    // Use least squared difference
    const long lDiff = rDiff * rDiff + gDiff * gDiff + bDiff * bDiff;
    return lDiff;
}

LOCAL COLORREF AvePixel(LPBYTE pSrcRow, DWORD dwSrcRowBytes, LPCOLORMAP pColorMap, UINT uColors, int iScale)
{
    const int iPower = iScale * iScale;
    const int iPower2 = iPower / 2;
    int red = iPower2;      // For rounding
    int grn = iPower2;
    int blu = iPower2;
    for (int row = iScale; row > 0; --row)
    {
        LPBYTE pSrc = pSrcRow;
        for (int col = iScale; col > 0; --col)
        {
            COLORREF rgb = RGB(pSrc[2], pSrc[1], pSrc[0]);
            pSrc += 3;
            // Map color based on pColorMap, uColors
            long lClosest = 5;
            for (UINT u = 0; u < uColors; ++u)
            {
                const long lDiff = ColorDiff(pColorMap[u].from, rgb);
                if (lDiff < lClosest)
                {
                    lClosest = lDiff;
                    rgb = pColorMap[u].to;
                }
            }
            // Check for "solid" color flag (no pixel averaging)
            if (rgb & 0xff000000)
            {
                return rgb;
            }
            red += GetRValue(rgb);
            grn += GetGValue(rgb);
            blu += GetBValue(rgb);
        }
        pSrcRow += dwSrcRowBytes;
    }
    // Return "average" pixel
    return RGB(red / iPower, grn / iPower, blu / iPower);
}

// Copy (and resize) 24-bit Bitmap mapping colors
LOCAL void CopyBitmap24(LPBYTE pDstRow, LPBYTE pSrcRow, int width, int height, LPCOLORMAP pColorMap, UINT uColors, int iScale)
{
    ASSERT( iScale > 0 );
    const DWORD dwSrcRowBytes = DWORD_PAD(width * 3);
    const DWORD dwDstRowBytes = DWORD_PAD(width / iScale * 3);

    for (int row = 0; row < height; row += iScale)
    {
        LPBYTE pDst = pDstRow;
        LPBYTE pSrc = pSrcRow + (row * dwSrcRowBytes);
        for (int col = 0; col < width; col += iScale)
        {
            const COLORREF rgb = AvePixel(pSrc, dwSrcRowBytes, pColorMap, uColors, iScale);
            *pDst++ = GetBValue(rgb);
            *pDst++ = GetGValue(rgb);
            *pDst++ = GetRValue(rgb);
            pSrc += (iScale * 3);
        }
        pDstRow += dwDstRowBytes;
    }
}

// Create a Device Independent Bitmap from current GIF image
// Colorize bitmap to match Windows desktop colors
// Returns NULL if error else handle to bitmap
HBITMAP CGIFWin::CreateMappedBitmap(LPCOLORMAP pMap, UINT uCount, int iScale /*=1*/)
{
    HBITMAP hBitmap = NULL;

    ASSERT( m_pGifFile && m_pBits );

    // Create memory device context compatible with current screen
    HDC hDC = ::CreateCompatibleDC(NULL);
    if (hDC)
    {
        // Create bitmap from current image state
        LPVOID pBits = NULL;
        BMI256 bmiSize = m_bmiDisplay;
        if (iScale > 0)
        {
            bmiSize.bmi.bmiHeader.biWidth /= iScale;
            bmiSize.bmi.bmiHeader.biHeight /= iScale;
        }

        hBitmap = ::CreateDIBSection(hDC, &bmiSize.bmi, DIB_RGB_COLORS, &pBits, /*handle=*/ NULL, /*offset=*/ 0L);
        if (hBitmap && pBits)
        {
		    const int cxScreen =  m_pGifFile->SWidth;
		    const int cyScreen = m_pGifFile->SHeight;
            ASSERT( m_bmiDisplay.bmi.bmiHeader.biBitCount == 24 );
            ::CopyBitmap24((LPBYTE) pBits, (LPBYTE) m_pBits, cxScreen, cyScreen, pMap, uCount, iScale);
        }
        VERIFY( ::DeleteDC(hDC) );
    }
    return hBitmap;
}

int CGIFWin::GetHeight()
{
	return m_pGifFile ? m_pGifFile->SHeight : 0;
}

int CGIFWin::GetWidth()
{
	return m_pGifFile ? m_pGifFile->SWidth : 0;
}

// Netscape 2.0 looping extension block
LOCAL GifByteType szNetscape20ext[] = "\x0bNETSCAPE2.0";
#define NSEXT_LOOP      0x01        // Loop Count field code

//
//  Appendix E. Interlaced Images.
//
//  The rows of an Interlaced images are arranged in the following order:
//  
//        Group 1 : Every 8th. row, starting with row 0.              (Pass 1)
//        Group 2 : Every 8th. row, starting with row 4.              (Pass 2)
//        Group 3 : Every 4th. row, starting with row 2.              (Pass 3)
//        Group 4 : Every 2nd. row, starting with row 1.              (Pass 4)
//  
const int InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
const int InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
//
//  The Following example illustrates how the rows of an interlaced image are
//  ordered.
//  
//        Row Number                                        Interlace Pass
//  
//   0    -----------------------------------------       1
//   1    -----------------------------------------                         4
//   2    -----------------------------------------                   3
//   3    -----------------------------------------                         4
//   4    -----------------------------------------             2
//   5    -----------------------------------------                         4
//   6    -----------------------------------------                   3
//   7    -----------------------------------------                         4
//   8    -----------------------------------------       1
//   9    -----------------------------------------                         4
//   10   -----------------------------------------                   3
//   11   -----------------------------------------                         4
//   12   -----------------------------------------             2
//   13   -----------------------------------------                         4
//   14   -----------------------------------------                   3
//   15   -----------------------------------------                         4
//   16   -----------------------------------------       1
//   17   -----------------------------------------                         4
//   18   -----------------------------------------                   3
//   19   -----------------------------------------                         4
//

// Fetch next image from GIF file
// Returns delay in msec, 0 for end-of-file, negative for error)
int CGIFWin::NextImage()
{
	// Error if no gif file!
	if (!m_pGifFile)
	{
		return -1;
	}
	const int cxScreen =  m_pGifFile->SWidth;
	const int cyScreen = m_pGifFile->SHeight;
	//					 ___________
	//		pBits1 ->	|			|
	//					|	current	|
	//					|	image	|
	//					|___________|
	//		pBits2 ->	|			|
	//					|	next	|
	//					|	image	|
	//					|___________|
	//		pLine ->	|___________|
	//
	const DWORD dwRowBytes = DWORD_PAD(cxScreen * 3);

#define XYOFFSET(x,y)	((y) * dwRowBytes + (x) * 3)

	const DWORD dwScreen = dwRowBytes * cyScreen;
	LPBYTE pBits1 = m_pBits;
	LPBYTE pBits2 = pBits1 + dwScreen;
	GifPixelType *pLine = pBits2 + dwScreen;

	GifRecordType RecordType;
	GifByteType *pExtension;
	int delay = 10;     // Default to 100 msec
	int dispose = 0;
	int transparent = GIF_NOT_TRANSPARENT;
	do {
		int i, ExtCode;

		if (DGifGetRecordType(m_pGifFile, &RecordType) == GIF_ERROR) 
		{
			break;
		}
		switch (RecordType)
		{
		case IMAGE_DESC_RECORD_TYPE:
			if (DGifGetImageDesc(m_pGifFile) != GIF_ERROR)
			{
				const int x = m_pGifFile->Image.Left;
				const int y = m_pGifFile->Image.Top;
				++m_iImageNum;
				TRACE("\nImage #%d:\n\n\tImage Size - Left = %d, Top = %d, Width = %d, Height = %d.\n",
					   m_iImageNum, x, y,
					   m_pGifFile->Image.Width, m_pGifFile->Image.Height);
				TRACE("\tImage is %s",
					   m_pGifFile->Image.Interlace ? "Interlaced" :
								"Non Interlaced");
				if (m_pGifFile->Image.ColorMap != NULL)
					TRACE(", BitsPerPixel = %d.\n",
						m_pGifFile->Image.ColorMap->BitsPerPixel);
				else
					TRACE(".\n");

				GifColorType* pColorTable;
				if (m_pGifFile->Image.ColorMap == NULL)
				{
					TRACE("\tNo Image Color Map.\n");
					// Copy global bitmap info for display
					memcpy(&m_bmiDisplay, &m_bmiGlobal, sizeof(m_bmiDisplay));
					pColorTable = m_pGifFile->SColorMap->Colors;
				}
				else
				{
					TRACE("\tImage Has Color Map.\n");
					::InitBitmapInfo(&m_bmiDisplay.bmi, cxScreen, cyScreen);
					::CopyColorMap(m_pGifFile->Image.ColorMap, &m_bmiDisplay.bmi);
					pColorTable = m_pGifFile->Image.ColorMap->Colors;
				}

				// Always copy next -> current image
				memcpy(pBits1, pBits2, dwScreen);

				const int Width = m_pGifFile->Image.Width;
				const int Height = m_pGifFile->Image.Height;
				if (m_pGifFile->Image.Interlace)
				{
					// Need to perform 4 passes on the images:
					for (int pass = 0; pass < 4; pass++)
					{
						for (i = InterlacedOffset[pass]; i < Height; i += InterlacedJumps[pass])
						{
							if (DGifGetLine(m_pGifFile, pLine, Width) == GIF_ERROR)
							{
								TRACE("DGifGetLine error=%d\n", GifLastError());
								return -1;
							}
							CopyGIF(pBits1 + XYOFFSET(x, y + i), pLine, Width, transparent, pColorTable);
						}
					}
				}
				else
				{
					// Non-interlaced image
					for (i = 0; i < Height; i++)
					{
						if (DGifGetLine(m_pGifFile, pLine, Width) == GIF_ERROR)
						{
							TRACE("DGifGetLine error=%d\n", GifLastError());
							return -1;
						}
						CopyGIF(pBits1 + XYOFFSET(x, y + i), pLine, Width, transparent, pColorTable);
					}
				}
				// Prepare second image with next starting
				if (dispose == GIF_DISPOSE_BACKGND)
				{
					TRACE("*** GIF_DISPOSE_BACKGND ***\n");
					const int x = m_pGifFile->Image.Left;
					const int y = m_pGifFile->Image.Top;
					const int Width = m_pGifFile->Image.Width;
					const int Height = m_pGifFile->Image.Height;

					// Clear next image to background index
					// Note: if transparent restore to transparent color (else use GIF background color)
					const COLORREF rgbFill = (transparent == GIF_NOT_TRANSPARENT) ? m_rgbBackgnd : m_rgbTransparent;
					for (int i = 0; i < Height; ++i)
						::FillGIF(pBits2 + XYOFFSET(x, y + i), rgbFill, Width);
				}
				else if (dispose != GIF_DISPOSE_RESTORE)
				{
					// Copy current -> next (Update)
					memcpy(pBits2, pBits1, dwScreen);
				}
				dispose = 0;
				TRACE("\tdelay = %d msec\n", delay * 10);
				if (delay)
				{
					return delay * 10;
				}
			}
			break;
		case EXTENSION_RECORD_TYPE:
        {
			if (DGifGetExtension(m_pGifFile, &ExtCode, &pExtension) == GIF_ERROR)
			{
				TRACE("DGifGetExtension error=%d\n", GifLastError());
				return -2;
			}
			TRACE("\n");
            BOOL bNetscapeExt = FALSE;
			switch (ExtCode)
			{
			case COMMENT_EXT_FUNC_CODE:
				TRACE("GIF89 comment");
				break;
			case GRAPHICS_EXT_FUNC_CODE:
			{
				TRACE("GIF89 graphics control");
				ASSERT( pExtension[0] == 4 );
				// 
				int flag = pExtension[1];
				delay  = MAKEWORD(pExtension[2], pExtension[3]);
				transparent = (flag & GIF_TRANSPARENT) ? pExtension[4] : GIF_NOT_TRANSPARENT;
				dispose = (flag >> GIF_DISPOSE_SHIFT) & GIF_DISPOSE_MASK;

				TRACE(" delay = %d, dispose = %d transparent = %d\n", delay, dispose, transparent);
				break;
			}
			case PLAINTEXT_EXT_FUNC_CODE:
				TRACE("GIF89 plaintext");
				break;
			case APPLICATION_EXT_FUNC_CODE:
            {
				TRACE("GIF89 application block\n");
                ASSERT( pExtension );
                if (memcmp(pExtension, szNetscape20ext, szNetscape20ext[0]) == 0)
                {
                    TRACE("Netscape 2.0 extension\n");
                    bNetscapeExt = TRUE;
                }
				break;
            }
			default:
				TRACE("pExtension record of unknown type");
				break;
			}
			TRACE(" (Ext Code = %d):\n", ExtCode);
			do
			{
				if (DGifGetExtensionNext(m_pGifFile, &pExtension) == GIF_ERROR)
				{
					TRACE("DGifGetExtensionNext error=%d\n", GifLastError());
					return -3;
				}
                // Process Netscape 2.0 extension (GIF looping)
                if (pExtension && bNetscapeExt)
                {
                    GifByteType bLength = pExtension[0];
                    int iSubCode = pExtension[1] & 0x07;
                    if (bLength == 3 && iSubCode == NSEXT_LOOP)
                    {
                        UINT uLoopCount = MAKEWORD(pExtension[2], pExtension[3]);
                        m_uLoopCount = uLoopCount - 1;
                        TRACE("Looping extension, uLoopCount=%u\n", m_uLoopCount);
                    }
                }
			}
			while (pExtension);
			break;
        }
		case TERMINATE_RECORD_TYPE:
			break;
		default:		     // Should be trapped by DGifGetRecordType
			break;
		}
	}
	while (RecordType != TERMINATE_RECORD_TYPE);
	return 0;
}
