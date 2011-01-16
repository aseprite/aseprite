/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989   *
******************************************************************************
* Program to display GIF file under X11 window system.			     *
* Options:								     *
* -q : quiet printing mode.						     *
* -p PosX PosY : defines the position where to put the image.		     *
* -d Display : what display should go to.				     *
* -f : force attempt to allocate the exact colors. This usually look bad...  *
* -h : on-line help.							     *
******************************************************************************
* History:								     *
* 28 Dec 89 - Version 1.0 by Gershon Elber, color allocation is based on the *
*		xgif program by John Bradley, bradley@cis.ipenn.edu.	     *
*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <graphics.h>
#include <stdlib.h>
#include <alloc.h>
#include <io.h>
#include <dos.h>
#include <bios.h>
#endif /* __MSDOS__ */

#ifdef HAVE_LIBX11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#endif /* HAVE_LIBX11 */

#ifndef __MSDOS__
#include <stdlib.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"Gif2X11"

#define ICON_SIZE	60
#define ABS(x)		((x) > 0 ? (x) : (-(x)))

#ifdef __MSDOS__
extern unsigned int
    _stklen = 16384;			     /* Increase default stack size. */
#endif /* __MSDOS__ */
#ifdef SYSV
static char *VersionStr =
        "Gif toolkit module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr = "Gif2X11 q%- p%-PosX|PosY!d!d d%-Display!s f%- h%- GifFile!*s";
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
	" q%- p%-PosX|PosY!d!d d%-Display!s f%- h%- GifFile!*s";
#endif /* SYSV */

/* Make some variables global, so we could access them faster: */
static int
    PosFlag = FALSE,
    HelpFlag = FALSE,
    DisplayFlag = FALSE,
    ForceFlag = FALSE,
    ColorMapSize = 0,
    BackGround = 0,
    XPosX = 0,
    XPosY = 0,
    InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
static char
    *DisplayName = NULL;
static ColorMapObject
    *ColorMap;

/* X specific staff goes here. XColorTable will hold the GIF image colors,   */
/* while XPixelTable will hold the pixel number so we can redirect through   */
/* it when forming the image bitmap in X format.			     */
/* Note the table has 256 entry which is the maximum allowed in GIF format.  */
static XColor XColorTable[256];
static unsigned long XPixelTable[256];
static Display *XDisplay;
static int XScreen;
static Window Xroot, XImageWndw;
static Colormap XColorMap;
static GC XGraphContext;
static Visual *XVisual;
static XImage *XImageBuffer;
static Pixmap XIcon;
static Cursor XCursor;

#define BYTESPERPIXEL 4

static void Screen2X(int argc, char **argv, GifRowType *ScreenBuffer,
		     int ScreenWidth, int ScreenHeight);
static void AllocateColors1(void);
static void AllocateColors2(void);

/******************************************************************************
* Interpret the command line and scan the given GIF file.		      *
******************************************************************************/
int main(int argc, char **argv)
{
    int	i, j, Error, NumFiles, ImageNum = 0, Size, Row, Col, Width, Height,
        ExtCode, Count;
    GifRecordType RecordType;
    GifByteType *Extension;
    char **FileName = NULL;
    GifRowType *ScreenBuffer;
    GifFileType *GifFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr,
		&GifQuietPrint, &PosFlag, &XPosX, &XPosY,
	        &DisplayFlag, &DisplayName, &ForceFlag,
		&HelpFlag, &NumFiles, &FileName)) != FALSE ||
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

    /* Lets see if we can get access to the X server before we even start: */
    if ((XDisplay = (Display *) XOpenDisplay(DisplayName)) == NULL)
	GIF_EXIT("Failed to access X server, abored.");
    XScreen = DefaultScreen(XDisplay);
    Xroot = RootWindow(XDisplay, XScreen);
    XColorMap = DefaultColormap(XDisplay, XScreen);
    XGraphContext = DefaultGC(XDisplay, XScreen);
    XVisual = DefaultVisual(XDisplay, XScreen);
    XSetBackground(XDisplay, XGraphContext, BlackPixel(XDisplay, XScreen));
    XSetForeground(XDisplay, XGraphContext, WhitePixel(XDisplay, XScreen));

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
		    fprintf(stderr, "Image %d is not confined to screen dimension, aborted.\n",ImageNum);
		    exit(EXIT_FAILURE);
		}
		if (GifFile->Image.Interlace) {
		    /* Need to perform 4 passes on the images: */
		    for (Count = i = 0; i < 4; i++)
			for (j = Row + InterlacedOffset[i]; j<Row + Height;
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
    ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);
    if (ColorMap == NULL) {
        fprintf(stderr, "Gif Image does not have a colormap\n");
        exit(EXIT_FAILURE);
    }
    ColorMapSize = ColorMap->ColorCount;
    Screen2X(argc, argv, ScreenBuffer, GifFile->SWidth, GifFile->SHeight);

    for (i = GifFile->SHeight - 1 ; i >= 0 ; i--) {
	free( ScreenBuffer[ i ] );
    }
    free( ScreenBuffer );
    if ( XImageBuffer != (XImage *) NULL )
	XDestroyImage( XImageBuffer );

    if ( XIcon )
	XFreePixmap( XDisplay , XIcon );

    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError();
	exit(EXIT_FAILURE);
    }
    GifQprintf("\n");

	return 0 ;
}

