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
 *      Z-buffered polygon filler helpers (gouraud shading, tmapping, etc).
 *
 *      Original routines by Michael Bukin.
 *      Modified to support z-buffered polygon drawing by Bertrand Coconnier
 *
 *      See readme.txt for copyright information.
 */

#define ZBUF_PTR	float*

#ifndef __bma_czscan_h
#define __bma_czscan_h

/* _poly_zbuf_flat:
 *  Fills a single-color polygon scanline.
 */
void FUNC_POLY_ZBUF_FLAT(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   float z;
   unsigned long c;
   PIXEL_PTR d;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   z = info->z;
   c = info->c;
   d = (PIXEL_PTR) addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PUT_PIXEL(d, c);
         *zb = z;
      }
      zb++;
      z += info->dz;
   }
}



#ifdef _bma_zbuf_gcol

/* _poly_zbuf_gcol:
 *  Fills a single-color gouraud shaded polygon scanline.
 */
void FUNC_POLY_ZBUF_GCOL(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   float z;
   fixed c, dc;
   PIXEL_PTR d;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   z = info->z;
   c = info->c;
   dc = info->dc;
   d = (PIXEL_PTR) addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PUT_PIXEL(d, (c >> 16));
         *zb = z;
      }
      c += dc;
      zb++;
      z += info->dz;
   }
}

#endif /* _bma_zbuf_gcol */



/* _poly_zbuf_grgb:
 *  Fills an gouraud shaded polygon scanline.
 */
void FUNC_POLY_ZBUF_GRGB(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   fixed r, g, b, dr, dg, db;
   PIXEL_PTR d;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   r = info->r;
   g = info->g;
   b = info->b;
   dr = info->dr;
   dg = info->dg;
   db = info->db;
   d = (PIXEL_PTR) addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PUT_RGB(d, (r >> 16), (g >> 16), (b >> 16));
         *zb = z;
      }
      r += dr;
      g += dg;
      b += db;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex:
 *  Fills an affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed u, v, du, dv;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   u = info->u;
   v = info->v;
   du = info->du;
   dv = info->dv;
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         PUT_PIXEL(d, color);
         *zb = z;
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_mask:
 *  Fills a masked affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_MASK(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed u, v, du, dv;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   u = info->u;
   v = info->v;
   du = info->du;
   dv = info->dv;
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    PUT_PIXEL(d, color);
            *zb = z;
         }
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_lit:
 *  Fills a lit affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_LIT(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed u, v, c, du, dv, dc;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   PS_BLENDER blender;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   u = info->u;
   v = info->v;
   c = info->c;
   du = info->du;
   dv = info->dv;
   dc = info->dc;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_BLEND(blender, (c >> 16), color);

         PUT_PIXEL(d, color);
         *zb = z;
      }
      u += du;
      v += dv;
      c += dc;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_mask_lit:
 *  Fills a masked lit affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_MASK_LIT(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed u, v, c, du, dv, dc;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   PS_BLENDER blender;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   u = info->u;
   v = info->v;
   c = info->c;
   du = info->du;
   dv = info->dv;
   dc = info->dc;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    color = PS_BLEND(blender, (c >> 16), color);
	    PUT_PIXEL(d, color);
            *zb = z;
         }
      }
      u += du;
      v += dv;
      c += dc;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_ptex:
 *  Fills a perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   double fu, fv, fz, dfu, dfv, dfz;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   fu = info->fu;
   fv = info->fv;
   fz = info->z;
   dfu = info->dfu;
   dfv = info->dfv;
   dfz = info->dz;
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         PUT_PIXEL(d, color);
         *zb = (float) fz;
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}



