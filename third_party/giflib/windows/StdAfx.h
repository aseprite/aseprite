// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Local Header Files

// TODO: reference additional headers your program requires here
//#ifndef _T
//    #define _T(x)		TEXT(x)
//#endif
#define _countof(a)		(sizeof(a) / sizeof(a[0]))

#ifdef _DEBUG

#include <assert.h>

	#define ASSERT(x)		assert(x)
	#define VERIFY(x)		assert(x)
	#define TRACE			DebugTrace

void __inline __cdecl DebugTrace(LPCTSTR pszFormat, ...)
{
	TCHAR szBuffer[1024];
	va_list arglist;
	va_start(arglist, pszFormat );
#ifdef WIN32
	VERIFY( wvsprintf(szBuffer, pszFormat, arglist) < _countof(szBuffer));
	OutputDebugString(szBuffer);
#else
	VERIFY( vsprintf(szBuffer, pszFormat, arglist) < _countof(szBuffer));
	fprintf(stderr, szBuffer);
#endif
	va_end(arglist);
}

#else // !_DEBUG

	#define ASSERT(x)
	#define VERIFY(x)		(x)
	#define TRACE			(void)

#endif // _DEBUG

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