/******************************************************************************
* The real display routine.						      *
******************************************************************************/
static void Screen2X(int argc, char **argv, GifRowType *ScreenBuffer,
		     int ScreenWidth, int ScreenHeight)
{
#define	WM_DELETE_WINDOW	"WM_DELETE_WINDOW"

	Status	rc ;
	Atom	atomKill ;

    int i, j, c, Size, x, y,
        MinIntensity, MaxIntensity, AvgIntensity, IconSizeX, IconSizeY;
    char *XImageData, *XIconData, KeyBuffer[81];
    unsigned long ValueMask;
    GifColorType *ColorMapEntry = ColorMap->Colors;
    XSetWindowAttributes SetWinAttr;
    XSizeHints Hints;
    XEvent Event;
    XExposeEvent *EEvent;
    XKeyEvent *KEvent;
    XComposeStatus Stat;
    KeySym KS;

    /* Let find out what are the intensities in the color map: */
    MaxIntensity = 0;
    MinIntensity = 256 * 100;
    for (i = 0; i < ColorMapSize; i++) {
	c = ColorMapEntry[i].Red * 30 +
	    ColorMapEntry[i].Green * 59 +
	    ColorMapEntry[i].Blue * 11;
	if (c > MaxIntensity) MaxIntensity = c;
	if (c < MinIntensity) MinIntensity = c;
    }
    AvgIntensity = (MinIntensity + MaxIntensity) / 2;

    /* The big trick here is to select the colors so lets do this first: */
    if (ForceFlag)
	AllocateColors2();
    else
	AllocateColors1();

    SetWinAttr.background_pixel = BlackPixel( XDisplay, XScreen );
    SetWinAttr.border_pixel = WhitePixel( XDisplay, XScreen );
    ValueMask = CWBackPixel | CWBorderPixel;

    Hints.flags = PSize | PMinSize | PMaxSize;
    Hints.x = Hints.y = 1;
    Hints.width = Hints.min_width = Hints.max_width = ScreenWidth;
    Hints.height = Hints.min_height = Hints.max_height = ScreenHeight;
    if (PosFlag) {
	Hints.flags |= USPosition;
	Hints.x = XPosX;
	Hints.y = XPosY;
    }

    XImageWndw = XCreateWindow(XDisplay, Xroot, XPosX, XPosY,
			       ScreenWidth, ScreenHeight,
			       1, 0,
			       CopyFromParent, CopyFromParent,
			       ValueMask, &SetWinAttr);

    /* Set up the icon bit map to be a shrinked BW version of the image: */
    if (ScreenWidth > ScreenHeight) {
	IconSizeX = (ICON_SIZE / 8) * 8;
	IconSizeY = (ScreenHeight * ICON_SIZE) / ScreenWidth;
    }
    else {
	IconSizeY = ICON_SIZE;
	IconSizeX = (((ScreenWidth * ICON_SIZE) / ScreenHeight) / 8) * 8;
    }
    XIconData = (char *) malloc(IconSizeX * IconSizeY / 8);
    memset(XIconData, 0, IconSizeX * IconSizeY / 8);
    for (i = 0; i < IconSizeY; i++) {
	y = (i * ScreenHeight / IconSizeY);
	Size = i * IconSizeX / 8;
	for (j = 0; j < IconSizeX; j++) {
	    x = j * ScreenWidth / IconSizeX;
	    c = ScreenBuffer[y][x];
	    c = ColorMapEntry[c].Red * 30 +
		ColorMapEntry[c].Green * 59 +
		ColorMapEntry[c].Blue * 11 > AvgIntensity;
	    XIconData[Size + j / 8] |= c << (j % 8);
	}
    }

    XIcon = XCreateBitmapFromData(XDisplay, XImageWndw, XIconData,
				  IconSizeX, IconSizeY);

    XSetStandardProperties(XDisplay, XImageWndw,
			   PROGRAM_NAME, PROGRAM_NAME, XIcon,
			   argv, argc,
			   &Hints);

    free( XIconData );

    atomKill = XInternAtom(XDisplay, WM_DELETE_WINDOW, False );
    rc = XSetWMProtocols( XDisplay, XImageWndw , &atomKill , 1 );
    if ( rc == 0 )
	GIF_EXIT("Failed to trap WM_DELETE_WINDOW event" );

    XSelectInput(XDisplay, XImageWndw, ExposureMask | KeyPressMask);

    /* Set out own cursor: */
    XCursor = XCreateFontCursor(XDisplay, XC_diamond_cross);
    XDefineCursor(XDisplay, XImageWndw, XCursor);
    
    XMapWindow(XDisplay, XImageWndw);

    /* Create the image in X format: */
    if ((XImageData = (char *) malloc(ScreenWidth * ScreenHeight * BYTESPERPIXEL)) == NULL)
	GIF_EXIT("Failed to allocate memory required, aborted.");

    for (i = 0; i < ScreenHeight; i++) {
	y = i * ScreenWidth;
	for (j = 0; j < ScreenWidth; j++) {
	    XImageData[(y + j)*BYTESPERPIXEL] = XColorTable[ScreenBuffer[i][j]].blue;
	    XImageData[(y + j)*BYTESPERPIXEL+1] = XColorTable[ScreenBuffer[i][j]].green;
	    XImageData[(y + j)*BYTESPERPIXEL+2] = XColorTable[ScreenBuffer[i][j]].red;
    }
    }
    XImageBuffer = XCreateImage(XDisplay, XVisual, DefaultDepth(XDisplay, DefaultScreen(XDisplay)), ZPixmap, 0,
				XImageData, ScreenWidth, ScreenHeight,
                 BitmapPad(XDisplay), ScreenWidth*BYTESPERPIXEL);

    while (TRUE) {
	XNextEvent(XDisplay, &Event);
	switch (Event.type) {
	    case Expose:
	        EEvent = (XExposeEvent *) &Event;
		XPutImage(XDisplay, XImageWndw, XGraphContext, XImageBuffer,
			  EEvent->x, EEvent->y,
			  EEvent->x, EEvent->y,
			  EEvent->width, EEvent->height);
		break;
	    case KeyPress:
	        KEvent = (XKeyEvent *) &Event;
		XLookupString(KEvent, KeyBuffer, 80, &KS, &Stat);
		if (KeyBuffer[0] == 3) return;
	/*	if (KeyBuffer[0] == 3) { free(XImageData ); return; }	made by XDestroyImage	*/
		break;
	    case ClientMessage :
		if ( Event.xclient.data.l[0] == atomKill ) return ;
		break ;
	}
    }
}

