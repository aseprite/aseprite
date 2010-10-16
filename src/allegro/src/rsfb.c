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
 *      Sprite rotation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Flipping routines by Andrew Geers.
 *
 *      Optimized by Sven Sandberg.
 *
 *      Blending added by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include <math.h>


static int __col;


/* Blender for draw_trans_*_sprite.  */
/* 8 bit */
#define DTS_BLENDER8           COLOR_MAP*
#define MAKE_DTS_BLENDER8()    color_map
#define DTS_BLEND8(b,o,n)      ((b)->data[(n)& 0xFF][(o) & 0xFF])
/* 15 bit */
#define DTS_BLENDER15          BLENDER_FUNC
#define MAKE_DTS_BLENDER15()   _blender_func15
#define DTS_BLEND15(b,o,n)     ((*(b))((n), (o), _blender_alpha))
/* 16 bit */
#define DTS_BLENDER16          BLENDER_FUNC
#define MAKE_DTS_BLENDER16()   _blender_func16
#define DTS_BLEND16(b,o,n)     ((*(b))((n), (o), _blender_alpha))
/* 24 bit */
#define DTS_BLENDER24          BLENDER_FUNC
#define MAKE_DTS_BLENDER24()   _blender_func24
#define DTS_BLEND24(b,o,n)     ((*(b))((n), (o), _blender_alpha))
/* 32 bit */
#define DTS_BLENDER32          BLENDER_FUNC
#define MAKE_DTS_BLENDER32()   _blender_func32
#define DTS_BLEND32(b,o,n)     ((*(b))((n), (o), _blender_alpha))

/* Blender for draw_lit_*_sprite.  */
/* 8 bit */
#define DLS_BLENDER8            unsigned char*
#define MAKE_DLS_BLENDER8(a)    (color_map->data[(a) & 0xFF])
#define DLS_BLEND8(b,a,c)       ((b)[(c) & 0xFF])
#define DLSX_BLEND8(b,c)        ((b)[(c) & 0xFF])
/* 15 bit */
#define DLS_BLENDER15            BLENDER_FUNC
#define MAKE_DLS_BLENDER15(a)    _blender_func15
#define DLS_BLEND15(b,a,n)       ((*(b))(_blender_col_15, (n), (a)))
#define DLSX_BLEND15(b,n)        ((*(b))(_blender_col_15, (n), _blender_alpha))
/* 16 bit */
#define DLS_BLENDER16            BLENDER_FUNC
#define MAKE_DLS_BLENDER16(a)    _blender_func16
#define DLS_BLEND16(b,a,n)       ((*(b))(_blender_col_16, (n), (a)))
#define DLSX_BLEND16(b,n)        ((*(b))(_blender_col_16, (n), _blender_alpha))
/* 24 bit */
#define DLS_BLENDER24            BLENDER_FUNC
#define MAKE_DLS_BLENDER24(a)    _blender_func24
#define DLS_BLEND24(b,a,n)       ((*(b))(_blender_col_24, (n), (a)))
#define DLSX_BLEND24(b,n)        ((*(b))(_blender_col_24, (n), _blender_alpha))
/* 32 bit */
#define DLS_BLENDER32            BLENDER_FUNC
#define MAKE_DLS_BLENDER32(a)    _blender_func32
#define DLS_BLEND32(b,a,n)       ((*(b))(_blender_col_32, (n), (a)))
#define DLSX_BLEND32(b,n)        ((*(b))(_blender_col_32, (n), _blender_alpha))


