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
 *      Bitmap stretching functions.
 *
 *      By Michael Bukin.
 *      Fixed and improved in 2007 by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



/* Information for stretching line */
static struct {
   int xcstart; /* x counter start */
   int sxinc; /* amount to increment src x every time */
   int xcdec; /* amount to deccrement counter by, increase sptr when this reaches 0 */
   int xcinc; /* amount to increment counter by when it reaches 0 */
   int linesize; /* size of a whole row of pixels */
} _al_stretch;



/* Stretcher macros */
#define DECLARE_STRETCHER(type, size, put, get) \
   int xc = _al_stretch.xcstart; \
   uintptr_t dend = dptr + _al_stretch.linesize; \
   ASSERT(dptr); \
   ASSERT(sptr); \
   for (; dptr < dend; dptr += size, sptr += _al_stretch.sxinc) { \
      put(dptr, get((type*)sptr)); \
      if (xc <= 0) { \
	 sptr += size; \
	 xc += _al_stretch.xcinc; \
      } \
      else \
	 xc -= _al_stretch.xcdec; \
   }



#define DECLARE_MASKED_STRETCHER(type, size, put, get, mask) \
   int xc = _al_stretch.xcstart; \
   uintptr_t dend = dptr + _al_stretch.linesize; \
   ASSERT(dptr); \
   ASSERT(sptr); \
   for (; dptr < dend; dptr += size, sptr += _al_stretch.sxinc) { \
      int color = get((type*)sptr); \
      if (color != mask) \
	 put(dptr, get((type*)sptr)); \
      if (xc <= 0) { \
	 sptr += size; \
	 xc += _al_stretch.xcinc; \
      } \
      else \
	 xc -= _al_stretch.xcdec; \
   }



#ifdef ALLEGRO_GFX_HAS_VGA
/*
 * Mode-X line stretcher.
 */
static void stretch_linex(uintptr_t dptr, unsigned char *sptr)
{
   int plane;
   int first_xc = _al_stretch.xcstart;
   int dw = _al_stretch.linesize;

   ASSERT(dptr);
   ASSERT(sptr);

   for (plane = 0; plane < 4; plane++) {
      int xc = first_xc;
      unsigned char *s = sptr;
      uintptr_t d = dptr / 4;
      uintptr_t dend = (dptr + dw) / 4;

      outportw(0x3C4, (0x100 << (dptr & 3)) | 2);

      for (; d < dend; d++, s += 4 * _al_stretch.sxinc) {
	 bmp_write8(d, *s);
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
      }

      /* Move to the beginning of next plane.  */
      if (first_xc <= 0) {
	  sptr++;
	  first_xc += _al_stretch.xcinc;
      }
      else
	 first_xc -= _al_stretch.xcdec;

      dptr++;
      sptr += _al_stretch.sxinc;
      dw--;
   }
}

/*
 * Mode-X masked line stretcher.
 */
static void stretch_masked_linex(uintptr_t dptr, unsigned char *sptr)
{
   int plane;
   int dw = _al_stretch.linesize;
   int first_xc = _al_stretch.xcstart;

   ASSERT(dptr);
   ASSERT(sptr);

   for (plane = 0; plane < 4; plane++) {
      int xc = first_xc;
      unsigned char *s = sptr;
      uintptr_t d = dptr / 4;
      uintptr_t dend = (dptr + dw) / 4;

      outportw(0x3C4, (0x100 << (dptr & 3)) | 2);

      for (; d < dend; d++, s += 4 * _al_stretch.sxinc) {
	 unsigned long color = *s;
	 if (color != 0)
	    bmp_write8(d, color);
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
	 if (xc <= 0) s++, xc += _al_stretch.xcinc;
	 else xc -= _al_stretch.xcdec;
      }

      /* Move to the beginning of next plane.  */
      if (first_xc <= 0) {
	 sptr++;
	 first_xc += _al_stretch.xcinc;
      }
      else
	 first_xc -= _al_stretch.xcdec;

      dptr++;
      sptr += _al_stretch.sxinc;
      dw--;
   }
}
#endif



