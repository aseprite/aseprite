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
 *      DirectDraw bitmap management routines.
 *
 *      By Stefan Schimanski.
 *
 *      Improved page flipping mechanism by Robin Burrows.
 *
 *      Improved video bitmap allocation scheme by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#define PREFIX_I                "al-wddbmp INFO: "
#define PREFIX_W                "al-wddbmp WARNING: "
#define PREFIX_E                "al-wddbmp ERROR: "


/* The video bitmap allocation scheme works as follows:
 *  - the screen is allocated as a single DirectDraw surface (primary or overlay,
 *    depending on the driver) at startup,
 *  - the first video bitmap reuses the DirectDraw surface of the screen which is
 *    then assigned to flipping_page[0],
 *  - the second video bitmap allocates flipping_page[1] and uses it as its 
 *    DirectDraw surface; it also destroys the single surface pointed to by
 *    flipping_page[0] and creates a DirectDraw flipping chain whose frontbuffer
 *    is connected back to flipping_page[0] and backbuffer to flipping_page[1],
 *  - the third video bitmap allocates flipping_page[2] and uses it as its 
 *    DirectDraw surface; it also destroys the flipping chain pointed to by
 *    flipping_page[0] and flipping_page[1] and creates a new flipping chain
 *    whose frontbuffer is connected back to flipping_page[0], first backbuffer
 *    back to flipping_page[1] and second backbuffer to flipping_page[2].
 *
 * When a video bitmap is to be destroyed, the flipping chain (if it exists) is
 * destroyed and recreated with one less backbuffer, and its surfaces are assigned
 * back to the flipping_page[] array in order. If the video bitmap is not attached
 * to the surface that was just destroyed, its surface is assigned to the video
 * bitmap parent of the just destroyed surface.
 *
 * After triple buffering setup:
 *
 *  screen   video_bmp[0]  video_bmp[1]      video_bmp[2]
 *      \         |              |                |
 *       \        |              |                |
 *       page_flipping[0]  page_flipping[1]  page_flipping[2]
 *        (frontbuffer)-----(backbuffer1)-----(backbuffer2)
 *
 * After request_video_bitmap(video_bmp[1]):
 *
 *  screen   video_bmp[0]  video_bmp[1]      video_bmp[2]
 *      \                \/                        |
 *       \               /\                        |
 *       page_flipping[0]  page_flipping[1]  page_flipping[2]
 *        (frontbuffer)-----(backbuffer1)-----(backbuffer2)
 *
 * This ensures that every video bitmap keeps track of the actual part of the
 * video memory it represents (see the documentation of DirectDrawSurface::Flip).
 */

static DDRAW_SURFACE *flipping_page[3] = {NULL, NULL, NULL};
static int n_flipping_pages = 0;



/* dd_err:
 *  Returns a DirectDraw error string.
 */
