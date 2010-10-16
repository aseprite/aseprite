/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Functions for drawing to GDI without using DirectX.
 *
 *      By Marian Dvorsky.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif


static HPALETTE current_hpalette = NULL;



/* set_gdi_color_format:
 *  Sets right values for pixel color format to work with GDI.
 */
void set_gdi_color_format(void)
{
   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;

   _rgb_r_shift_16 = 11;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;

   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;

   _rgb_r_shift_32 = 16;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 0;
}



/* destroy_current_hpalette:
 *  Destroys the current Windows PALETTE.
 */
static void destroy_current_hpalette(void)
{
   if (current_hpalette) {
      DeleteObject(current_hpalette);
      current_hpalette = NULL;
   }

   _remove_exit_func(destroy_current_hpalette);
}



/* set_palette_to_hdc:
 *  Selects and realizes an Allegro PALETTE to a Windows DC.
 */
void set_palette_to_hdc(HDC dc, PALETTE pal)
{
   PALETTEENTRY palPalEntry[256];
   int i;

   if (current_hpalette) {
      for (i = 0; i < 256; i++) {
	 palPalEntry[i].peRed = _rgb_scale_6[pal[i].r];
	 palPalEntry[i].peGreen = _rgb_scale_6[pal[i].g];
	 palPalEntry[i].peBlue = _rgb_scale_6[pal[i].b];
	 palPalEntry[i].peFlags = 0;
      }

      SetPaletteEntries(current_hpalette, 0, 256, (LPPALETTEENTRY) & palPalEntry);
   }
   else {
      current_hpalette = convert_palette_to_hpalette(pal);
      _add_exit_func(destroy_current_hpalette,
		     "destroy_current_hpalette");
   }

   SelectPalette(dc, current_hpalette, FALSE);
   RealizePalette(dc);
   select_palette(pal);
}



/* convert_palette_to_hpalette:
 *  Converts an Allegro PALETTE to a Windows PALETTE.
 */
HPALETTE convert_palette_to_hpalette(PALETTE pal)
{
   HPALETTE hpal;
   LOGPALETTE *lp;
   int i;

   lp = (LOGPALETTE *) _AL_MALLOC(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * 256);
   if (!lp)
      return NULL;

   lp->palNumEntries = 256;
   lp->palVersion = 0x300;

   for (i = 0; i < 256; i++) {
      lp->palPalEntry[i].peRed = _rgb_scale_6[pal[i].r];
      lp->palPalEntry[i].peGreen = _rgb_scale_6[pal[i].g];
      lp->palPalEntry[i].peBlue = _rgb_scale_6[pal[i].b];
      lp->palPalEntry[i].peFlags = 0;
   }

   hpal = CreatePalette(lp);

   _AL_FREE(lp);

   return hpal;
}



/* convert_hpalette_to_palette:
 *  Converts a Windows PALETTE to an Allegro PALETTE.
 */
void convert_hpalette_to_palette(HPALETTE hpal, PALETTE pal)
{
   PALETTEENTRY lp[256];
   int i;

   GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY) & lp);

   for (i = 0; i < 256; i++) {
      pal[i].r = lp[i].peRed >> 2;
      pal[i].g = lp[i].peGreen >> 2;
      pal[i].b = lp[i].peBlue >> 2;
   }
}



/* get_bitmap_info:
 *  Returns a BITMAPINFO structure suited to an Allegro BITMAP.
 *  You have to free the memory allocated by this function.
 */
static BITMAPINFO *get_bitmap_info(BITMAP *bitmap, PALETTE pal)
{
   BITMAPINFO *bi;
   int bpp, i;

   bi = (BITMAPINFO *) _AL_MALLOC(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);

   bpp = bitmap_color_depth(bitmap);
   if (bpp == 15)
      bpp = 16;

   ZeroMemory(&bi->bmiHeader, sizeof(BITMAPINFOHEADER));

   bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bi->bmiHeader.biBitCount = bpp;
   bi->bmiHeader.biPlanes = 1;
   bi->bmiHeader.biWidth = bitmap->w;
   bi->bmiHeader.biHeight = -bitmap->h;
   bi->bmiHeader.biClrUsed = 256;
   bi->bmiHeader.biCompression = BI_RGB;

   if (pal) {
      for (i = 0; i < 256; i++) {
	 bi->bmiColors[i].rgbRed = _rgb_scale_6[pal[i].r];
	 bi->bmiColors[i].rgbGreen = _rgb_scale_6[pal[i].g];
	 bi->bmiColors[i].rgbBlue = _rgb_scale_6[pal[i].b];
	 bi->bmiColors[i].rgbReserved = 0;
      }
   }

   return bi;
}



