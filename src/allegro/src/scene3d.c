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
 *      The 3D scene renderer.
 *
 *      By Calin Andrian.
 * 
 *	Merging into Allegro and API changes by Bertrand Coconnier.
 *
 *      See readme.txt for copyright information.
 */


#include <float.h>
#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

/* hash tables divide the screen height in (up to) HASH_NUM parts */
#define HASH_NUM 256
/* each part is made of 2^HASH_SHIFT lines */
#define HASH_SHIFT 3
/* Consequently, scanline sorting algorithm can handle up to 
   HASH_NUM * 2^HASH_SHIFT lines 
 */

static POLYGON_EDGE *scene_edge = NULL, *scene_inact;
static POLYGON_INFO *scene_poly = NULL;
static int scene_nedge = 0, scene_maxedge = 0;
static int scene_npoly = 0, scene_maxpoly = 0;
static BITMAP *scene_bmp;
static COLOR_MAP *scene_cmap;
static int scene_alpha;
static int last_x, scene_y;
static uintptr_t scene_addr;
static float last_z;
static POLYGON_EDGE **hash = NULL;

float scene_gap = 100.0;



/* create_scene:
 *  Allocate memory for a scene, nedge and npoly are your estimate of how
 *  many edges and how many polygons you will render (you cannot get over
 *  the limit specified here).
 *  If you use same values in succesive calls, the space will be reused.
 *  Returns negative numbers if allocations fail, zero on success.
 */
int create_scene(int nedge, int npoly)
{
   if (nedge != scene_maxedge) {        /* don't realloc, we don't need */
      scene_maxedge = 0;                /* the old data */
      if (scene_edge) _AL_FREE(scene_edge);
      scene_edge = _AL_MALLOC(nedge * sizeof(POLYGON_EDGE));
      if (!scene_edge) return -1;
   }

   if (npoly != scene_maxpoly) {
      scene_maxpoly = 0;
      if (scene_poly) _AL_FREE(scene_poly);
      scene_poly = _AL_MALLOC(npoly * sizeof(POLYGON_INFO));
      if (!scene_poly) {
         _AL_FREE(scene_edge);
         scene_edge = NULL;
         return -2;
      }
   }

   if (!hash) {
      hash = _AL_MALLOC(HASH_NUM * sizeof(POLYGON_EDGE*));
      if (!hash) {
         _AL_FREE(scene_edge);
	 _AL_FREE(scene_poly);
         return -3;
      }
   }

   scene_maxedge = nedge;
   scene_maxpoly = npoly;
   return 0;
}



/* clear_scene:
 *  Initializes a scene. The bitmap is the bitmap you will eventually render
 *  on.
 */
void clear_scene(BITMAP* bmp)
{
   int i;
   ASSERT(bmp);
   ASSERT(hash);

   scene_inact = NULL;
   scene_bmp = bmp;
   scene_nedge = scene_npoly = 0;

   for (i=0; i<HASH_NUM; i++)
      hash[i] = NULL;
}



/* destroy_scene:
 *  Deallocate memory previously allocated by create_scene.
 */
void destroy_scene(void)
{
   if (scene_edge) {
      _AL_FREE(scene_edge);
      scene_edge = NULL;
   }

   if (scene_poly) {
      _AL_FREE(scene_poly);
      scene_poly = NULL;
   }

   if (hash) {
      _AL_FREE(hash);
      hash = NULL;
   }

   scene_maxedge = scene_maxpoly = 0;
}



/* _add_edge_hash:
 *  Adds an edge structure to a linked list, returning the new head pointer.
 *  This function uses hash tables when sort_by_x == FALSE to speed up edge
 *  sorting.
 */
static POLYGON_EDGE *_add_edge_hash(POLYGON_EDGE *list, POLYGON_EDGE *edge, int sort_by_x)
{
   POLYGON_EDGE *pos = list;
   POLYGON_EDGE *prev = NULL;

   if (sort_by_x) {
      while ((pos) && (pos->x < edge->x)) {
	 prev = pos;
	 pos = pos->next;
      }
   }
   else {
      int i;
      int empty = 1, first = 1;
   
      i = edge->top >> HASH_SHIFT;
      
      ASSERT(i < HASH_NUM);

      if (hash[i]) {
         pos = hash[i];
         prev = pos->prev;
         empty = 0;
      }
   
      while ((pos) && (pos->top < edge->top)) {
         prev = pos;
         pos = pos->next;
         first = 0;
      }
      
      if (first || empty)
         hash[i] = edge;
   }
   
   edge->next = pos;
   edge->prev = prev;
   
   if (pos)
      pos->prev = edge;
      
   if (prev) {
      prev->next = edge;
      return list;
   }
   else
      return edge;
}



