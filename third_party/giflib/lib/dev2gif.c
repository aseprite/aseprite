/*****************************************************************************
 *   "Gif-Lib" - Yet another gif library.
 *
 * Written by:  Gershon Elber            IBM PC Ver 1.1,    Jun. 1989
 *****************************************************************************
 * Module to dump graphic devices into a GIF file. Current supported devices:
 * 1. EGA, VGA, SVGA (800x600), Hercules on the IBM PC (#define __MSDOS__).
 * 2. SGI 4D Irix using gl library (#define SGI_GL__).
 * 3. X11 using libX.a (#define __X11__).
 * 4. (2 & 3 have been changed to HAVE_GL_S and HAVE_LIBX11 and should be set
 * by the configure script.)
 *****************************************************************************
 * History:
 * 22 Jun 89 - Version 1.0 by Gershon Elber.
 * 12 Aug 90 - Version 1.1 by Gershon Elber (added devices).
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <dos.h>
#include <alloc.h>
#include <graphics.h>
#endif /* __MSDOS__ */

#ifdef HAVE_LIBX11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif /* HAVE_LIBX11 */

#ifdef HAVE_LIBGL_S
#include <gl/gl.h>
#endif /* HAVE_LIBGL_S */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gif_lib.h"

#define SVGA_SPECIAL 999    /* 800 by 600 Super VGA mode. */

static int GraphDriver = -1,    /* Device parameters - reasonable values. */
   GraphMode = -1, ScreenColorBits = 1;
static long ScreenXMax = 100, ScreenYMax = 100;

#ifdef __MSDOS__
static unsigned int ScreenBase;
#endif /* __MSDOS__ */

#if defined(HAVE_LIBGL_S) || defined(HAVE_LIBX11)
GifByteType *GlblGifBuffer = NULL, *GlblGifBufferPtr = NULL;
#endif /* HAVE_LIBGL_S || HAVE_LIBX11 */

#ifdef HAVE_LIBGL_S
static int QuantizeRGBBuffer(int Width, int Height, long *RGBBuffer,
                             GifColorType * ColorMap,
                             GifByteType * GIFBuffer);
#endif /* HAVE_LIBGL_S */

static void GetScanLine(GifPixelType * ScanLine, int Y);
static int HandleGifError(GifFileType * GifFile);

/******************************************************************************
 * Dump the given Device, into given File as GIF format:
 * Return 0 on success, -1 if device not supported, or GIF-LIB error number.
 * Device is selected via the ReqGraphDriver. Device mode is selected via
 * ReqGraphMode1/2 as follows:
 * 1. IBM PC Hercules card: HERCMONO (one mode only) in ReqGraphMode1,
 *    ReqGraphMode2/3 are ignored.
 * 2. IBM PC EGA card: EGALO/EGAHI in ReqGraphMode1,
 *    ReqGraphMode2/3 are ignored.
 * 3. IBM PC EGA64 card: EGA64LO/EGA64HI in ReqGraphMode1,
 *    ReqGraphMode2/3 are ignored.
 * 4. IBM PC EGAMONO card: EGAMONOHI (one mode only) in ReqGraphMode1,
 *    ReqGraphMode2/3 are ignored.
 * 5. IBM PC VGA card: VGALO/VGAMED/VGAHI in ReqGraphMode1,
 *    ReqGraphMode2/3 are ignored.
 * 6. IBM PC SVGA card: ReqGraphMode1/2 are both ignored. Fixed mode (800x600
 *    16 colors) is assumed.
 * 7. SGI 4D using GL: window id to dump (as returned by winget()) in
 *    ReqGraphMode1, ReqGraphMode2/3 are ignored.
 * 8. X11: Window id in ReqGraphMode1, Display id in ReqGraphMode2, Color
 *    map id in  ReqGraphMode3.
 *****************************************************************************/