/* get_dib_from_bitmap:
 *  Creates a Windows device-independent bitmap (DIB) from an Allegro BITMAP.
 *  You have to free the memory allocated by this function.
 */
static BYTE *get_dib_from_bitmap(BITMAP *bitmap)
{
   int bpp;
   int x, y;
   int pitch;
   int col;
   BYTE *pixels;
   BYTE *src, *dst;

   bpp = bitmap_color_depth(bitmap);
   pitch = bitmap->w * BYTES_PER_PIXEL(bpp);
   pitch = (pitch + 3) & ~3;	/* align on dword */

   pixels = (BYTE *) _AL_MALLOC_ATOMIC(bitmap->h * pitch);
   if (!pixels)
      return NULL;

   switch (bpp) {

      case 8:
	 for (y = 0; y < bitmap->h; y++) {
	    memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w);
	 }
	 break;

      case 15:
	 if ((_rgb_r_shift_15 == 10) && (_rgb_g_shift_15 == 5) && (_rgb_b_shift_15 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w * 2);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       src = bitmap->line[y];
	       dst = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(WORD *) (src);
		  *((WORD *) (dst)) = (WORD) ((getb15(col) >> 3) | ((getg15(col) >> 3) << 5) | ((getr15(col) >> 3) << 10));
		  src += 2;
		  dst += 2;
	       }
	    }
	 }
	 break;

      case 16:
	 /* the format of a 16-bit DIB is 5-5-5 as above */
	 for (y = 0; y < bitmap->h; y++) {
	    src = bitmap->line[y];
	    dst = pixels + y * pitch;

	    for (x = 0; x < bitmap->w; x++) {
	       col = *(WORD *) (src);
	       *((WORD *) (dst)) = (WORD) ((getb16(col) >> 3) | ((getg16(col) >> 3) << 5) | ((getr16(col) >> 3) << 10));
	       src += 2;
	       dst += 2;
	    }
	 }
	 break;

      case 24:
	 if ((_rgb_r_shift_24 == 16) && (_rgb_g_shift_24 == 8) && (_rgb_b_shift_24 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w * 3);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       src = bitmap->line[y];
	       dst = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(DWORD *) (src);
		  src += 3;
		  *(dst++) = getb24(col);
		  *(dst++) = getg24(col);
		  *(dst++) = getr24(col);
	       }
	    }
	 }
	 break;

      case 32:
	 if ((_rgb_r_shift_32 == 16) && (_rgb_g_shift_32 == 8) && (_rgb_b_shift_32 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w * 4);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       src = bitmap->line[y];
	       dst = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(DWORD *) (src);
		  src += 4;
		  *(dst++) = getb32(col);
		  *(dst++) = getg32(col);
		  *(dst++) = getr32(col);
		  dst++;
	       }
	    }
	 }
	 break;
   }

   return pixels;
}



/* get_dib_from_hbitmap:
 *  Creates a Windows device-independent bitmap (DIB) from a Windows BITMAP.
 *  You have to free the memory allocated by this function.
 */
static BYTE *get_dib_from_hbitmap(int bpp, HBITMAP hbitmap)
{
   BITMAPINFOHEADER bi;
   BITMAPINFO *binfo;
   HDC hdc;
   HPALETTE hpal, holdpal;
   int col;
   WINDOWS_BITMAP bm;
   int pitch;
   BYTE *pixels;
   BYTE *ptr;
   int x, y;

   if (!hbitmap)
      return NULL;

   if (bpp == 15)
      bpp = 16;

   if (!GetObject(hbitmap, sizeof(bm), (LPSTR) & bm))
      return NULL;

   pitch = bm.bmWidth * BYTES_PER_PIXEL(bpp);
   pitch = (pitch + 3) & ~3;	/* align on dword */

   pixels = (BYTE *) _AL_MALLOC_ATOMIC(bm.bmHeight * pitch);
   if (!pixels)
      return NULL;

   ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));

   bi.biSize = sizeof(BITMAPINFOHEADER);
   bi.biBitCount = bpp;
   bi.biPlanes = 1;
   bi.biWidth = bm.bmWidth;
   bi.biHeight = -abs(bm.bmHeight);
   bi.biClrUsed = 256;
   bi.biCompression = BI_RGB;

   binfo = _AL_MALLOC(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);
   binfo->bmiHeader = bi;

   hdc = GetDC(NULL);

   hpal = convert_palette_to_hpalette(_current_palette);
   holdpal = SelectPalette(hdc, hpal, TRUE);
   RealizePalette(hdc);

   GetDIBits(hdc, hbitmap, 0, bm.bmHeight, pixels, binfo, DIB_RGB_COLORS);

   ptr = pixels;

   /* This never occurs, because if screen or memory bitmap is 8-bit, 
    * we ask Windows to convert it to truecolor. It is safer, but a little 
    * bit slower.
    */

   if (bpp == 8) {
      for (y = 0; y < bm.bmWidth; y++) {
	 for (x = 0; x < bm.bmHeight; x++) {
	    col = *ptr;

	    if ((col < 10) || (col >= 246)) {
	       /* we have to remap colors from system palette */
	       *(ptr++) = makecol8(binfo->bmiColors[col].rgbRed, binfo->bmiColors[col].rgbGreen, binfo->bmiColors[col].rgbBlue);
	    }
	    else {
	       /* our palette is shifted by 10 */
	       *(ptr++) = col - 10;
	    }
	 }
      }
   }

   _AL_FREE(binfo);

   SelectPalette(hdc, holdpal, TRUE);
   DeleteObject(hpal);
   ReleaseDC(NULL, hdc);

   return pixels;
}