/* poly_plane:
 *  Computes the plane's equation coefficients A, B, C, D, in the original
 *  space (before projection) for perspective projection:
 *    Axz + Byz + Cz + D = 0
 *  This routine changed by Bertrand Coconnier in order to handle coincident
 *  vertices in the polygon (According to Ben Davis' hack in poly3d.c). 
 *  Now the function computes the polygon area in planes xy, yz, and xz in
 *  order to obtain the polygon normal. Plane equation coefficients are
 *  then computed.
 */
static void poly_plane(V3D *vtx[], POLYGON_INFO *poly, int vc)
{
   int i;
   float d;
   /* Vertices are persp-projected. Plane equation must be computed in
    * the original space, so we have to "un-persp-project" vertices.
    */
   float z0 = fixtof(vtx[0]->z);
   float x0 = fixtof(vtx[0]->x) * z0; /* "un-persp-projection" */
   float y0 = fixtof(vtx[0]->y) * z0; /* "un-persp-projection" */
   float z = fixtof(vtx[vc-1]->z);
   float x = fixtof(vtx[vc-1]->x) * z; /* "un-persp-projection" */
   float y = fixtof(vtx[vc-1]->y) * z; /* "un-persp-projection" */
   
   poly->a = (z0 + z) * (y - y0); /* area in plane yz */
   poly->b = (x0 + x) * (z - z0); /* area in plane zx */
   poly->c = (y0 + y) * (x - x0); /* area in plane xy */
   
   for(i=1; i<vc; i++) {
      z = fixtof(vtx[i]->z);
      x = fixtof(vtx[i]->x) * z;
      y = fixtof(vtx[i]->y) * z;
      poly->a += (z0 + z) * (y0 - y); /* area in plane yz */
      poly->b += (x0 + x) * (z0 - z); /* area in plane zx */
      poly->c += (y0 + y) * (x0 - x); /* area in plane xy */
      x0 = x;
      y0 = y;
      z0 = z;
   }

   /* dot product of the plane normal (poly->a, poly->b, poly->c) with
    * the vector (x, y, z)
    */
   d = poly->a * x + poly->b * y + poly->c * z;
   /* prepare A, B, C for extracting 1/z */
   if ((d < 1e-10) && (d > -1e-10)) {
      if (d >= 0)
         d = 1e-10;
      else
         d = -1e-10;
   }
   poly->a /= d; poly->b /= d; poly->c /= d;
}



/* poly_plane_f:
 *  Floating point version of poly_plane.
 *  See poly_plane comments.
 */
static void poly_plane_f(V3D_f *vtx[], POLYGON_INFO *poly, int vc)
{
   int i;
   float d;

   float z0 = vtx[0]->z;
   float x0 = vtx[0]->x * z0;
   float y0 = vtx[0]->y * z0;
   float z = vtx[vc-1]->z;
   float x = vtx[vc-1]->x * z;
   float y = vtx[vc-1]->y * z;
   
   poly->a = (z0 + z) * (y - y0);
   poly->b = (x0 + x) * (z - z0);
   poly->c = (y0 + y) * (x - x0);
   
   for(i=1; i<vc; i++) {
      z = vtx[i]->z;
      x = vtx[i]->x * z;
      y = vtx[i]->y * z;
      poly->a += (z0 + z) * (y0 - y);
      poly->b += (x0 + x) * (z0 - z);
      poly->c += (y0 + y) * (x0 - x);
      x0 = x;
      y0 = y;
      z0 = z;
   }

   d = poly->a * x + poly->b * y + poly->c * z;

   if ((d < 1e-10) && (d > -1e-10)) {
      if (d >= 0)
         d = 1e-10;
      else
         d = -1e-10;
   }
   poly->a /= d; poly->b /= d; poly->c /= d;
}