int
DumpScreen2Gif(const char *FileName,
               int ReqGraphDriver,
               long ReqGraphMode1,
               long ReqGraphMode2,
               long ReqGraphMode3) {

    int i, j, k;
    GifPixelType *ScanLine;
    GifFileType *GifFile;
    ColorMapObject *ColorMap = NULL;
#ifdef __MSDOS__
    static GifColorType MonoChromeColorMap[] = {
        {0, 0, 0},
        {255, 255, 255}
    };
    /* I have no idea what default EGA64 (4 colors) should be (I guessed...). */
    static GifColorType EGA64ColorMap[] = {
        {0, 0, 0},    /* 0. Black */
        {255, 0, 0},    /* 1. Red */
        {0, 255, 0},    /* 2. Green */
        {0, 0, 255},    /* 3. Blue */
    };
    static GifColorType EGAColorMap[] = {
        {0, 0, 0},    /* 0. Black */
        {0, 0, 170},    /* 1. Blue */
        {0, 170, 0},    /* 2. Green */
        {0, 170, 170},    /* 3. Cyan */
        {170, 0, 0},    /* 4. Red */
        {170, 0, 170},    /* 5. Magenta */
        {170, 170, 0},    /* 6. Brown */
        {170, 170, 170},    /* 7. LightGray */
        {85, 85, 85},    /* 8. DarkGray */
        {85, 85, 255},    /* 9. LightBlue */
        {85, 255, 85},    /* 10. LightGreen */
        {85, 255, 255},    /* 11. LightCyan */
        {255, 85, 85},    /* 12. LightRed */
        {255, 85, 255},    /* 13. LightMagenta */
        {255, 255, 85},    /* 14. Yellow */
        {255, 255, 255},    /* 15. White */
    };
#endif /* __MSDOS__ */
#if defined(HAVE_LIBGL_S)
    long *RGBBuffer;
#ifndef HAVE_LIBX11
    GifColorType ColorMap256[256];
#endif /* Undefined HAVE_LIBX11 */
#endif /* HAVE_LIBGL_S */
#ifdef HAVE_LIBX11
    XImage *XImg;
    unsigned long XPixel;
    GifColorType ColorMap256[256];
    XColor XColorTable[256];    /* Up to 256 colors in X. */
    XWindowAttributes WinAttr;
#endif /* HAVE_LIBX11 */
    switch (ReqGraphDriver) {    /* Return on non supported screens. */
#ifdef __MSDOS__
      case HERCMONO:
          ScreenXMax = 720;
          ScreenYMax = 350;
          ScreenColorBits = 1;
          ScreenBase = 0xb000;
          ColorMap = MakeMapObject(2, MonoChromeColorMap);
          break;
      case EGA:
          switch (ReqGraphMode1) {
            case EGALO:
                ScreenYMax = 200;
                break;
            case EGAHI:
                ScreenYMax = 350;
                break;
            default:
                return -1;
          }
          ScreenXMax = 640;
          ScreenColorBits = 4;
          ScreenBase = 0xa000;
          ColorMap = MakeMapObject(16, EGAColorMap);
          break;
      case EGA64:
          switch (ReqGraphMode1) {
            case EGA64LO:
                ScreenYMax = 200;
                break;
            case EGA64HI:
                ScreenYMax = 350;
                break;
            default:
                return -1;
          }
          ScreenXMax = 640;
          ScreenColorBits = 2;
          ScreenBase = 0xa000;
          ColorMap = MakeMapObject(4, EGA64ColorMap);
          break;
      case EGAMONO:
          switch (ReqGraphMode1) {
            case EGAMONOHI:
                ScreenYMax = 350;
                break;
            default:
                return -1;
          }
          ScreenXMax = 640;
          ScreenColorBits = 1;
          ScreenBase = 0xa000;
          ColorMap = MakeMapObject(2, MonoChromeColorMap);
          break;
      case VGA:
          switch (ReqGraphMode1) {
            case VGALO:
                ScreenYMax = 200;
                break;
            case VGAMED:
                ScreenYMax = 350;
                break;
            case VGAHI:
                ScreenYMax = 480;
                break;
            default:
                return -1;
          }
          ScreenXMax = 640;
          ScreenColorBits = 4;
          ScreenBase = 0xa000;
          ColorMap = MakeMapObject(16, EGAColorMap);
          break;
      case SVGA_SPECIAL:
          ScreenXMax = 800;
          ScreenYMax = 600;
          ScreenColorBits = 4;
          ScreenBase = 0xa000;
          ColorMap = MakeMapObject(16, EGAColorMap);
          break;
#endif /* __MSDOS__ */
#ifdef HAVE_LIBGL_S
      case GIF_DUMP_SGI_WINDOW:
          winset(ReqGraphMode1);    /* Select window as active window. */
          getsize(&ScreenXMax, &ScreenYMax);

          RGBBuffer = (long *)malloc(sizeof(long) * ScreenXMax * ScreenYMax);
          readsource(SRC_FRONT);
          if (lrectread((short)0, (short)0, (short)(ScreenXMax - 1),
                        (short)(ScreenYMax - 1), RGBBuffer) !=
                 ScreenXMax * ScreenYMax) {    /* Get data. */
              free(RGBBuffer);
              return -1;
          }
          GlblGifBuffer = (GifByteType *) malloc(sizeof(GifByteType) *
                                                 ScreenXMax * ScreenYMax);
          i = QuantizeRGBBuffer(ScreenXMax, ScreenYMax, RGBBuffer,
                                ColorMap256, GlblGifBuffer);
          /* Find minimum color map size to hold all quantized colors. */
          for (ScreenColorBits = 1;
               (1 << ScreenColorBits) < i && ScreenColorBits < 8;
               ScreenColorBits++) ;

          /* Start to dump with top line as GIF expects it. */
          GlblGifBufferPtr = GlblGifBuffer + ScreenXMax * (ScreenYMax - 1);
          ColorMap = MakeMapObject(256, ColorMap256);
          free(RGBBuffer);
          break;
#endif /* HAVE_LIBGL_S */
#ifdef HAVE_LIBX11
      case GIF_DUMP_X_WINDOW:
          XGetWindowAttributes((Display *) ReqGraphMode2,
                               (Window) ReqGraphMode1, &WinAttr);
          ScreenXMax = WinAttr.width;
          ScreenYMax = WinAttr.height;

          XImg = XGetImage((Display *) ReqGraphMode2,
                           (Window) ReqGraphMode1,
                           0, 0, ScreenXMax - 1, ScreenYMax - 1,
                           AllPlanes, XYPixmap);

          GlblGifBuffer = (GifByteType *)malloc(sizeof(GifByteType) *
                                                 ScreenXMax * ScreenYMax);

          /* Scan the image for all different colors exists. */
          for (i = 0; i < 256; i++)
              XColorTable[i].pixel = 0;
          k = FALSE;
          for (i = 0; i < ScreenXMax; i++)
              for (j = 0; j < ScreenYMax; j++) {
                  XPixel = XGetPixel(XImg, i, j);
                  if (XPixel > 255) {
                      if (!k) {
                          /* Make sure we state it once only. */
                          fprintf(stderr, "X Color table - truncated.\n");
                          k = TRUE;
                      }
                      XPixel = 255;
                  }
                  XColorTable[XPixel].pixel = XPixel;
              }
          /* Find the RGB representation of the colors. */
          XQueryColors((Display *) ReqGraphMode2,
                       (Colormap) ReqGraphMode3, XColorTable, 256);
          /* Count number of active colors (Note color 0 is always in) */
          /* and create the Gif color map from it.  */
          ColorMap = MakeMapObject(256, ColorMap256);
          ColorMap->Colors[0].Red = ColorMap->Colors[0].Green =
             ColorMap->Colors[0].Blue = 0;
          for (i = j = 1; i < 256; i++)
              if (XColorTable[i].pixel) {
                  ColorMap->Colors[j].Red = XColorTable[i].red / 256;
                  ColorMap->Colors[j].Green = XColorTable[i].green / 256;
                  ColorMap->Colors[j].Blue = XColorTable[i].blue / 256;
                  /* Save the X color index into the Gif table: */
                  XColorTable[i].pixel = j++;
              }
          /* and set the number of colors in the Gif color map. */
          for (ScreenColorBits = 1;
               (1 << ScreenColorBits) < j && ScreenColorBits < 8;
               ScreenColorBits++) ;

          /* Prepare the Gif image buffer as indices into the Gif color */
          /* map from the X image.  */
          GlblGifBufferPtr = GlblGifBuffer;
          for (i = 0; i < ScreenXMax; i++)
              for (j = 0; j < ScreenYMax; j++)
                  *GlblGifBufferPtr++ =
                     XColorTable[XGetPixel(XImg, j, i) & 0xff].pixel;
          XDestroyImage(XImg);

          GlblGifBufferPtr = GlblGifBuffer;
          ColorMap = MakeMapObject(256, ColorMap256);
          break;
#endif /* HAVE_LIBX11 */
      default:
          return -1;
    }

    ScanLine = (GifPixelType *)malloc(sizeof(GifPixelType) * ScreenXMax);

    GraphDriver = ReqGraphDriver;
    GraphMode = ReqGraphMode1;

    if ((GifFile = EGifOpenFileName(FileName, FALSE)) == NULL ||
        EGifPutScreenDesc(GifFile, ScreenXMax, ScreenYMax, ScreenColorBits,
                          0, ColorMap) == GIF_ERROR ||
        EGifPutImageDesc(GifFile, 0, 0, ScreenXMax, ScreenYMax, FALSE,
                         NULL) == GIF_ERROR) {
        free((char *)ScanLine);
#if defined(HAVE_LIBGL_S) || defined(HAVE_LIBX11)
        free((char *)GlblGifBuffer);
#endif
        return HandleGifError(GifFile);
    }

    for (i = 0; i < ScreenYMax; i++) {
        GetScanLine(ScanLine, i);
        if (EGifPutLine(GifFile, ScanLine, ScreenXMax) == GIF_ERROR) {
            free((char *)ScanLine);
#if defined(HAVE_LIBGL_S) || defined(HAVE_LIBX11)
            free((char *)GlblGifBuffer);
#endif
            return HandleGifError(GifFile);
        }
    }

    if (EGifCloseFile(GifFile) == GIF_ERROR) {
        free((char *)ScanLine);
#if defined(HAVE_LIBGL_S) || defined(HAVE_LIBX11)
        free((char *)GlblGifBuffer);
#endif
        return HandleGifError(GifFile);
    }

    free((char *)ScanLine);
#if defined(HAVE_LIBGL_S) || defined(HAVE_LIBX11)
    free((char *)GlblGifBuffer);
#endif
    return 0;
}