#ifdef DEBUGMODE
static char *dd_err(long err)
{
   static char err_str[64];

   switch (err) {

      case DD_OK:
         _al_sane_strncpy(err_str, "DD_OK", sizeof(err_str));
         break;

      case DDERR_GENERIC:
         _al_sane_strncpy(err_str, "DDERR_GENERIC", sizeof(err_str));
         break;

     case DDERR_INCOMPATIBLEPRIMARY:
         _al_sane_strncpy(err_str, "DDERR_INCOMPATIBLEPRIMARY", sizeof(err_str));
         break;

     case DDERR_INVALIDCAPS:
         _al_sane_strncpy(err_str, "DDERR_INVALIDCAPS", sizeof(err_str));
         break;

      case DDERR_INVALIDOBJECT:
         _al_sane_strncpy(err_str, "DDERR_INVALIDOBJECT", sizeof(err_str));
         break;

      case DDERR_INVALIDPARAMS:
         _al_sane_strncpy(err_str, "DDERR_INVALIDPARAMS", sizeof(err_str));
         break;

     case DDERR_INVALIDPIXELFORMAT:
         _al_sane_strncpy(err_str, "DDERR_INVALIDPIXELFORMAT", sizeof(err_str));
         break;

      case DDERR_NOFLIPHW:
         _al_sane_strncpy(err_str, "DDERR_NOFLIPHW", sizeof(err_str));
         break;

      case DDERR_NOTFLIPPABLE:
         _al_sane_strncpy(err_str, "DDERR_NOTFLIPPABLE", sizeof(err_str));
         break;

      case DDERR_OUTOFMEMORY:
         _al_sane_strncpy(err_str, "DDERR_OUTOFMEMORY", sizeof(err_str));
         break;

      case DDERR_OUTOFVIDEOMEMORY:
         _al_sane_strncpy(err_str, "DDERR_OUTOFVIDEOMEMORY", sizeof(err_str));
         break;

     case DDERR_PRIMARYSURFACEALREADYEXISTS:
         _al_sane_strncpy(err_str, "DDERR_PRIMARYSURFACEALREADYEXISTS", sizeof(err_str));
         break;

      case DDERR_SURFACEBUSY:
         _al_sane_strncpy(err_str, "DDERR_SURFACEBUSY", sizeof(err_str));
         break;

      case DDERR_SURFACELOST:
         _al_sane_strncpy(err_str, "DDERR_SURFACELOST", sizeof(err_str));
         break;

     case DDERR_UNSUPPORTED:
         _al_sane_strncpy(err_str, "DDERR_UNSUPPORTED", sizeof(err_str));
         break;

     case DDERR_UNSUPPORTEDMODE:
         _al_sane_strncpy(err_str, "DDERR_UNSUPPORTEDMODE", sizeof(err_str));
         break;

     case DDERR_WASSTILLDRAWING:
         _al_sane_strncpy(err_str, "DDERR_WASSTILLDRAWING", sizeof(err_str));
         break;

      default:
         _al_sane_strncpy(err_str, "DDERR_UNKNOWN", sizeof(err_str));
         break;
   }

   return err_str;
}
#else
#define dd_err(hr) "\0"
#endif



/* create_directdraw2_surface:
 *  Wrapper around DirectDraw2::CreateSurface taking the surface characteristics
 *  as parameters. Returns a DirectDrawSurface2 interface if successful.
 */
static LPDIRECTDRAWSURFACE2 create_directdraw2_surface(int w, int h, LPDDPIXELFORMAT pixel_format, int type, int n_backbuffers)
{
   DDSURFACEDESC ddsurf_desc;
   LPDIRECTDRAWSURFACE ddsurf1;
   LPVOID ddsurf2;
   HRESULT hr;

   /* describe surface characteristics */
   memset(&ddsurf_desc, 0, sizeof(DDSURFACEDESC));
   ddsurf_desc.dwSize = sizeof(DDSURFACEDESC);
   ddsurf_desc.dwFlags = DDSD_CAPS;

   switch (type) {

      case DDRAW_SURFACE_PRIMARY:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
         _TRACE(PREFIX_I "Creating primary surface...");
         break;

      case DDRAW_SURFACE_OVERLAY:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY;
         ddsurf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         ddsurf_desc.dwHeight = h;
         ddsurf_desc.dwWidth = w;

         if (pixel_format) {    /* use pixel format */
            ddsurf_desc.dwFlags |= DDSD_PIXELFORMAT;
            ddsurf_desc.ddpfPixelFormat = *pixel_format;
         }

         _TRACE(PREFIX_I "Creating overlay surface...");
         break;

      case DDRAW_SURFACE_VIDEO:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN;
         ddsurf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         ddsurf_desc.dwHeight = h;
         ddsurf_desc.dwWidth = w;
         _TRACE(PREFIX_I "Creating video surface...");
         break;

      case DDRAW_SURFACE_SYSTEM:
         ddsurf_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
         ddsurf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
         ddsurf_desc.dwHeight = h;
         ddsurf_desc.dwWidth = w;

         if (pixel_format) {    /* use pixel format */
            ddsurf_desc.dwFlags |= DDSD_PIXELFORMAT;
            ddsurf_desc.ddpfPixelFormat = *pixel_format;
         }

         _TRACE(PREFIX_I "Creating system surface...");
         break;

      default:
         _TRACE(PREFIX_E "Unknown surface type\n");
         return NULL;
   }

   /* set backbuffer requirements */
   if (n_backbuffers > 0) {
      ddsurf_desc.ddsCaps.dwCaps |=  DDSCAPS_COMPLEX | DDSCAPS_FLIP;
      ddsurf_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
      ddsurf_desc.dwBackBufferCount = n_backbuffers;
      _TRACE("...with %d backbuffer(s)\n", n_backbuffers);
   }

   /* create the surface */
   hr = IDirectDraw2_CreateSurface(directdraw, &ddsurf_desc, &ddsurf1, NULL);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Unable to create the surface (%s)\n", dd_err(hr));
      return NULL;
   }

   /* retrieve the DirectDrawSurface2 interface */
   hr = IDirectDrawSurface_QueryInterface(ddsurf1, &IID_IDirectDrawSurface2, &ddsurf2);

   /* there is a bug in the COM part of DirectX 3:
    *  If we release the DirectSurface interface, the actual
    *  object is also released. It is fixed in DirectX 5.
    */
   if (_dx_ver >= 0x500)
      IDirectDrawSurface_Release(ddsurf1);

   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Unable to retrieve the DirectDrawSurface2 interface (%s)\n", dd_err(hr));
      return NULL;
   }

   return (LPDIRECTDRAWSURFACE2)ddsurf2;
}



