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
 *      The 3D polygon clipper.
 *
 *      By Calin Andrian.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



#define INT_NONE        0
#define INT_1COL        1
#define INT_3COL        2
#define INT_3COLP       4
#define INT_UV          8



/* point_inside:
 *  Copies a vertex to the output table.
 */
#define point_inside(vv)                                 \
{                                                        \
   v3->x = v2->x; v3->y = v2->y; v3->z = v2->z;          \
   v3->u = v2->u; v3->v = v2->v; v3->c = v2->c;          \
   vv++;                                                 \
}



/* point_interp_f:
 *  Interpolates between v1 and v2, with slope "t", resulting vertex v3.
 */
#define point_interp_f(vv)                               \
{                                                        \
   v3->x = (v2->x - v1->x) * t + v1->x;                  \
   v3->y = (v2->y - v1->y) * t + v1->y;                  \
   v3->z = (v2->z - v1->z) * t + v1->z;                  \
							 \
   if (flags & INT_1COL) {                               \
      v3->c = (int)((v2->c - v1->c) * t + v1->c);        \
   }                                                     \
   else if (flags & INT_3COLP) {                         \
      int bpp = bitmap_color_depth(screen);              \
      int r = (int)((getr_depth(bpp, v2->c) - getr_depth(bpp, v1->c)) * t + getr_depth(bpp, v1->c)); \
      int g = (int)((getg_depth(bpp, v2->c) - getg_depth(bpp, v1->c)) * t + getg_depth(bpp, v1->c)); \
      int b = (int)((getb_depth(bpp, v2->c) - getb_depth(bpp, v1->c)) * t + getb_depth(bpp, v1->c)); \
      v3->c = makecol_depth(bpp, r&255, g&255, b&255);   \
   }                                                     \
   else if (flags & INT_3COL) {                          \
      int r = (int)(((v2->c & 0xFF0000) - (v1->c & 0xFF0000)) * t + (v1->c & 0xFF0000)); \
      int g = (int)(((v2->c & 0x00FF00) - (v1->c & 0x00FF00)) * t + (v1->c & 0x00FF00)); \
      int b = (int)(((v2->c & 0x0000FF) - (v1->c & 0x0000FF)) * t + (v1->c & 0x0000FF)); \
      v3->c = (r & 0xFF0000) | (g & 0x00FF00) | (b & 0x0000FF); \
   }                                                     \
   if (flags & INT_UV) {                                 \
      v3->u = (v2->u - v1->u) * t + v1->u;               \
      v3->v = (v2->v - v1->v) * t + v1->v;               \
   }                                                     \
   vv++;                                                 \
}



/* clip3d_f:
 *  Clips a 3D polygon against planes x = -z, x = z, y = -z, y = z,
 *  z = min_z, z = max_z. If max_z <= min_z the z = max_z clipping is
 *  not done. Type is the rendering style - POLYTYPE_*, vc is the 
 *  number of vertices, vtx contains the input vertices, vout will 
 *  receive the result, vtmp and out must be supplied for temporary 
 *  storage. The size of arrays vout, vtmp and out should be twice as 
 *  big as vtx.
 */