/* far_z:
 *  Compares z values of the plane to which 'edge' belongs and the plane
 *  of 'poly', in point x, y.
 *  Returns true if 'edge' is farther than 'poly'.
 */
static int far_z(int y, POLYGON_EDGE *edge, POLYGON_INFO *poly)
{
   float z1, z2, zd;

   z1 = edge->dat.z;

   /* z2 = 1/z of the plane in the point x, y */
   z2 = poly->a * fixtof(edge->x) + poly->b * y + poly->c;
   zd = (z2 - z1) * scene_gap;
   if (zd > z1)
      return 1;
   if (zd < -z1)
      return 0;

   return (z2 - z1 + 100.0 * (poly->a - edge->poly->a) > 0);
}



/* init_poly:
 *  Initializes different elements in the poly structure.
 */
static void init_poly(int type, POLYGON_INFO *poly)
{
   static int flag_table[] = {
      INTERP_Z,
      INTERP_Z,
      INTERP_Z,
      INTERP_Z,
      INTERP_Z,
      INTERP_Z | INTERP_THRU,
      INTERP_Z | INTERP_THRU,
      INTERP_Z | INTERP_BLEND,
      INTERP_Z | INTERP_BLEND,
      INTERP_Z | INTERP_THRU | INTERP_BLEND,
      INTERP_Z | INTERP_THRU | INTERP_BLEND,
      INTERP_Z | INTERP_THRU | INTERP_TRANS,
      INTERP_Z | INTERP_THRU | INTERP_TRANS,
      INTERP_Z | INTERP_THRU | INTERP_TRANS,
      INTERP_Z | INTERP_THRU | INTERP_TRANS
   };
   
   poly->alt_drawer = _optim_alternative_drawer;
   poly->inside = 0;

   type &= ~POLYTYPE_ZBUF;
   poly->flags |= flag_table[type];

   if (poly->flags & POLYTYPE_ZBUF) {
      poly->flags |= INTERP_THRU;
   }

   poly->cmap = color_map;
   poly->alpha = _blender_alpha;

   if (bitmap_color_depth(scene_bmp) == 8) {
      poly->flags &= ~INTERP_BLEND;
   } 
   else {
      if (poly->flags & INTERP_BLEND) {
         poly->b15 = _blender_col_15;
         poly->b16 = _blender_col_16;
         poly->b24 = _blender_col_24;
         poly->b32 = _blender_col_32;
      }
   }

   if ((type == POLYTYPE_FLAT) && (_drawing_mode != DRAW_MODE_SOLID)) {
      poly->flags |= INTERP_NOSOLID;
      poly->dmode = _drawing_mode;
      switch(_drawing_mode) {
         case DRAW_MODE_MASKED_PATTERN:
            poly->flags |= INTERP_THRU;
         case DRAW_MODE_COPY_PATTERN:
         case DRAW_MODE_SOLID_PATTERN:
            poly->dpat = _drawing_pattern;
            poly->xanchor = _drawing_x_anchor;
            poly->yanchor = _drawing_y_anchor;
            break;
         default:
            poly->flags |= INTERP_THRU;
            poly->dpat = NULL;
            poly->xanchor = poly->yanchor = 0;
            break;
      }
   }

   scene_npoly++;
}



/* scene_polygon3d:
 *  Put a polygon in the rendering list. Nothing is really rendered at this
 *  moment. Should be called between clear_scene() and render_scene().
 *  Unlike polygon3d(), the polygon may be concave or self-intersecting.
 *  Shapes that penetrate one another may look OK, but they are not really
 *  handled by this code.
 *  Note that the texture is stored as a pointer only, and you should keep
 *  the actual bitmap until render_scene(), where it is used.
 *  Returns zero on success, or a negative number if the polygon won't be
 *  rendered for lack of rendering routine.
 */
