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
 *      3D polygon drawing routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_3D_H
#define ALLEGRO_3D_H

#include "base.h"
#include "fixed.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct BITMAP;

typedef struct V3D                  /* a 3d point (fixed point version) */
{
   fixed x, y, z;                   /* position */
   fixed u, v;                      /* texture map coordinates */
   int c;                           /* color */
} V3D;


typedef struct V3D_f                /* a 3d point (floating point version) */
{
   float x, y, z;                   /* position */
   float u, v;                      /* texture map coordinates */
   int c;                           /* color */
} V3D_f;


#define POLYTYPE_FLAT               0
#define POLYTYPE_GCOL               1
#define POLYTYPE_GRGB               2
#define POLYTYPE_ATEX               3
#define POLYTYPE_PTEX               4
#define POLYTYPE_ATEX_MASK          5
#define POLYTYPE_PTEX_MASK          6
#define POLYTYPE_ATEX_LIT           7
#define POLYTYPE_PTEX_LIT           8
#define POLYTYPE_ATEX_MASK_LIT      9
#define POLYTYPE_PTEX_MASK_LIT      10
#define POLYTYPE_ATEX_TRANS         11
#define POLYTYPE_PTEX_TRANS         12
#define POLYTYPE_ATEX_MASK_TRANS    13
#define POLYTYPE_PTEX_MASK_TRANS    14
#define POLYTYPE_MAX                15
#define POLYTYPE_ZBUF               16

AL_VAR(float, scene_gap);

AL_FUNC(void, _soft_polygon3d, (struct BITMAP *bmp, int type, struct BITMAP *texture, int vc, V3D *vtx[]));
AL_FUNC(void, _soft_polygon3d_f, (struct BITMAP *bmp, int type, struct BITMAP *texture, int vc, V3D_f *vtx[]));
AL_FUNC(void, _soft_triangle3d, (struct BITMAP *bmp, int type, struct BITMAP *texture, V3D *v1, V3D *v2, V3D *v3));
AL_FUNC(void, _soft_triangle3d_f, (struct BITMAP *bmp, int type, struct BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3));
AL_FUNC(void, _soft_quad3d, (struct BITMAP *bmp, int type, struct BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4));
AL_FUNC(void, _soft_quad3d_f, (struct BITMAP *bmp, int type, struct BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4));
AL_FUNC(int, clip3d, (int type, fixed min_z, fixed max_z, int vc, AL_CONST V3D *vtx[], V3D *vout[], V3D *vtmp[], int out[]));
AL_FUNC(int, clip3d_f, (int type, float min_z, float max_z, int vc, AL_CONST V3D_f *vtx[], V3D_f *vout[], V3D_f *vtmp[], int out[]));

AL_FUNC(fixed, polygon_z_normal, (AL_CONST V3D *v1, AL_CONST V3D *v2, AL_CONST V3D *v3));
AL_FUNC(float, polygon_z_normal_f, (AL_CONST V3D_f *v1, AL_CONST V3D_f *v2, AL_CONST V3D_f *v3));

/* Note: You are not supposed to mix ZBUFFER with BITMAP even though it is
 * currently possible. This is just the internal representation, and it may
 * change in the future.
 */
typedef struct BITMAP ZBUFFER;

AL_FUNC(ZBUFFER *, create_zbuffer, (struct BITMAP *bmp));
AL_FUNC(ZBUFFER *, create_sub_zbuffer, (ZBUFFER *parent, int x, int y, int width, int height));
AL_FUNC(void, set_zbuffer, (ZBUFFER *zbuf));
AL_FUNC(void, clear_zbuffer, (ZBUFFER *zbuf, float z));
AL_FUNC(void, destroy_zbuffer, (ZBUFFER *zbuf));

AL_FUNC(int, create_scene, (int nedge, int npoly));
AL_FUNC(void, clear_scene, (struct BITMAP* bmp));
AL_FUNC(void, destroy_scene, (void));
AL_FUNC(int, scene_polygon3d, (int type, struct BITMAP *texture, int vx, V3D *vtx[]));
AL_FUNC(int, scene_polygon3d_f, (int type, struct BITMAP *texture, int vx, V3D_f *vtx[]));
AL_FUNC(void, render_scene, (void));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_3D_H */