/* _poly_zbuf_ptex_mask:
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_MASK(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   double fu, fv, fz, dfu, dfv, dfz;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   fu = info->fu;
   fv = info->fv;
   fz = info->z;
   dfu = info->dfu;
   dfv = info->dfv;
   dfz = info->dz;
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    PUT_PIXEL(d, color);
            *zb = (float) fz;
         }
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}



/* _poly_zbuf_ptex_lit:
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_LIT(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed c, dc;
   double fu, fv, fz, dfu, dfv, dfz;
   PS_BLENDER blender;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   c = info->c;
   dc = info->dc;
   fu = info->fu;
   fv = info->fv;
   fz = info->z;
   dfu = info->dfu;
   dfv = info->dfv;
   dfz = info->dz;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_BLEND(blender, (c >> 16), color);

         PUT_PIXEL(d, color);
         *zb = (float) fz;
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      c += dc;
      zb++;
   }
}



/* _poly_zbuf_ptex_mask_lit:
 *  Fills a masked lit perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_MASK_LIT(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed c, dc;
   double fu, fv, fz, dfu, dfv, dfz;
   PS_BLENDER blender;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   c = info->c;
   dc = info->dc;
   fu = info->fu;
   fv = info->fv;
   fz = info->z;
   dfu = info->dfu;
   dfv = info->dfv;
   dfz = info->dz;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);

         if (!IS_MASK(color)) {
	    color = PS_BLEND(blender, (c >> 16), color);
	    PUT_PIXEL(d, color);
            *zb = (float) fz;
         }
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      c += dc;
      zb++;
   }
}



/* _poly_zbuf_atex_trans:
 *  Fills a trans affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_TRANS(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed u, v, du, dv;
   PS_BLENDER blender;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   PIXEL_PTR r;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   u = info->u;
   v = info->v;
   du = info->du;
   dv = info->dv;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   r = (PIXEL_PTR) info->read_addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));

         PUT_PIXEL(d, color);
         *zb = z;
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_atex_mask_trans:
 *  Fills a trans masked affine texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_ATEX_MASK_TRANS(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   fixed u, v, du, dv;
   PS_BLENDER blender;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   PIXEL_PTR r;
   float z;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   u = info->u;
   v = info->v;
   du = info->du;
   dv = info->dv;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   r = (PIXEL_PTR) info->read_addr;
   z = info->z;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < z) {
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
	 if (!IS_MASK(color)) {
            color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));
            PUT_PIXEL(d, color);
            *zb = z;
	 }
      }
      u += du;
      v += dv;
      zb++;
      z += info->dz;
   }
}



/* _poly_zbuf_ptex_trans:
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_TRANS(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   double fu, fv, fz, dfu, dfv, dfz;
   PS_BLENDER blender;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   PIXEL_PTR r;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   fu = info->fu;
   fv = info->fv;
   fz = info->z;
   dfu = info->dfu;
   dfv = info->dfv;
   dfz = info->dz;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   r = (PIXEL_PTR) info->read_addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
         color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));

         PUT_PIXEL(d, color);
         *zb = (float) fz;
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}



/* _poly_zbuf_ptex_mask_trans:
 *  Fills a trans masked perspective correct texture mapped polygon scanline.
 */
void FUNC_POLY_ZBUF_PTEX_MASK_TRANS(uintptr_t addr, int w, POLYGON_SEGMENT *info)
{
   int x;
   int vmask, vshift, umask;
   double fu, fv, fz, dfu, dfv, dfz;
   PS_BLENDER blender;
   PIXEL_PTR texture;
   PIXEL_PTR d;
   PIXEL_PTR r;
   ZBUF_PTR zb;

   ASSERT(addr);
   ASSERT(info);

   vmask = info->vmask << info->vshift;
   vshift = 16 - info->vshift;
   umask = info->umask;
   fu = info->fu;
   fv = info->fv;
   fz = info->z;
   dfu = info->dfu;
   dfv = info->dfv;
   dfz = info->dz;
   blender = MAKE_PS_BLENDER();
   texture = (PIXEL_PTR) (info->texture);
   d = (PIXEL_PTR) addr;
   r = (PIXEL_PTR) info->read_addr;
   zb = (ZBUF_PTR) info->zbuf_addr;

   for (x = w - 1; x >= 0; INC_PIXEL_PTR(d), INC_PIXEL_PTR(r), x--) {
      if (*zb < fz) {
         long u = fu / fz;
         long v = fv / fz;
         PIXEL_PTR s = OFFSET_PIXEL_PTR(texture, ((v >> vshift) & vmask) + ((u >> 16) & umask));
         unsigned long color = GET_MEMORY_PIXEL(s);
	 if (!IS_MASK(color)) {
            color = PS_ALPHA_BLEND(blender, color, GET_PIXEL(r));
            PUT_PIXEL(d, color);
            *zb = (float) fz;
	 }
      }
      fu += dfu;
      fv += dfv;
      fz += dfz;
      zb++;
   }
}

#endif /* !__bma_czscan_h */