/* gfx_directx_create_surface:
 *  Creates a DirectDraw surface.
 */ 
DDRAW_SURFACE *gfx_directx_create_surface(int w, int h, LPDDPIXELFORMAT pixel_format, int type)
{
   DDRAW_SURFACE *surf;

   surf = _AL_MALLOC(sizeof(DDRAW_SURFACE));
   if (!surf)
      return NULL;

   /* create the surface with the specified characteristics */
   surf->id = create_directdraw2_surface(w, h, pixel_format,type, 0);
   if (!surf->id) {
      _AL_FREE(surf);
      return NULL;
   }

   surf->flags = type;
   surf->lock_nesting = 0;

   register_ddraw_surface(surf);

   return surf;
}



/* gfx_directx_destroy_surface:
 *  Destroys a DirectDraw surface.
 */
void gfx_directx_destroy_surface(DDRAW_SURFACE *surf)
{
   IDirectDrawSurface2_Release(surf->id);
   unregister_ddraw_surface(surf);
   _AL_FREE(surf);
}



/* gfx_directx_make_bitmap_from_surface:
 *  Connects a DirectDraw surface with an Allegro bitmap.
 */
BITMAP *gfx_directx_make_bitmap_from_surface(DDRAW_SURFACE *surf, int w, int h, int id)
{
   BITMAP *bmp;
   int i;

   bmp = (BITMAP *) _AL_MALLOC(sizeof(BITMAP) + sizeof(char *) * h);
   if (!bmp)
      return NULL;

   bmp->w =w;
   bmp->cr = w;
   bmp->h = h;
   bmp->cb = h;
   bmp->clip = TRUE;
   bmp->cl = 0;
   bmp->ct = 0;
   bmp->vtable = &_screen_vtable;
   bmp->write_bank = gfx_directx_write_bank;
   bmp->read_bank = gfx_directx_write_bank;
   bmp->dat = NULL;
   bmp->id = id;
   bmp->extra = NULL;
   bmp->x_ofs = 0;
   bmp->y_ofs = 0;
   bmp->seg = _video_ds();
   for (i = 0; i < h; i++)
      bmp->line[i] = pseudo_surf_mem;

   bmp->extra = surf;

   return bmp;
}



/* gfx_directx_created_sub_bitmap:
 */