/* get_bitmap_from_dib:
 *  Creates an Allegro BITMAP from a Windows device-independent bitmap (DIB).
 */
static BITMAP *get_bitmap_from_dib(int bpp, int w, int h, BYTE *pixels)
{
   int x, y;
   int pitch;
   int col;
   int b, g, r;
   BYTE *src, *dst;
   BITMAP *bitmap;

   bitmap = create_bitmap_ex(bpp, w, h);
   pitch = bitmap->w * BYTES_PER_PIXEL(bpp);
   pitch = (pitch + 3) & ~3;	/* align on dword */

   switch (bpp) {

      case 8:
	 for (y = 0; y < bitmap->h; y++) {
	    memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w);
	 }
	 break;

      case 15:
	 if ((_rgb_r_shift_15 == 10) && (_rgb_g_shift_15 == 5) && (_rgb_b_shift_15 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w * 2);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       dst = bitmap->line[y];
	       src = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(WORD *) (src);
		  *((WORD *) (dst)) = makecol15(_rgb_scale_5[(col >> 10) & 0x1F], _rgb_scale_5[(col >> 5) & 0x1F], _rgb_scale_5[col & 0x1F]);
		  src += 2;
		  dst += 2;
	       }
	    }
	 }
	 break;

      case 16:
	 /* the format of a 16-bit DIB is 5-5-5 as above */
	 for (y = 0; y < bitmap->h; y++) {
	    dst = bitmap->line[y];
	    src = pixels + y * pitch;

	    for (x = 0; x < bitmap->w; x++) {
	       col = *(WORD *) (src);
	       *((WORD *) (dst)) = makecol16(_rgb_scale_5[(col >> 10) & 0x1F], _rgb_scale_5[(col >> 5) & 0x1F], _rgb_scale_5[col & 0x1F]);
	       src += 2;
	       dst += 2;
	    }
	 }
	 break;

      case 24:
	 if ((_rgb_r_shift_24 == 16) && (_rgb_g_shift_24 == 8) && (_rgb_b_shift_24 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w * 3);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       dst = bitmap->line[y];
	       src = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  r = *(src++);
		  g = *(src++);
		  b = *(src++);
		  col = makecol24(r, g, b);
		  *((WORD *) dst) = (col & 0xFFFF);
		  dst += 2;
		  *(dst++) = (col >> 16);
	       }
	    }
	 }
	 break;

      case 32:
	 if ((_rgb_r_shift_32 == 16) && (_rgb_g_shift_32 == 8) && (_rgb_b_shift_32 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w * 4);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       dst = bitmap->line[y];
	       src = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  b = *(src++);
		  g = *(src++);
		  r = *(src++);
		  col = makecol32(r, g, b);
		  src++;
		  *((DWORD *) dst) = col;
		  dst += 4;
	       }
	    }
	 }
	 break;
   }

   return bitmap;
}



/* convert_bitmap_to_hbitmap:
 *  Converts an Allegro BITMAP to a Windows BITMAP.
 */
HBITMAP convert_bitmap_to_hbitmap(BITMAP *bitmap)
{
   HDC hdc;
   HBITMAP hbmp;
   BITMAPINFO *bi;
   HPALETTE hpal, holdpal;
   BYTE *pixels;

   /* get the DIB first */
   bi = get_bitmap_info(bitmap, NULL);
   pixels = get_dib_from_bitmap(bitmap);
   hpal = convert_palette_to_hpalette(_current_palette);

   /* now that we have the DIB, convert it to a DDB */
   hdc = GetDC(NULL);
   holdpal = SelectPalette(hdc, hpal, TRUE);
   RealizePalette(hdc);
   hbmp = CreateDIBitmap(hdc, &bi->bmiHeader, CBM_INIT, pixels, bi, DIB_RGB_COLORS);
   ReleaseDC(NULL, hdc);

   SelectPalette(hdc, holdpal, TRUE);
   DeleteObject(hpal);

   _AL_FREE(pixels);
   _AL_FREE(bi);

   return hbmp;
}



