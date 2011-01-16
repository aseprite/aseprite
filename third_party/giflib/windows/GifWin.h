/*********************************************************
*
*	File:	GifWin.h
*	Title:	Graphics Interchange Format header
*
*	Author:	Lennie Araki
*	Date:	24-Nov-1999
*
*	This class is a thin wrapper around the open source
*	giflib-1.4.0 for opening, parsing and displaying
*	Compuserve GIF files on Windows.
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

#ifndef __GIFWIN_H__
#define __GIFWIN_H__

#include <commctrl.h>                   // For LPCOLORMAP

typedef struct GifFileType GifFileType;			// Opaque to avoid namespace collisions

// Fixed length struct to allocate 256-color BITMAPINFO
// (to avoid dynamic allocation)
typedef struct tagbmi256
{
    BITMAPINFO bmi; 
    RGBQUAD    fill[255];
} BMI256; 

// Win32 Class for wrapping libgif functionality
class CGIFWin
{
protected:
	GifFileType* m_pGifFile;
	LPBYTE m_pBits;
    BMI256 m_bmiGlobal;
	BMI256 m_bmiDisplay;
	COLORREF m_rgbTransparent;
	COLORREF m_rgbBackgnd;
	int m_iImageNum;
    UINT m_uLoopCount;               // Netscape 2.0 loop count
public:
// Constructor/destructor
	CGIFWin();
	~CGIFWin();
// Operations
	int Open(LPCTSTR pszFileName, COLORREF rgbBack = RGB(255,255,255));
	void Close();
	int Draw(HDC hDC, LPCRECT pRect, int iFactor = 0);
	int NextImage();
    UINT GetLoopCount() const
    {   return m_uLoopCount;  }
    HBITMAP CreateMappedBitmap(LPCOLORMAP pMap, UINT uCount, int iScale = 1);

// Get image size (in pixels)
	int GetHeight();
	int GetWidth();
};

#endif // __GIFWIN_H__