void gfx_directx_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   bmp->extra = parent;
}



/* recreate_flipping_chain:
 *  Destroys the previous flipping chain and creates a new one.
 */
static int recreate_flipping_chain(int n_pages)
{
   int w, h, type, n_backbuffers;
   DDSCAPS ddscaps;
   HRESULT hr;

   ASSERT(n_pages > 0);

   /* set flipping chain characteristics */
   w = gfx_directx_forefront_bitmap->w;
   h = gfx_directx_forefront_bitmap->h;
   type = flipping_page[0]->flags & DDRAW_SURFACE_TYPE_MASK;
   n_backbuffers = n_pages - 1;

   /* release existing flipping chain */
   if (flipping_page[0]->id) {
      hr = IDirectDrawSurface2_Release(flipping_page[0]->id);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "Unable to release the primary surface (%s)", dd_err(hr));
         return -1;
      }
   }

   /* create the new flipping chain with the specified characteristics */
   flipping_page[0]->id = create_directdraw2_surface(w, h, ddpixel_format, type, n_backbuffers);
   if (!flipping_page[0]->id) 
      return -1;

   /* retrieve the backbuffers */
   if (n_backbuffers > 0) {
      memset(&ddscaps, 0, sizeof(DDSCAPS));

      /* first backbuffer */
      ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
      hr = IDirectDrawSurface2_GetAttachedSurface(flipping_page[0]->id, &ddscaps, &flipping_page[1]->id);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "Unable to retrieve the first backbuffer (%s)", dd_err(hr));
         return -1;
      }

      flipping_page[1]->flags = flipping_page[0]->flags;
      flipping_page[1]->lock_nesting = 0;

      if (n_backbuffers > 1) {
         /* second backbuffer */
         ddscaps.dwCaps = DDSCAPS_FLIP;
         hr = IDirectDrawSurface2_GetAttachedSurface(flipping_page[1]->id, &ddscaps, &flipping_page[2]->id);
         if (FAILED(hr)) {
            _TRACE(PREFIX_E "Unable to retrieve the second backbuffer (%s)", dd_err(hr));
            return -1;
         }

         flipping_page[2]->flags = flipping_page[0]->flags;
         flipping_page[2]->lock_nesting = 0;
      }
   }

   /* attach the global palette if needed */
   if (flipping_page[0]->flags & DDRAW_SURFACE_INDEXED) {
      hr = IDirectDrawSurface2_SetPalette(flipping_page[0]->id, ddpalette);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "Unable to attach the global palette (%s)", dd_err(hr));
         return -1;
      }
   }

   return 0;
}



/* gfx_directx_create_video_bitmap:
 */
BITMAP *gfx_directx_create_video_bitmap(int width, int height)
{
   DDRAW_SURFACE *surf;
   BITMAP *bmp;

   /* try to detect page flipping and triple buffering patterns */
   if ((width == gfx_directx_forefront_bitmap->w) && (height == gfx_directx_forefront_bitmap->h)) {

      switch (n_flipping_pages) {

         case 0:
            /* recycle the forefront surface as the first flipping page */
            flipping_page[0] = DDRAW_SURFACE_OF(gfx_directx_forefront_bitmap);
            bmp = gfx_directx_make_bitmap_from_surface(flipping_page[0], width, height, BMP_ID_VIDEO);
            if (bmp) {
               flipping_page[0]->parent_bmp = bmp;
               n_flipping_pages++;
               return bmp;
            }
            else {
               flipping_page[0] = NULL;
               return NULL;
            }

         case 1:
         case 2:
            /* try to attach an additional page to the flipping chain */
            flipping_page[n_flipping_pages] = _AL_MALLOC(sizeof(DDRAW_SURFACE));
            if (recreate_flipping_chain(n_flipping_pages+1) == 0) {
               bmp = gfx_directx_make_bitmap_from_surface(flipping_page[n_flipping_pages], width, height, BMP_ID_VIDEO);
               if (bmp) {
                  flipping_page[n_flipping_pages]->parent_bmp = bmp;
                  n_flipping_pages++;
                  return bmp;
               }
            }

            recreate_flipping_chain(n_flipping_pages);
            _AL_FREE(flipping_page[n_flipping_pages]);
            flipping_page[n_flipping_pages] = NULL;
            return NULL;
      }
   }

   /* create the DirectDraw surface */
   if (ddpixel_format)
      surf = gfx_directx_create_surface(width, height, ddpixel_format, DDRAW_SURFACE_SYSTEM);
   else
      surf = gfx_directx_create_surface(width, height, NULL, DDRAW_SURFACE_VIDEO);

   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = gfx_directx_make_bitmap_from_surface(surf, width, height, BMP_ID_VIDEO);
   if (!bmp) {
      gfx_directx_destroy_surface(surf);
      return NULL;
   }

   return bmp;
}