/* convert_hbitmap_to_bitmap:
 *  Converts a Windows BITMAP to an Allegro BITMAP.
 */
BITMAP *convert_hbitmap_to_bitmap(HBITMAP bitmap)
{
   BYTE *pixels;
   BITMAP *bmp;
   WINDOWS_BITMAP bm;
   int bpp;

   if (!GetObject(bitmap, sizeof(bm), (LPSTR) & bm))
      return NULL;

   if (bm.bmBitsPixel == 8) {
      /* ask windows to save truecolor image, then convert to our format */
      bpp = 24;
   }
   else
      bpp = bm.bmBitsPixel;

   /* get the DIB first */
   pixels = get_dib_from_hbitmap(bpp, bitmap);

   /* now that we have the DIB, convert it to a BITMAP */
   bmp = get_bitmap_from_dib(bpp, bm.bmWidth, bm.bmHeight, pixels);

   _AL_FREE(pixels);

   return bmp;
}



/* draw_to_hdc:
 *  Draws an entire Allegro BITMAP to a Windows DC. Has a syntax similar to draw_sprite().
 */
void draw_to_hdc(HDC dc, BITMAP *bitmap, int x, int y)
{
   stretch_blit_to_hdc(bitmap, dc, 0, 0, bitmap->w, bitmap->h, x, y, bitmap->w, bitmap->h);
}



/* blit_to_hdc:
 *  Blits an Allegro BITMAP to a Windows DC. Has a syntax similar to blit().
 */
void blit_to_hdc(BITMAP *bitmap, HDC dc, int src_x, int src_y, int dest_x, int dest_y, int w, int h)
{
   stretch_blit_to_hdc(bitmap, dc, src_x, src_y, w, h, dest_x, dest_y, w, h);
}



/* stretch_blit_to_hdc:
 *  Blits an Allegro BITMAP to a Windows DC. Has a syntax similar to stretch_blit().
 */
void stretch_blit_to_hdc(BITMAP *bitmap, HDC dc, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h)
{
   AL_CONST int bottom_up_src_y = bitmap->h - src_y - src_h;
   BYTE *pixels;
   BITMAPINFO *bi;

   bi = get_bitmap_info(bitmap, _current_palette);
   pixels = get_dib_from_bitmap(bitmap);

   /* Windows treats all source bitmaps as bottom-up when using StretchDIBits
    * unless the source (x,y) is (0,0).  To work around this buggy behavior, we
    * can use negative heights to reverse the direction of the blits.
    *
    * See <http://wiki.allegro.cc/StretchDIBits> for a detailed explanation.
    */
   if (bottom_up_src_y == 0 && src_x == 0 && src_h != bitmap->h) {
      StretchDIBits(dc, dest_x, dest_h+dest_y-1, dest_w, -dest_h,
	 src_x, bitmap->h-src_y+1, src_w, -src_h, pixels, bi,
	 DIB_RGB_COLORS, SRCCOPY);
   }
   else {
      StretchDIBits(dc, dest_x, dest_y, dest_w, dest_h,
	 src_x, bottom_up_src_y, src_w, src_h, pixels, bi,
	 DIB_RGB_COLORS, SRCCOPY);
   }

   _AL_FREE(pixels);
   _AL_FREE(bi);
}



/* blit_from_hdc:
 *  Blits from a Windows DC to an Allegro BITMAP. Has a syntax similar to blit().
 */
void blit_from_hdc(HDC dc, BITMAP *bitmap, int src_x, int src_y, int dest_x, int dest_y, int w, int h)
{
   stretch_blit_from_hdc(dc, bitmap, src_x, src_y, w, h, dest_x, dest_y, w, h);
}



/* stretch_blit_from_hdc:
 *  Blits from a Windows DC to an Allegro BITMAP. Has a syntax similar to stretch_blit().
 */
void stretch_blit_from_hdc(HDC dc, BITMAP *bitmap, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h)
{
   HBITMAP hbmp, holdbmp;
   HDC hmemdc;
   BITMAP *newbmp;

   hmemdc = CreateCompatibleDC(dc);
   hbmp = CreateCompatibleBitmap(dc, dest_w, dest_h);

   holdbmp = SelectObject(hmemdc, hbmp);

   StretchBlt(hmemdc, 0, 0, dest_w, dest_h, dc, src_x, src_y, src_w, src_h, SRCCOPY);
   SelectObject(hmemdc, holdbmp);

   newbmp = convert_hbitmap_to_bitmap(hbmp);
   blit(newbmp, bitmap, 0, 0, dest_x, dest_y, dest_w, dest_h);

   destroy_bitmap(newbmp);
   DeleteObject(hbmp);
   DeleteDC(hmemdc);
}