#ifdef HAVE_LIBGL_S

/******************************************************************************
 * Quantize the given 24 bit (8 per RGB) into 256 colors.
 *****************************************************************************/
static int
QuantizeRGBBuffer(int Width,
                  int Height,
                  long *RGBBuffer,
                  GifColorType * ColorMap,
                  GifByteType * GIFBuffer) {

    int i;
    GifByteType *RedInput, *GreenInput, *BlueInput;

    /* Convert the RGB Buffer into 3 seperated buffers: */
    RedInput = (GifByteType *) malloc(sizeof(GifByteType) * Width * Height);
    GreenInput = (GifByteType *) malloc(sizeof(GifByteType) * Width * Height);
    BlueInput = (GifByteType *) malloc(sizeof(GifByteType) * Width * Height);

    for (i = 0; i < Width * Height; i++) {
        RedInput[i] = RGBBuffer[i] & 0xff;
        GreenInput[i] = (RGBBuffer[i] >> 8) & 0xff;
        BlueInput[i] = (RGBBuffer[i] >> 16) & 0xff;
    }
    for (i = 0; i < 256; i++)
        ColorMap[i].Red = ColorMap[i].Green = ColorMap[i].Blue = 0;

    i = 256;
    QuantizeBuffer(Width, Height, &i,
                   RedInput, GreenInput, BlueInput, GIFBuffer, ColorMap);

    free(RedInput);
    free(GreenInput);
    free(BlueInput);

    return i;    /* Real number of colors in color table. */
}
#endif /* HAVE_LIBGL_S */