#define SCANLINE_TRANS_DRAWER8(bits_pp, GETPIXEL)                     \
   static void draw_scanline_trans_##bits_pp(BITMAP *bmp, BITMAP *spr,\
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c, c2;                                                      \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DTS_BLENDER8 blender = MAKE_DTS_BLENDER8();                     \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c2 = bmp_read##bits_pp(addr);                             \
	    c = DTS_BLEND8(blender, c2, c);                           \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_TRANS_DRAWER15(bits_pp, GETPIXEL)                    \
   static void draw_scanline_trans_##bits_pp(BITMAP *bmp, BITMAP *spr,\
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c, c2;                                                      \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DTS_BLENDER15 blender = MAKE_DTS_BLENDER15();                   \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c2 = bmp_read##bits_pp(addr);                             \
	    c = DTS_BLEND15(blender, c2, c);                          \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_TRANS_DRAWER16(bits_pp, GETPIXEL)                    \
   static void draw_scanline_trans_##bits_pp(BITMAP *bmp, BITMAP *spr,\
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c, c2;                                                      \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DTS_BLENDER16 blender = MAKE_DTS_BLENDER16();                   \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c2 = bmp_read##bits_pp(addr);                             \
	    c = DTS_BLEND16(blender, c2, c);                          \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_TRANS_DRAWER24(bits_pp, GETPIXEL)                    \
   static void draw_scanline_trans_##bits_pp(BITMAP *bmp, BITMAP *spr,\
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c, c2;                                                      \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DTS_BLENDER24 blender = MAKE_DTS_BLENDER24();                   \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c2 = bmp_read##bits_pp(addr);                             \
	    c = DTS_BLEND24(blender, c2, c);                          \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_TRANS_DRAWER32(bits_pp, GETPIXEL)                    \
   static void draw_scanline_trans_##bits_pp(BITMAP *bmp, BITMAP *spr,\
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c, c2;                                                      \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DTS_BLENDER32 blender = MAKE_DTS_BLENDER32();                   \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c2 = bmp_read##bits_pp(addr);                             \
	    c = DTS_BLEND32(blender, c2, c);                          \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }


#define SCANLINE_LIT_DRAWER8(bits_pp, GETPIXEL)                       \
   static void draw_scanline_lit_##bits_pp(BITMAP *bmp, BITMAP *spr,  \
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c;                                                          \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DLS_BLENDER8 blender = MAKE_DLS_BLENDER8(__col);                \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c = DLS_BLEND8(blender, __col, c);                        \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_LIT_DRAWER15(bits_pp, GETPIXEL)                      \
   static void draw_scanline_lit_##bits_pp(BITMAP *bmp, BITMAP *spr,  \
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c;                                                          \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DLS_BLENDER15 blender = MAKE_DLS_BLENDER15(__col);              \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c = DLS_BLEND15(blender, __col, c);                       \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_LIT_DRAWER16(bits_pp, GETPIXEL)                      \
   static void draw_scanline_lit_##bits_pp(BITMAP *bmp, BITMAP *spr,  \
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c;                                                          \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DLS_BLENDER16 blender = MAKE_DLS_BLENDER16(__col);              \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c = DLS_BLEND16(blender, __col, c);                       \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_LIT_DRAWER24(bits_pp, GETPIXEL)                      \
   static void draw_scanline_lit_##bits_pp(BITMAP *bmp, BITMAP *spr,  \
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c;                                                          \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DLS_BLENDER24 blender = MAKE_DLS_BLENDER24(__col);              \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c = DLS_BLEND24(blender, __col, c);                       \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#define SCANLINE_LIT_DRAWER32(bits_pp, GETPIXEL)                      \
   static void draw_scanline_lit_##bits_pp(BITMAP *bmp, BITMAP *spr,  \
				       fixed l_bmp_x, int bmp_y_i,    \
				       fixed r_bmp_x,                 \
				       fixed l_spr_x, fixed l_spr_y,  \
				       fixed spr_dx, fixed spr_dy)    \
   {                                                                  \
      int c;                                                          \
      uintptr_t addr, end_addr;                                       \
      unsigned char **spr_line = spr->line;                           \
      DLS_BLENDER32 blender = MAKE_DLS_BLENDER32(__col);              \
                                                                      \
      r_bmp_x >>= 16;                                                 \
      l_bmp_x >>= 16;                                                 \
      bmp_select(bmp);                                                \
      addr = bmp_write_line(bmp, bmp_y_i);                            \
      end_addr = addr + r_bmp_x * ((bits_pp + 7) / 8);                \
      addr += l_bmp_x * ((bits_pp + 7) / 8);                          \
      for (; addr <= end_addr; addr += ((bits_pp + 7) / 8)) {         \
	 GETPIXEL;                                                    \
	 if (c != MASK_COLOR_##bits_pp) {                             \
	    c = DLS_BLEND32(blender, __col, c);                       \
	    bmp_write##bits_pp(addr, c);                              \
	 }                                                            \
	 l_spr_x += spr_dx;                                           \
	 l_spr_y += spr_dy;                                           \
      }                                                               \
   }