int clip3d_f(int type, float min_z, float max_z, int vc, AL_CONST V3D_f *vtx[], V3D_f *vout[], V3D_f *vtmp[], int out[])
{
   int i, j, vo, vt, flags;
   float t;
   V3D_f *v3;

   AL_CONST V3D_f *v1, *v2, **vin;

   static int flag_table[] = {
      INT_NONE,                             /* flat */
      INT_3COLP,                            /* gcol */
      INT_3COL,                             /* grgb */
      INT_UV,                               /* atex */
      INT_UV,                               /* ptex */
      INT_UV,                               /* atex mask */
      INT_UV,                               /* ptex mask */
      INT_UV + INT_1COL,                    /* atex lit */
      INT_UV + INT_1COL,                    /* ptex lit */
      INT_UV + INT_1COL,                    /* atex mask lit */
      INT_UV + INT_1COL,                    /* ptex mask lit */
      INT_UV,                               /* atex trans */
      INT_UV,                               /* ptex trans */
      INT_UV,                               /* atex mask trans */
      INT_UV                                /* ptex mask trans */
   };

   type &= ~POLYTYPE_ZBUF;
   flags = flag_table[type];

   if (max_z > min_z) {
      vt = 0;

      for (i=0; i<vc; i++)
	 out[i] = (vtx[i]->z > max_z);

      for (i=0, j=vc-1; i<vc; j=i, i++) {
	 v1 = vtx[j];
	 v2 = vtx[i];
	 v3 = vtmp[vt];

	 if ((out[j] & out[i]) != 0)
	    continue;

	 if ((out[j] | out[i]) == 0) {
	    point_inside(vt);
	    continue;
	 }

	 t = (max_z - v1->z) / (v2->z - v1->z);
	 point_interp_f(vt);
	 v3 = vtmp[vt];

	 if (out[j])
	    point_inside(vt);
      }
      vin = (AL_CONST V3D_f**)vtmp;
   }
   else {
      vt = vc;
      vin = vtx;
   }

   vo = 0;

   for (i=0; i<vt; i++)
      out[i] = (vin[i]->z < min_z);

   for (i=0, j=vt-1; i<vt; j=i, i++) {
      v1 = vin[j];
      v2 = vin[i];
      v3 = vout[vo];

      if ((out[j] & out[i]) != 0)
	 continue;

      if ((out[j] | out[i]) == 0) {
	 point_inside(vo);
	 continue;
      }

      t = (min_z - v1->z) / (v2->z - v1->z);
      point_interp_f(vo);
      v3 = vout[vo];

      if (out[j])
	 point_inside(vo);
   }

   vt = 0;

   for (i=0; i<vo; i++)
      out[i] = (vout[i]->x < -vout[i]->z);

   for (i=0, j=vo-1; i<vo; j=i, i++) {
      v1 = vout[j];
      v2 = vout[i];
      v3 = vtmp[vt];

      if ((out[j] & out[i]) != 0)
	 continue;

      if ((out[j] | out[i]) == 0) {
	 point_inside(vt);
	 continue;
      }

      t = (-v1->z - v1->x) / (v2->x - v1->x + v2->z - v1->z);
      point_interp_f(vt);
      v3 = vtmp[vt];

      if (out[j])
	 point_inside(vt);
   }

   vo = 0;

   for (i=0; i<vt; i++)
      out[i] = (vtmp[i]->x > vtmp[i]->z);

   for (i=0, j=vt-1; i<vt; j=i, i++) {
      v1 = vtmp[j];
      v2 = vtmp[i];
      v3 = vout[vo];

      if ((out[j] & out[i]) != 0)
	 continue;

      if ((out[j] | out[i]) == 0) {
	 point_inside(vo);
	 continue;
      }

      t = (v1->z - v1->x) / (v2->x - v1->x - v2->z + v1->z);
      point_interp_f(vo);
      v3 = vout[vo];

      if (out[j])
	 point_inside(vo);
   }

   vt = 0;

   for (i=0; i<vo; i++)
      out[i] = (vout[i]->y < -vout[i]->z);

   for (i=0, j=vo-1; i<vo; j=i, i++) {
      v1 = vout[j];
      v2 = vout[i];
      v3 = vtmp[vt];

      if ((out[j] & out[i]) != 0)
	 continue;

      if ((out[j] | out[i]) == 0) {
	 point_inside(vt);
	 continue;
      }

      t = (-v1->z - v1->y) / (v2->y - v1->y + v2->z - v1->z);
      point_interp_f(vt);
      v3 = vtmp[vt];

      if (out[j])
	 point_inside(vt);
   }

   vo = 0;

   for (i=0; i<vt; i++)
      out[i] = (vtmp[i]->y > vtmp[i]->z);

   for (i=0, j=vt-1; i<vt; j=i, i++) {
      v1 = vtmp[j];
      v2 = vtmp[i];
      v3 = vout[vo];

      if ((out[j] & out[i]) != 0)
	 continue;

      if ((out[j] | out[i]) == 0) {
	 point_inside(vo);
	 continue;
      }

      t = (v1->z - v1->y) / (v2->y - v1->y - v2->z + v1->z);
      point_interp_f(vo);
      v3 = vout[vo];

      if (out[j])
	 point_inside(vo);
   }

   if (type == POLYTYPE_FLAT)
      vout[0]->c = vtx[0]->c;

   return vo;
}