#ifdef ALLEGRO_COLOR8
static void stretch_line8(uintptr_t dptr, unsigned char *sptr)
{
   DECLARE_STRETCHER(unsigned char, 1, bmp_write8, *);
}

static void stretch_masked_line8(uintptr_t dptr, unsigned char *sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned char, 1, bmp_write8, *, 0);
}
#endif



#ifdef ALLEGRO_COLOR16
static void stretch_line15(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_STRETCHER(unsigned short, 2, bmp_write15, *);
}

static void stretch_line16(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_STRETCHER(unsigned short, 2, bmp_write16, *);
}

static void stretch_masked_line15(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned short, 2, bmp_write15, *, MASK_COLOR_15);
}

static void stretch_masked_line16(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned short, 2, bmp_write16, *, MASK_COLOR_16);
}
#endif



#ifdef ALLEGRO_COLOR24
static void stretch_line24(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_STRETCHER(unsigned char, 3, bmp_write24, READ3BYTES);
}

static void stretch_masked_line24(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_MASKED_STRETCHER(unsigned char, 3, bmp_write24, READ3BYTES, MASK_COLOR_24);
}
#endif



#ifdef ALLEGRO_COLOR32
static void stretch_line32(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_STRETCHER(uint32_t, 4, bmp_write32, *);
}

static void stretch_masked_line32(uintptr_t dptr, unsigned char* sptr)
{
   DECLARE_MASKED_STRETCHER(uint32_t, 4, bmp_write32, *, MASK_COLOR_32);
}
#endif



/*
 * Stretch blit work-horse.
 */