int scene_polygon3d(int type, BITMAP *texture, int vc, V3D *vtx[])
{
   int c;
   V3D *v1, *v2;
   POLYGON_EDGE *edge;
   POLYGON_INFO *poly;

   ASSERT(scene_nedge + vc <= scene_maxedge);
   ASSERT(scene_npoly < scene_maxpoly);

   edge = &scene_edge[scene_nedge];
   poly = &scene_poly[scene_npoly];

   /* set up the drawing mode */
   poly->drawer = _get_scanline_filler(type, &poly->flags, &poly->info,
                                      texture, scene_bmp);
   if (!poly->drawer)
      return -1;

   init_poly(type, poly);
   poly->color = vtx[0]->c;
   poly_plane(vtx, poly, vc);

   v2 = vtx[vc-1];

   /* fill the edge table */
   for (c=0; c<vc; c++) {
      v1 = v2;
      v2 = vtx[c];

      if (_fill_3d_edge_structure(edge, v1, v2, poly->flags, scene_bmp)) {
         edge->poly = poly;
	 scene_inact = _add_edge_hash(scene_inact, edge, FALSE);
	 edge++;
         scene_nedge++;
      }
   }
   return 0;
}



/* scene_polygon3d_f:
 *  Floating point version of scene_polygon3d.
 */
int scene_polygon3d_f(int type, BITMAP *texture, int vc, V3D_f *vtx[])
{
   int c;
   V3D_f *v1, *v2;
   POLYGON_EDGE *edge;
   POLYGON_INFO *poly;

   ASSERT(scene_nedge + vc <= scene_maxedge);
   ASSERT(scene_npoly < scene_maxpoly);

   edge = &scene_edge[scene_nedge];
   poly = &scene_poly[scene_npoly];

   /* set up the drawing mode */
   poly->drawer = _get_scanline_filler(type, &poly->flags, &poly->info,
                                      texture, scene_bmp);
   if (!poly->drawer)
      return -1;

   init_poly(type, poly);
   poly->color = vtx[0]->c;
   poly_plane_f(vtx, poly, vc);

   v2 = vtx[vc-1];

   /* fill the edge table */
   for (c=0; c<vc; c++) {
      v1 = v2;
      v2 = vtx[c];

      if (_fill_3d_edge_structure_f(edge, v1, v2, poly->flags, scene_bmp)) {
         edge->poly = poly;
	 scene_inact = _add_edge_hash(scene_inact, edge, FALSE);
	 edge++;
         scene_nedge++;
      }
   }
   return 0;
}



/* scene_segment:
 *  Draws a piece of the scanline, corresponding to a polygon. The start and
 *  end values for x are taken from e01 and e02, the polygon style from poly.
 */