/******************************************************************************
 * Update the given scan line buffer with the pixel levels of the Y line.
 * This routine is device specific, so make sure you know was you are doing
 *****************************************************************************/
static void
GetScanLine(GifPixelType * ScanLine,
            int Y) {
    
#ifdef __MSDOS__
    unsigned char ScreenByte;
    int i, j, k;
    unsigned int BufferOffset, Bit;
    union REGS InRegs, OutRegs;
#endif /* __MSDOS__ */

    switch (GraphDriver) {
#ifdef __MSDOS__
      case HERCMONO:
          BufferOffset = 0x2000 * (Y % 4) + (Y / 4) * (ScreenXMax / 8);
          /* In one scan lines we have ScreenXMax / 8 bytes: */
          for (i = 0, k = 0; i < ScreenXMax / 8; i++) {
              ScreenByte = (unsigned char)peekb(ScreenBase, BufferOffset++);
              for (j = 0, Bit = 0x80; j < 8; j++) {
                  ScanLine[k++] = (ScreenByte & Bit ? 1 : 0);
                  Bit >>= 1;
              }
          }
          break;
      case EGA:
      case EGA64:
      case EGAMONO:
      case VGA:
      case SVGA_SPECIAL:
          InRegs.x.dx = Y;
          InRegs.h.bh = 0;
          InRegs.h.ah = 0x0d;    /* BIOS Read dot. */
          for (i = 0; i < ScreenXMax; i++) {
              InRegs.x.cx = i;
              int86(0x10, &InRegs, &OutRegs);
              ScanLine[i] = OutRegs.h.al;
          }

          /* Makr this line as done by putting a xored dot on the left. */
          InRegs.x.dx = Y;
          InRegs.h.bh = 0;
          InRegs.h.ah = 0x0c;    /* BIOS Write dot (xor mode). */
          InRegs.h.al = 0x81;    /* Xor with color 1. */
          InRegs.x.cx = 0;
          int86(0x10, &InRegs, &OutRegs);
          InRegs.x.dx = Y;
          InRegs.h.bh = 0;
          InRegs.h.ah = 0x0c;    /* BIOS Write dot (xor mode). */
          InRegs.h.al = 0x81;    /* Xor with color 1. */
          InRegs.x.cx = 1;
          int86(0x10, &InRegs, &OutRegs);

          if (Y == ScreenYMax - 1) {    /* Last row - clear all marks we
                                         * made. */
              for (i = 0; i < ScreenYMax; i++) {
                  InRegs.h.bh = 0;
                  InRegs.h.ah = 0x0c;    /* BIOS Write dot (xor mode). */
                  InRegs.h.al = 0x81;    /* Xor back with color 1. */
                  InRegs.x.dx = i;
                  InRegs.x.cx = 0;
                  int86(0x10, &InRegs, &OutRegs);
                  InRegs.h.bh = 0;
                  InRegs.h.ah = 0x0c;    /* BIOS Write dot (xor mode). */
                  InRegs.h.al = 0x81;    /* Xor back with color 1. */
                  InRegs.x.dx = i;
                  InRegs.x.cx = 1;
                  int86(0x10, &InRegs, &OutRegs);
              }
          }
          break;
#endif /* __MSDOS__ */
#ifdef HAVE_LIBGL_S
      case GIF_DUMP_SGI_WINDOW:
          memcpy(ScanLine, GlblGifBufferPtr,
                 ScreenXMax * sizeof(GifPixelType));
          GlblGifBufferPtr -= ScreenXMax;
          break;
#endif /* HAVE_LIBGL_S */
#ifdef HAVE_LIBX11
      case GIF_DUMP_X_WINDOW:
          memcpy(ScanLine, GlblGifBufferPtr,
                 ScreenXMax * sizeof(GifPixelType));
          GlblGifBufferPtr += ScreenXMax;
          break;
#endif /* HAVE_LIBX11 */
      default:
          break;
    }
}

/******************************************************************************
 * Handle last GIF error. Try to close the file and free all allocated memory.
 *****************************************************************************/
static int
HandleGifError(GifFileType * GifFile) {

    int i = GifLastError();

    if (EGifCloseFile(GifFile) == GIF_ERROR) {
        GifLastError();
    }
    return i;
}