static void _al_stretch_blit(BITMAP *src, BITMAP *dst,
    int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh,
   int masked)
{
   int y; /* current dst y */
   int yc; /* y counter */
   int sxofs, dxofs; /* start offsets */
   int syinc; /* amount to increment src y each time */
   int ycdec; /* amount to deccrement counter by, increase sy when this reaches 0 */
   int ycinc; /* amount to increment counter by when it reaches 0 */
   int size = 0; /* pixel size */
   int dxbeg, dxend; /* clipping information */
   int dybeg, dyend;
   int i;

   void (*stretch_line)(uintptr_t, unsigned char*) = 0;

   ASSERT(src);
   ASSERT(dst);

   /* vtable hook; not called if dest is a memory surface */   
   if (src->vtable->do_stretch_blit && !is_memory_bitmap(dst)) {
      src->vtable->do_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh, masked);
      return;
   }

   if ((sw <= 0) || (sh <= 0) || (dw <= 0) || (dh <= 0))
      return;

   /* Find out which stretcher should be used */
   if (masked) {
      switch (bitmap_color_depth(dst)) {
#ifdef ALLEGRO_COLOR8
	 case 8:
	    if (is_linear_bitmap(dst))
	       stretch_line = stretch_masked_line8;
#ifdef ALLEGRO_GFX_HAS_VGA
	    else
	       stretch_line = stretch_masked_linex;
#endif
	    size = 1;
	    break;
#endif
#ifdef ALLEGRO_COLOR16
	 case 15:
	    stretch_line = stretch_masked_line15;
	    size = 2;
	    break;
	 case 16:
	    stretch_line = stretch_masked_line16;
	    size = 2;
	    break;
#endif
#ifdef ALLEGRO_COLOR24
	 case 24:
	    stretch_line = stretch_masked_line24;
	    size = 3;
	    break;
#endif
#ifdef ALLEGRO_COLOR32
	 case 32:
	    stretch_line = stretch_masked_line32;
	    size = 4;
	    break;
#endif
      }
   }
   else {
      switch (bitmap_color_depth(dst)) {
#ifdef ALLEGRO_COLOR8
	 case 8:
	    if (is_linear_bitmap(dst))
	       stretch_line = stretch_line8;
#ifdef ALLEGRO_GFX_HAS_VGA
	    else
	       stretch_line = stretch_linex;
#endif
	    size = 1;
	    break;
#endif
#ifdef ALLEGRO_COLOR16
	 case 15:
	    stretch_line = stretch_line15;
	    size = 2;
	    break;
	 case 16:
	    stretch_line = stretch_line16;
	    size = 2;
	    break;
#endif
#ifdef ALLEGRO_COLOR24
	 case 24:
	    stretch_line = stretch_line24;
	    size = 3;
	    break;
#endif
#ifdef ALLEGRO_COLOR32
	 case 32:
	    stretch_line = stretch_line32;
	    size = 4;
	    break;
#endif
      }
   }

   ASSERT(stretch_line);

   if (dst->clip) {
      dybeg = ((dy > dst->ct) ? dy : dst->ct);
      dyend = (((dy + dh) < dst->cb) ? (dy + dh) : dst->cb);
      if (dybeg >= dyend)
	 return;

      dxbeg = ((dx > dst->cl) ? dx : dst->cl);
      dxend = (((dx + dw) < dst->cr) ? (dx + dw) : dst->cr);
      if (dxbeg >= dxend)
	 return;
   }
   else {
      dxbeg = dx;
      dxend = dx + dw;
      dybeg = dy;
      dyend = dy + dh;
   }

   syinc = sh / dh;
   ycdec = sh - (syinc*dh);
   ycinc = dh - ycdec;
   yc = ycinc;
   sxofs = sx * size;
   dxofs = dx * size;

   _al_stretch.sxinc = sw / dw * size;
   _al_stretch.xcdec = sw - ((sw/dw)*dw);
   _al_stretch.xcinc = dw - _al_stretch.xcdec;
   _al_stretch.linesize = (dxend-dxbeg)*size;

   /* get start state (clip) */
   _al_stretch.xcstart = _al_stretch.xcinc;
   for (i = 0; i < dxbeg-dx; i++, sxofs += _al_stretch.sxinc) {
      if (_al_stretch.xcstart <= 0) {
	 _al_stretch.xcstart += _al_stretch.xcinc;
	 sxofs += size;
      }
      else
	 _al_stretch.xcstart -= _al_stretch.xcdec;
   }

   dxofs += i * size;

   /* skip clipped lines */
   for (y = dy; y < dybeg; y++, sy += syinc) {
      if (yc <= 0) {
	 sy++;
	 yc += ycinc;
      }
      else
	    yc -= ycdec;
   }

   /* Stretch it */

   bmp_select(dst);

   for (; y < dyend; y++, sy += syinc) {
      (*stretch_line)(bmp_write_line(dst, y) + dxofs, src->line[sy] + sxofs);
      if (yc <= 0) {
	 sy++;
	 yc += ycinc;
      }
      else
	    yc -= ycdec;
   }
   
   bmp_unwrite_line(dst);
}



/* stretch_blit:
 *  Opaque bitmap scaling function.
 */
void stretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh,
		  int dx, int dy, int dw, int dh)
{
   ASSERT(src);
   ASSERT(dst);

   #ifdef ALLEGRO_MPW
      if (is_system_bitmap(src) && is_system_bitmap(dst))
         system_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh);
      else
   #endif
         _al_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh, 0);
}



/* masked_stretch_blit:
 *  Masked bitmap scaling function.
 */
void masked_stretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh,
                         int dx, int dy, int dw, int dh)
{
   ASSERT(src);
   ASSERT(dst);

   _al_stretch_blit(src, dst, sx, sy, sw, sh, dx, dy, dw, dh, 1);
}



/* stretch_sprite:
 *  Masked version of stretch_blit().
 */
void stretch_sprite(BITMAP *dst, BITMAP *src, int x, int y, int w, int h)
{
   ASSERT(src);
   ASSERT(dst);

   _al_stretch_blit(src, dst, 0, 0, src->w, src->h, x, y, w, h, 1);
}