static void scene_segment(POLYGON_EDGE *e01, POLYGON_EDGE *e02,
                          POLYGON_INFO *poly)
{
   int x, w, gap, flags;
   fixed step, width;
   float w1, step_f;
   int x01 = fixceil(e01->x);
   int x02 = fixceil(e02->x);
   float z01 = e01->dat.z;
   POLYGON_EDGE *e1 = poly->left_edge;
   POLYGON_EDGE *e2 = poly->right_edge;
   POLYGON_SEGMENT *info = &poly->info, *dat1, *dat2;
   SCANLINE_FILLER drawer;

   if ((x01 < last_x) && (z01 < last_z))
      x01 = last_x;
   if (scene_bmp->clip) {
      if (x01 < scene_bmp->cl)
         x01 = scene_bmp->cl;
      if (x02 > scene_bmp->cr)
         x02 = scene_bmp->cr;
   }
   if (x01 >= x02)
      return;

   if (!e2) {
      e2 = e02;
      while (e2 && (e2->poly != poly))
        e2 = e2->next;

      if (!e2) return;
      poly->right_edge = e2;
   }

   x = fixceil(e1->x);
   w = fixceil(e2->x) - x;
   if (w <= 0) return;

   /* Subpixel and subtexel accuracy */
   step = (x << 16) - e1->x;
   step_f = fixtof(step);
   width = e2->x - e1->x;
   w1 = 65536. / width;

   flags = poly->flags;
   dat1 = &e1->dat;
   dat2 = &e2->dat;

   if (flags & INTERP_FLAT) {
      info->c = poly->color;
   } 
   else {
      if (flags & INTERP_1COL) {
         info->dc = fixdiv(dat2->c - dat1->c, width);
         info->c = dat1->c + fixmul(step, info->dc);
      }

      if (flags & INTERP_3COL) {
         info->dr = fixdiv(dat2->r - dat1->r, width);
         info->dg = fixdiv(dat2->g - dat1->g, width);
         info->db = fixdiv(dat2->b - dat1->b, width);
         info->r = dat1->r + fixmul(step, info->dr);
         info->g = dat1->g + fixmul(step, info->dg);
         info->b = dat1->b + fixmul(step, info->db);
      }

      if (flags & INTERP_FIX_UV) {
         info->du = fixdiv(dat2->u - dat1->u, width);
         info->dv = fixdiv(dat2->v - dat1->v, width);
         info->u = dat1->u + fixmul(step, info->du);
         info->v = dat1->v + fixmul(step, info->dv);
      }

      if (flags & INTERP_FLOAT_UV) {
         info->dfu = (dat2->fu - dat1->fu) * w1;
         info->dfv = (dat2->fv - dat1->fv) * w1;
         info->fu = dat1->fu + step_f * info->dfu;
         info->fv = dat1->fv + step_f * info->dfv;
      }
   }

   info->dz = (dat2->z - dat1->z) * w1;
   info->z = dat1->z + step_f * info->dz;

   if (x < x01) {
      gap = x01 - x;
      x = x01;
      w -= gap;
      _clip_polygon_segment_f(info, gap, poly->flags);
   }

   if (x + w >= x02)
      w = x02 - x;

   if (w <= 0) return;

   if ((flags & OPT_FLOAT_UV_TO_FIX) && (info->dz == 0)) {
      float z1 = 1. / info->z;
      info->u = info->fu * z1;
      info->v = info->fv * z1;
      info->du = info->dfu * z1;
      info->dv = info->dfv * z1;
      drawer = poly->alt_drawer;
   } 
   else
      drawer = poly->drawer;

   color_map = poly->cmap;
   _blender_alpha = poly->alpha;
   if (flags & INTERP_BLEND) {
      _blender_col_15 = poly->b15;
      _blender_col_16 = poly->b16;
      _blender_col_24 = poly->b24;
      _blender_col_32 = poly->b32;
   }

   if (drawer == _poly_scanline_dummy) {
      if (flags & INTERP_NOSOLID) {
         drawing_mode(poly->dmode, poly->dpat, poly->xanchor, poly->yanchor);
         scene_bmp->vtable->hfill(scene_bmp, x, scene_y, x+w-1, poly->color);
         solid_mode();
      }
      else
         scene_bmp->vtable->hfill(scene_bmp, x, scene_y, x+w-1, poly->color);
   } 
   else {
      int dx = x * BYTES_PER_PIXEL(bitmap_color_depth(scene_bmp));
      if (flags & INTERP_ZBUF)
         info->zbuf_addr = bmp_write_line(_zbuffer, scene_y) + x * sizeof(float);

      info->read_addr = bmp_read_line(scene_bmp, scene_y) + dx;
      drawer(scene_addr + dx, w, info);
   }
}



/* scene_trans_seg:
 *  Checks whether p0 is visible either directly or through transparent
 *  polygons on top of it. If it's visible, draws all the visible stack
 *  with x values from e1 and e2. At entry, p is the top polygon.
 *  Returns nonzero if something was drawn.
 */
static int scene_trans_seg(POLYGON_EDGE *e1, POLYGON_EDGE *e2,
                           POLYGON_INFO *p0, POLYGON_INFO *p)
{
   int c;

   if (!p) return 0;
   c = 1;
   while (1) {
      if (p == p0) c = 0;
      if (!(p->flags & INTERP_THRU) || !p->next) break;
      p = p->next;
   }
   if (c) return 0;

   /* p is first opaque or the very last */
   while (p) {
      scene_segment(e1, e2, p);
      p = p->prev;
   }
   return 1;
}



/* render_scene:
 *  Renders all the specified scene_polygon3d()'s on the bitmap passed to
 *  clear_scene(). Rendering is done one scanline at a time, with no pixel
 *  being processed more than once. Note that between clear_scene() and
 *  render_scene() you shouldn't change the clip rectangle of the destination
 *  bitmap. Also, all the textures passed to scene_polygon3d() are stored
 *  as pointers only and actually used in render_scene().
 */