/******************************************************************************
* Routine to allocate the requested colors from the X server.		      *
* Colors are allocated until success by stripping off the least bits of the   *
* colors.								      *
******************************************************************************/
static void AllocateColors1(void)
{
    int Strip, Msk, i;
    char Msg[80];

    for (i = 0; i < 256; i++)
	XPixelTable[i] = 0;	   /* Put reasonable color for out of range. */

    for (Strip = 0, Msk = 0xff; Strip < 8; Strip++, Msk <<= 1) {
	for (i = 0; i < ColorMapSize; i++) {
	    /* Prepare color entry in X format. */
	    XColorTable[i].red = (ColorMap->Colors[i].Red & Msk) << 8;
	    XColorTable[i].green = (ColorMap->Colors[i].Green & Msk) << 8;
	    XColorTable[i].blue = (ColorMap->Colors[i].Blue & Msk) << 8;
	    XColorTable[i].flags = DoRed | DoGreen | DoBlue;
	    if (XAllocColor(XDisplay, XColorMap, &XColorTable[i]))
		XPixelTable[i] = XColorTable[i].pixel;
	    else
		break;
	}
	if (i < ColorMapSize)
	    XFreeColors(XDisplay, XColorMap, XPixelTable, i, 0L);
	else
	    break;
    }

    if (Strip == 8)
	GIF_EXIT("Can not display the image - not enough colors available.");

    if (Strip != 0) {
	sprintf(Msg, "%d bits were stripped off the color map.", Strip);
	GIF_MESSAGE(Msg);
    }
}