/* gfx_directx_destroy_video_bitmap:
 */
void gfx_directx_destroy_video_bitmap(BITMAP *bmp)
{
   DDRAW_SURFACE *surf, *tail_page;

   surf = DDRAW_SURFACE_OF(bmp);

   if ((surf == flipping_page[0]) || (surf == flipping_page[1]) || (surf == flipping_page[2])) {
      /* handle surfaces belonging to the flipping chain */
      if (--n_flipping_pages > 0) {
         tail_page = flipping_page[n_flipping_pages];

         /* If the surface attached to the bitmap is not the tail page
          * that is to be destroyed, attach it to the bitmap whose
          * attached surface is the tail page.
          */
         if (surf != tail_page) {
            surf->parent_bmp = tail_page->parent_bmp;
            surf->parent_bmp->extra = surf;
         }

         /* remove the tail page from the flipping chain */
         recreate_flipping_chain(n_flipping_pages);
         _AL_FREE(tail_page);
      }
      flipping_page[n_flipping_pages] = NULL;
   }
   else {
      /* destroy the surface */
      gfx_directx_destroy_surface(surf);
   }

   _AL_FREE(bmp);
}



/* flip_with_forefront_bitmap:
 *  Worker function for DirectDraw page flipping.
 */
static int flip_with_forefront_bitmap(BITMAP *bmp, int wait)
{
   DDRAW_SURFACE *surf;
   HRESULT hr;

   /* flip only in the foreground */
   if (!_win_app_foreground) {
      _win_thread_switch_out();
      return 0;
   }

   /* retrieve the underlying surface */
   surf = DDRAW_SURFACE_OF(bmp);
   if (surf == flipping_page[0])
      return 0;

   ASSERT((surf == flipping_page[1]) || (surf == flipping_page[2]));

   /* flip the contents of the surfaces */
   hr = IDirectDrawSurface2_Flip(flipping_page[0]->id, surf->id, wait ? DDFLIP_WAIT : 0);

   /* if the surface has been lost, try to restore all surfaces */
   if (hr == DDERR_SURFACELOST) {
      if (restore_all_ddraw_surfaces() == 0)
         hr = IDirectDrawSurface2_Flip(flipping_page[0]->id, surf->id, wait ? DDFLIP_WAIT : 0);
   }

   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't flip (%s)\n", dd_err(hr));
      return -1;
   }

   /* attach the surface to the former forefront bitmap */
   surf->parent_bmp = flipping_page[0]->parent_bmp;
   surf->parent_bmp->extra = surf;

   /* make the bitmap point to the forefront surface */
   flipping_page[0]->parent_bmp = bmp;
   bmp->extra = flipping_page[0];

   return 0;
}



/* gfx_directx_show_video_bitmap:
 */
int gfx_directx_show_video_bitmap(BITMAP *bmp)
{
   /* guard against show_video_bitmap(screen); */
   if (bmp == gfx_directx_forefront_bitmap)
      return 0;

   return flip_with_forefront_bitmap(bmp, _wait_for_vsync);
}



/* gfx_directx_request_video_bitmap:
 */