#ifdef ALLEGRO_COLOR8
   SCANLINE_TRANS_DRAWER8(8, c = spr_line[l_spr_y>>16][l_spr_x>>16]);
   SCANLINE_LIT_DRAWER8(8, c = spr_line[l_spr_y>>16][l_spr_x>>16]);
#endif

#ifdef ALLEGRO_COLOR16
   SCANLINE_TRANS_DRAWER15(15, c = ((unsigned short *)spr_line[l_spr_y>>16])
			   [l_spr_x>>16])
   SCANLINE_TRANS_DRAWER16(16, c = ((unsigned short *)spr_line[l_spr_y>>16])
			   [l_spr_x>>16])
   SCANLINE_LIT_DRAWER15(15, c = ((unsigned short *)spr_line[l_spr_y>>16])
			   [l_spr_x>>16])
   SCANLINE_LIT_DRAWER16(16, c = ((unsigned short *)spr_line[l_spr_y>>16])
			   [l_spr_x>>16])
#endif

#ifdef ALLEGRO_COLOR24
   #ifdef ALLEGRO_LITTLE_ENDIAN
      SCANLINE_TRANS_DRAWER24(24,
		      {
			 unsigned char *p = spr_line[l_spr_y>>16] +
						(l_spr_x>>16) * 3;
			 c = p[0];
			 c |= (int)p[1] << 8;
			 c |= (int)p[2] << 16;
		      })
      SCANLINE_LIT_DRAWER24(24,
		      {
			 unsigned char *p = spr_line[l_spr_y>>16] +
						(l_spr_x>>16) * 3;
			 c = p[0];
			 c |= (int)p[1] << 8;
			 c |= (int)p[2] << 16;
		      })
   #else
      SCANLINE_TRANS_DRAWER24(24,
		      {
			 unsigned char *p = spr_line[l_spr_y>>16] +
					    (l_spr_x>>16) * 3;
			 c = (int)p[0] << 16;
			 c |= (int)p[1] << 8;
			 c |= p[2];
		      })
      SCANLINE_LIT_DRAWER24(24,
		      {
			 unsigned char *p = spr_line[l_spr_y>>16] +
					    (l_spr_x>>16) * 3;
			 c = (int)p[0] << 16;
			 c |= (int)p[1] << 8;
			 c |= p[2];
		      })
   #endif
#endif

#ifdef ALLEGRO_COLOR32
   SCANLINE_TRANS_DRAWER32(32,
		   c = ((uint32_t *)spr_line[l_spr_y>>16])
		       [l_spr_x>>16])
   SCANLINE_LIT_DRAWER32(32,
		   c = ((uint32_t *)spr_line[l_spr_y>>16])
		       [l_spr_x>>16])
#endif

/* _parallelogram_map_standard_trans:
 *  Helper function for calling _parallelogram_map() with the appropriate
 *  scanline drawer. I didn't want to include this in the
 *  _parallelogram_map() function since then you can bypass it and define
 *  your own scanline drawer, eg. for anti-aliased rotations.
 */
static void _parallelogram_map_standard_trans(BITMAP *bmp, BITMAP *sprite,
				 fixed xs[4], fixed ys[4])
{
   if (is_linear_bitmap(bmp)) {
      switch (bitmap_color_depth(bmp)) {
	 #ifdef ALLEGRO_COLOR8
	    case 8:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_trans_8, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR16
	    case 15:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_trans_15, FALSE);
	       break;

	    case 16:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_trans_16, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR24
	    case 24:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_trans_24, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR32
	    case 32:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_trans_32, FALSE);
	       break;
	 #endif

	 default:
	    /* NOTREACHED */
	    ASSERT(0);
      }
   }
}


/* _parallelogram_map_standard_lit:
 *  Helper function for calling _parallelogram_map() with the appropriate
 *  scanline drawer. I didn't want to include this in the
 *  _parallelogram_map() function since then you can bypass it and define
 *  your own scanline drawer, eg. for anti-aliased rotations.
 */