/******************************************************************************
* Routine to allocate the requested colors from the X server.		      *
* Two stages are performed:						      *
* 1. Colors are requested directly.					      *
* 2. If not enough colors can be allocated, the closest current color	      *
*    in current table is selected instead.				      *
* This allocation is not optimal as when fail to allocate all colors one      *
* should pick the right colors to do allocate in order to minimize the        *
* closest distance from the unallocated ones under some norm (what is a good  *
* norm for the RGB space?). Improve it if you are bored.		      *
******************************************************************************/
static void AllocateColors2(void)
{
    int i, j, Index = 0, Count = 0, XNumOfColors;
    char Msg[80];
    unsigned long D, Distance, AvgDistance = 0, Red, Green, Blue;
    GifBooleanType Failed = FALSE;
    XColor *XOldColorTable;

    for (i = 0; i < 256; i++) {
	if (i < ColorMapSize) {          /* Prepere color entry in X format. */
	    XColorTable[i].red = ColorMap->Colors[i].Red << 8;
	    XColorTable[i].green = ColorMap->Colors[i].Green << 8;
	    XColorTable[i].blue = ColorMap->Colors[i].Blue << 8;
	    XColorTable[i].flags = DoRed | DoGreen | DoBlue;
	    XPixelTable[i] = -1;		       /* Not allocated yet. */
	}
	else
	    XPixelTable[i] = 0;    /* Put reasonable color for out of range. */
    }

    for (i = 0; i < ColorMapSize; i++)	      /* Allocate the colors from X: */
	if (XAllocColor(XDisplay, XColorMap, &XColorTable[i]))
	    XPixelTable[i] = XColorTable[i].pixel;
	else
	    Failed = TRUE;

    if (Failed) {
	XNumOfColors = DisplayCells(XDisplay, XScreen);
	XOldColorTable = (XColor *) malloc(sizeof(XColor) * XNumOfColors);
	for (i = 0; i < XNumOfColors; i++) XOldColorTable[i].pixel = i;
	XQueryColors(XDisplay, XColorMap, XOldColorTable, XNumOfColors);
	
	for (i = 0; i < ColorMapSize; i++) {
	    /* Allocate closest colors from X: */
	    if (XPixelTable[i] == -1) {      /* Failed to allocate this one. */
		Distance = 0xffffffff;

		Red = XColorTable[i].red;
		Green = XColorTable[i].green;
		Blue = XColorTable[i].blue;

		for (j = 0; j < XNumOfColors; j++) {
		    /* Find the closest color in 3D RGB space using L1 norm. */
		    if ((D = ABS(Red - XOldColorTable[j].red) +
			     ABS(Green - XOldColorTable[j].green) +
			     ABS(Blue - XOldColorTable[j].blue)) < Distance) {
			Distance = D;
			Index = j;
		    }
		}
	        XPixelTable[i] = Index;

		AvgDistance += Distance;
		Count++;
	    }
	}
	free(XOldColorTable);

	sprintf(Msg, "Colors will be approximated (average error = %ld).\n",
		AvgDistance / Count);
	GIF_MESSAGE(Msg);
    }
}