int gfx_directx_request_video_bitmap(BITMAP *bmp)
{
   /* guard against request_video_bitmap(screen); */
   if (bmp == gfx_directx_forefront_bitmap)
      return 0;

   return flip_with_forefront_bitmap(bmp, FALSE);
}



/* gfx_directx_poll_scroll:
 */
int gfx_directx_poll_scroll(void)
{
   HRESULT hr;

   ASSERT(n_flipping_pages == 3);

   hr = IDirectDrawSurface2_GetFlipStatus(flipping_page[0]->id, DDGFS_ISFLIPDONE);

   /* if the surface has been lost, try to restore all surfaces */
   if (hr == DDERR_SURFACELOST) {
      if (restore_all_ddraw_surfaces() == 0)
         hr = IDirectDrawSurface2_GetFlipStatus(flipping_page[0]->id, DDGFS_ISFLIPDONE);
   }

   if (FAILED(hr))
      return -1;

   return 0;
}



/* gfx_directx_create_system_bitmap:
 */
BITMAP *gfx_directx_create_system_bitmap(int width, int height)
{
   DDRAW_SURFACE *surf;
   BITMAP *bmp;

   /* create the DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, ddpixel_format, DDRAW_SURFACE_SYSTEM);
   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = gfx_directx_make_bitmap_from_surface(surf, width, height, BMP_ID_SYSTEM);
   if (!bmp) {
      gfx_directx_destroy_surface(surf);
      return NULL;
   }

   return bmp;
}



/* gfx_directx_destroy_system_bitmap:
 */
void gfx_directx_destroy_system_bitmap(BITMAP *bmp)
{
   /* Special case: use normal destroy_bitmap() for subbitmaps of system
    * bitmaps. Checked here rather than in destroy_bitmap() because that
    * function should not make assumptions about the relation between system
    * bitmaps and subbitmaps thereof. This duplicates code though and a
    * different solution would be better.
    */
   if (is_sub_bitmap(bmp)) {
      if (system_driver->destroy_bitmap) {
        if (system_driver->destroy_bitmap(bmp))
           return;
      }

      if (bmp->dat)
        _AL_FREE(bmp->dat);

      _AL_FREE(bmp);

      return;
   }

   /* destroy the surface */
   gfx_directx_destroy_surface(DDRAW_SURFACE_OF(bmp));

   _AL_FREE(bmp);
}



/* win_get_dc: (WinAPI)
 *  Returns device context of a video or system bitmap.
 */
HDC win_get_dc(BITMAP *bmp)
{
   LPDIRECTDRAWSURFACE2 ddsurf;
   HDC dc;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         ddsurf = DDRAW_SURFACE_OF(bmp)->id;
         hr = IDirectDrawSurface2_GetDC(ddsurf, &dc);

         /* If the surface has been lost, try to restore all surfaces
          * and, on success, try again to get the DC.
          */
         if (hr == DDERR_SURFACELOST) {
            if (restore_all_ddraw_surfaces() == 0)
               hr = IDirectDrawSurface2_GetDC(ddsurf, &dc);
         }

         if (hr == DD_OK)
            return dc;
      }
   }

   return NULL;
}



/* win_release_dc: (WinAPI)
 *  Releases device context of a video or system bitmap.
 */
void win_release_dc(BITMAP *bmp, HDC dc)
{
   LPDIRECTDRAWSURFACE2 ddsurf;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM | BMP_ID_VIDEO)) {
         ddsurf = DDRAW_SURFACE_OF(bmp)->id;
         hr = IDirectDrawSurface2_ReleaseDC(ddsurf, dc);

         /* If the surface has been lost, try to restore all surfaces
          * and, on success, try again to release the DC.
          */
         if (hr == DDERR_SURFACELOST) {
            if (restore_all_ddraw_surfaces() == 0)
               hr = IDirectDrawSurface2_ReleaseDC(ddsurf, dc);
         }
      }
   }
}