void render_scene(void)
{
   int p;
   #ifdef ALLEGRO_DOS
      int old87 = 0;
   #endif
   POLYGON_EDGE *edge, *start_edge = NULL;
   POLYGON_EDGE *active_edges = NULL, *last_edge = NULL;
   POLYGON_INFO *active_poly = NULL;

   ASSERT(scene_maxedge > 0);
   ASSERT(scene_maxpoly > 0);
   
   scene_cmap = color_map;
   scene_alpha = _blender_alpha;
   solid_mode();
   /* set fpu to single-precision, truncate mode */
   #ifdef ALLEGRO_DOS
      old87 = _control87(PC_24 | RC_CHOP, MCW_PC | MCW_RC);
   #endif

   acquire_bitmap(scene_bmp);
   bmp_select(scene_bmp);

   for (p=0; p<scene_npoly; p++) {
      scene_poly[p].inside = 0;
   }

   /* for each scanline in the clip region... */
   for (scene_y=scene_bmp->ct; scene_y<scene_bmp->cb; scene_y++) {
      scene_addr = bmp_write_line(scene_bmp, scene_y);

      /* check for newly active edges */
      edge = scene_inact;
      while ((edge) && (edge->top == scene_y)) {
	 POLYGON_EDGE *next_edge = edge->next;
	 scene_inact = _remove_edge(scene_inact, edge);
	 active_edges = _add_edge_hash(active_edges, edge, TRUE);
	 edge = next_edge;
      }

      /* no edges on this line */
      if (!active_edges) continue;

      /* fill the scanline */
      last_x = INT_MIN;
      last_z = 0.0;
      for (edge=active_edges; edge; edge=edge->next) {
         int x = fixceil(edge->x);
         POLYGON_INFO *poly = edge->poly;

         /* one polygon changes status */
         poly->inside = 1 - poly->inside;
         if (poly->inside) {
            POLYGON_INFO *pos = active_poly;
            POLYGON_INFO *prev = NULL;

            poly->left_edge = edge;
	    poly->right_edge = NULL;
	    
            /* find its place in the list */
            while (pos && far_z(scene_y, edge, pos)) {
               prev = pos;
               pos = pos->next;
            }
            /* poly overlaps pos. Was pos visible ? */
            if (scene_trans_seg(start_edge, edge, pos, active_poly)) {
               start_edge = edge;
            }
            /* link */
            poly->next = pos;
            poly->prev = prev;
            if (pos) pos->prev = poly;
            if (prev) 
	       prev->next = poly;
            else {
               start_edge = edge;
               active_poly = poly;
            }
         } 
	 else {
            poly->right_edge = edge;
            /* poly ends here. Was it visible ? */
            if (scene_trans_seg(start_edge, edge, poly, active_poly)) {
               start_edge = edge;
               if (x > last_x) {
                  last_x = x;
                  last_z = edge->dat.z;
               }
            }
            /* unlink */
            if (poly->next) 
	       poly->next->prev = poly->prev;
            if (poly->prev)
	       poly->prev->next = poly->next;
            else
               active_poly = poly->next;
         }
         /* prepare for backward scan */
         last_edge = edge;
      }

      /* update edges, remove dead ones, re-sort */
      edge = last_edge;
      active_edges = NULL;

      while (edge) {
	 POLYGON_EDGE *prev_edge = edge->prev;
	 if (scene_y < edge->bottom) {
            POLYGON_SEGMENT *dat = &edge->dat;
            int flags = edge->poly->flags;

	    edge->x += edge->dx;
            dat->z += dat->dz;

	    if (!(flags & INTERP_FLAT)) {
	       if (flags & INTERP_1COL)
	          dat->c += dat->dc;

               if (flags & INTERP_3COL) {
	          dat->r += dat->dr;
	          dat->g += dat->dg;
	          dat->b += dat->db;
	       }

	       if (flags & INTERP_FIX_UV) {
	          dat->u += dat->du;
	          dat->v += dat->dv;
	       }

	       if (flags & INTERP_FLOAT_UV) {
	          dat->fu += dat->dfu;
	          dat->fv += dat->dfv;
	       }
	    }

            active_edges = _add_edge_hash(active_edges, edge, TRUE);
	 }
	 edge = prev_edge;
      }
   }

   bmp_unwrite_line(scene_bmp);
   release_bitmap(scene_bmp);

   /* reset fpu mode */
   #ifdef ALLEGRO_DOS
      _control87(old87, MCW_PC | MCW_RC);
   #endif
   color_map = scene_cmap;
   _blender_alpha = scene_alpha;
   solid_mode();

   /* mark the tables as full */
   scene_nedge = scene_maxedge;
   scene_npoly = scene_maxpoly;
}