static void _parallelogram_map_standard_lit(BITMAP *bmp, BITMAP *sprite,
				 fixed xs[4], fixed ys[4], int color)
{
   __col = color;

   if (is_linear_bitmap(bmp)) {
      switch (bitmap_color_depth(bmp)) {
	 #ifdef ALLEGRO_COLOR8
	    case 8:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_lit_8, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR16
	    case 15:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_lit_15, FALSE);
	       break;

	    case 16:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_lit_16, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR24
	    case 24:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_lit_24, FALSE);
	       break;
	 #endif

	 #ifdef ALLEGRO_COLOR32
	    case 32:
	       _parallelogram_map(bmp, sprite, xs, ys,
				  draw_scanline_lit_32, FALSE);
	       break;
	 #endif

	 default:
	    /* NOTREACHED */
	    ASSERT(0);
      }
   }
}


/* _pivot_scaled_sprite_flip_trans:
 *  The most generic routine to which you specify the position with angles,
 *  scales, etc.
 */
static void _pivot_scaled_sprite_flip_trans(BITMAP *bmp, BITMAP *sprite,
			       fixed x, fixed y, fixed cx, fixed cy,
			       fixed angle, fixed scale, int v_flip)
{
   fixed xs[4], ys[4];

   _rotate_scale_flip_coordinates(sprite->w << 16, sprite->h << 16,
				  x, y, cx, cy, angle, scale, scale,
				  FALSE, v_flip, xs, ys);

   _parallelogram_map_standard_trans(bmp, sprite, xs, ys);
}


/* _pivot_scaled_sprite_flip_trans:
 *  The most generic routine to which you specify the position with angles,
 *  scales, etc.
 */
static void _pivot_scaled_sprite_flip_lit(BITMAP *bmp, BITMAP *sprite,
			       fixed x, fixed y, fixed cx, fixed cy,
			       fixed angle, fixed scale, int v_flip,
			       int color)
{
   fixed xs[4], ys[4];

   _rotate_scale_flip_coordinates(sprite->w << 16, sprite->h << 16,
				  x, y, cx, cy, angle, scale, scale,
				  FALSE, v_flip, xs, ys);

   _parallelogram_map_standard_lit(bmp, sprite, xs, ys, color);
}


void rotate_sprite_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
      (y<<16) + (sprite->h * 0x10000) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, 0x10000, FALSE);
}


void rotate_sprite_v_flip_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
      (y<<16) + (sprite->h * 0x10000) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, 0x10000, TRUE);
}


void rotate_scaled_sprite_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
      (y<<16) + (sprite->h * scale) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, scale, FALSE);
}


void rotate_scaled_sprite_v_flip_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
      (y<<16) + (sprite->h * scale) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, scale, TRUE);
}


void pivot_sprite_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, FALSE);
}


void pivot_sprite_v_flip_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, TRUE);
}


void pivot_scaled_sprite_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, FALSE);
}


void pivot_scaled_sprite_v_flip_trans(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_trans(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, TRUE);
}


void rotate_sprite_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
      (y<<16) + (sprite->h * 0x10000) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, 0x10000, FALSE, color);
}


void rotate_sprite_v_flip_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, (x<<16) + (sprite->w * 0x10000) / 2,
      (y<<16) + (sprite->h * 0x10000) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, 0x10000, TRUE, color);
}


void rotate_scaled_sprite_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
      (y<<16) + (sprite->h * scale) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, scale, FALSE, color);
}


void rotate_scaled_sprite_v_flip_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, (x<<16) + (sprite->w * scale) / 2,
      (y<<16) + (sprite->h * scale) / 2,
      sprite->w << 15, sprite->h << 15,
      angle, scale, TRUE, color);
}


void pivot_sprite_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, FALSE, color);
}


void pivot_sprite_v_flip_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, 0x10000, TRUE, color);
}


void pivot_scaled_sprite_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, FALSE, color);
}


void pivot_scaled_sprite_v_flip_lit(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale, int color)
{
   ASSERT(bmp);
   ASSERT(sprite);

   _pivot_scaled_sprite_flip_lit(bmp, sprite, x<<16, y<<16, cx<<16, cy<<16, angle, scale, TRUE, color);
}

