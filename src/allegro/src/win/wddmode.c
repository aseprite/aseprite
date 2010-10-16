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
 *      DirectDraw video mode setting.
 *
 *      By Stefan Schimanski.
 *
 *      Unified setup code and refresh rate support by Eric Botcazou.
 *
 *      Graphics mode list fetching code by Henrik Stokseth.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#define PREFIX_I                "al-wddmode INFO: "
#define PREFIX_W                "al-wddmode WARNING: "
#define PREFIX_E                "al-wddmode ERROR: "


int _win_desktop_depth;
RGB_MAP _win_desktop_rgb_map;  /* for 8-bit desktops */


int mode_supported;


static int pixel_realdepth[] = {8, 15, 15, 16, 16, 24, 24, 32, 32, 0};

static DDPIXELFORMAT pixel_format[] = {
   /* 8-bit */
   {sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0, {8}, {0}, {0}, {0}, {0}},
   /* 16-bit RGB 5:5:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x7C00}, {0x03e0}, {0x001F}, {0}},
   /* 16-bit BGR 5:5:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x001F}, {0x03e0}, {0x7C00}, {0}},
   /* 16-bit RGB 5:6:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0xF800}, {0x07e0}, {0x001F}, {0}},
   /* 16-bit BGR 5:6:5 */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x001F}, {0x07e0}, {0xF800}, {0}},
   /* 24-bit RGB */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {24}, {0xFF0000}, {0x00FF00}, {0x0000FF}, {0}},
   /* 24-bit BGR */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {24}, {0x0000FF}, {0x00FF00}, {0xFF0000}, {0}},
   /* 32-bit RGB */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {32}, {0xFF0000}, {0x00FF00}, {0x0000FF}, {0}},
   /* 32-bit BGR */
   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {32}, {0x0000FF}, {0x00FF00}, {0xFF0000}, {0}} 
};


/* window thread callback parameters */
static int _wnd_width, _wnd_height, _wnd_depth, _wnd_refresh_rate, _wnd_flags;



/* wnd_set_video_mode:
 *  Called by window thread to set a gfx mode; this is needed because DirectDraw can only
 *  change the mode in the thread that handles the window.
 */
static int wnd_set_video_mode(void)
{
   HRESULT hr;
   HWND allegro_wnd = win_get_window();

   /* set the cooperative level to allow fullscreen access */
   hr = IDirectDraw2_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "SetCooperativeLevel() failed (%x)\n", hr);
      return -1;
   }

   /* switch to fullscreen mode */
   hr = IDirectDraw2_SetDisplayMode(directdraw, _wnd_width, _wnd_height, _wnd_depth,
                                                _wnd_refresh_rate, _wnd_flags);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "SetDisplayMode(%u, %u, %u, %u, %u) failed (%x)\n", _wnd_width, _wnd_height, _wnd_depth,
                                                                 _wnd_refresh_rate, _wnd_flags, hr);
      return -1;
   }

   return 0;
}



/* shift_gamma:
 *  Helper function for changing a 8-bit gamma value into
 *  a fake 6-bit gamma value (shifted 5-bit gamma value).
 */
static INLINE int shift_gamma(int gamma)
{
   if (gamma<128)
      return (gamma+7)>>2;
   else
      return (gamma-7)>>2;
}



/* build_desktop_rgb_map:
 *  Builds the RGB map corresponding to the desktop palette.
 */
void build_desktop_rgb_map(void)
{
   PALETTE pal;
   PALETTEENTRY system_palette[PAL_SIZE];
   HDC dc;
   int i;

   /* retrieve Windows system palette */
   dc = GetDC(NULL);
   GetSystemPaletteEntries(dc, 0, PAL_SIZE, system_palette);
   ReleaseDC(NULL, dc);

   for (i=0; i<PAL_SIZE; i++) {
      pal[i].r = shift_gamma(system_palette[i].peRed);
      pal[i].g = shift_gamma(system_palette[i].peGreen);
      pal[i].b = shift_gamma(system_palette[i].peBlue);
   }

   create_rgb_table(&_win_desktop_rgb_map, pal, NULL);

   /* create_rgb_table() never maps RGB triplets to index 0 */
   _win_desktop_rgb_map.data[pal[0].r>>1][pal[0].g>>1][pal[0].b>>1] = 0;
}



/* get_color_shift:
 *  Returns shift value for color mask.
 */
static int get_color_shift(int mask)
{
   int n;

   for (n = 0; ((mask & 1) == 0) && (mask != 0); n++)
      mask >>= 1;

   return n;
}



/* get_color_bits:
 *  Returns used bits in color mask.
 */
static int get_color_bits(int mask)
{
   int n;
   int num = 0;

   for (n = 0; n < 32; n++) {
      num += (mask & 1);
      mask >>= 1;
   }

   return num;
}



/* gfx_directx_compare_color_depth:
 *  Compares the requested color depth with the desktop depth and find
 *  the best pixel format for color conversion if they don't match.
 *  Returns 0 if the depths match, -1 if they don't.
 */ 
int gfx_directx_compare_color_depth(int color_depth)
{
   DDSURFACEDESC surf_desc;
   HRESULT hr;
   int i;

   /* get current video mode */
   surf_desc.dwSize = sizeof(surf_desc);
   hr = IDirectDraw2_GetDisplayMode(directdraw, &surf_desc);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't get color format.\n");
      return -1;
   }

   /* get the *real* color depth of the desktop */
   _win_desktop_depth = surf_desc.ddpfPixelFormat.dwRGBBitCount;
   if (_win_desktop_depth == 16) /* sure? */
      _win_desktop_depth = get_color_bits(surf_desc.ddpfPixelFormat.dwRBitMask) +
                      get_color_bits(surf_desc.ddpfPixelFormat.dwGBitMask) +
                      get_color_bits(surf_desc.ddpfPixelFormat.dwBBitMask);

   if (color_depth == _win_desktop_depth) {
      ddpixel_format = NULL;
      return 0;
   }
   else {
      /* test for the same depth and RGB order */
      for (i=0 ; pixel_realdepth[i] ; i++) {
         if ((pixel_realdepth[i] == color_depth) &&
            ((surf_desc.ddpfPixelFormat.dwRBitMask & pixel_format[i].dwRBitMask) ||
                (surf_desc.ddpfPixelFormat.dwBBitMask & pixel_format[i].dwBBitMask) ||
                   (_win_desktop_depth == 8) || (color_depth == 8))) {
                      ddpixel_format = &pixel_format[i];
                      break;
         }
      }

      return -1;
   }
}



/* gfx_directx_update_color_format:
 *  Sets the _rgb* variables for correct color format.
 */
int gfx_directx_update_color_format(DDRAW_SURFACE *surf, int color_depth)
{
   DDPIXELFORMAT pixel_format;
   HRESULT hr;
   int shift_r, shift_g, shift_b;

   /* get pixel format */
   pixel_format.dwSize = sizeof(DDPIXELFORMAT);
   hr = IDirectDrawSurface2_GetPixelFormat(surf->id, &pixel_format);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't get color format.\n");
      return -1;
   }

   /* pass color format */
   shift_r = get_color_shift(pixel_format.dwRBitMask);
   shift_g = get_color_shift(pixel_format.dwGBitMask);
   shift_b = get_color_shift(pixel_format.dwBBitMask);

   /* set correct shift values */
   switch (color_depth) {
      case 15:
         _rgb_r_shift_15 = shift_r;
         _rgb_g_shift_15 = shift_g;
         _rgb_b_shift_15 = shift_b;
         break;

      case 16:
         _rgb_r_shift_16 = shift_r;
         _rgb_g_shift_16 = shift_g;
         _rgb_b_shift_16 = shift_b;
         break;

      case 24:
         _rgb_r_shift_24 = shift_r;
         _rgb_g_shift_24 = shift_g;
         _rgb_b_shift_24 = shift_b;
         break;

      case 32:
         _rgb_r_shift_32 = shift_r;
         _rgb_g_shift_32 = shift_g;
         _rgb_b_shift_32 = shift_b;
         break;
   }

   return 0;
}



/* EnumModesCallback:
 *  Callback function for enumerating the graphics modes.
 */ 
static HRESULT CALLBACK EnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID gfx_mode_list_ptr)
{
   GFX_MODE_LIST *gfx_mode_list = (GFX_MODE_LIST *)gfx_mode_list_ptr;

   /* build gfx mode-list */
   gfx_mode_list->mode = _al_sane_realloc(gfx_mode_list->mode, sizeof(GFX_MODE) * (gfx_mode_list->num_modes + 1));
   if (!gfx_mode_list->mode)
      return DDENUMRET_CANCEL;

   gfx_mode_list->mode[gfx_mode_list->num_modes].width  = lpDDSurfaceDesc->dwWidth;
   gfx_mode_list->mode[gfx_mode_list->num_modes].height = lpDDSurfaceDesc->dwHeight;
   gfx_mode_list->mode[gfx_mode_list->num_modes].bpp    = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

   /* check if 16 bpp mode is 16 bpp or 15 bpp */
   if (gfx_mode_list->mode[gfx_mode_list->num_modes].bpp == 16) {
      gfx_mode_list->mode[gfx_mode_list->num_modes].bpp = 
         get_color_bits(lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask) +
         get_color_bits(lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask) +
         get_color_bits(lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask);
   }

   gfx_mode_list->num_modes++;

   return DDENUMRET_OK;
}



/* gfx_directx_fetch_mode_list:
 *  Creates a list of available video modes.
 *  Returns the list on success, NULL on failure.
 */
GFX_MODE_LIST *gfx_directx_fetch_mode_list(void)
{
   GFX_MODE_LIST *gfx_mode_list;
   int enum_flags, dx_was_off;
   HRESULT hr;

   /* enumerate VGA Mode 13h under DirectX 5 or greater */
   if (_dx_ver >= 0x500)
      enum_flags = DDEDM_STANDARDVGAMODES;
   else
      enum_flags = 0;

   if (!directdraw) {
      init_directx();
      dx_was_off = TRUE;
   }
   else {
      dx_was_off = FALSE;
   }

   /* start enumeration */
   gfx_mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!gfx_mode_list)
      goto Error;

   gfx_mode_list->num_modes = 0;
   gfx_mode_list->mode = NULL;

   hr = IDirectDraw2_EnumDisplayModes(directdraw, enum_flags, NULL, gfx_mode_list, EnumModesCallback);
   if (FAILED(hr)) {
      _AL_FREE(gfx_mode_list);
      goto Error;
   }

   /* terminate mode list */
   gfx_mode_list->mode = _al_sane_realloc(gfx_mode_list->mode, sizeof(GFX_MODE) * (gfx_mode_list->num_modes + 1));
   if (!gfx_mode_list->mode)
      goto Error;

   gfx_mode_list->mode[gfx_mode_list->num_modes].width  = 0;
   gfx_mode_list->mode[gfx_mode_list->num_modes].height = 0;
   gfx_mode_list->mode[gfx_mode_list->num_modes].bpp    = 0;

   if (dx_was_off)
      exit_directx();

   return gfx_mode_list;

 Error:
   if (dx_was_off)
      exit_directx();

   return NULL;
}



/* set_video_mode:
 *  Sets the requested fullscreen video mode.
 */
int set_video_mode(int w, int h, int v_w, int v_h, int color_depth)
{
   HWND allegro_wnd = win_get_window();
   _wnd_width = w;
   _wnd_height = h;
   _wnd_depth = (color_depth == 15 ? 16 : color_depth);
   _wnd_refresh_rate = _refresh_rate_request;

   /* use VGA Mode 13h under DirectX 5 or greater */
   if ((w == 320) && (h == 200) && (color_depth == 8) && (_dx_ver >= 0x500))
      _wnd_flags = DDSDM_STANDARDVGAMODE;
   else
      _wnd_flags = 0;

   while (TRUE) {
      /* let the window thread do the hard work */ 
      _TRACE(PREFIX_I "setting display mode(%u, %u, %u, %u)\n",
             _wnd_width, _wnd_height, _wnd_depth, _wnd_refresh_rate);

      if (wnd_call_proc(wnd_set_video_mode) == 0) {
         /* check that the requested mode was properly set by the driver */
         if (gfx_directx_compare_color_depth(color_depth) == 0)
            goto Done;
      }

      /* try with no refresh rate request */
      if (_wnd_refresh_rate > 0)
         _wnd_refresh_rate = 0;
      else
         break;
   }
      
   _TRACE(PREFIX_E "Unable to set any display mode.\n");
   return -1;

 Done:
   /* remove the window controls */
   SetWindowLong(allegro_wnd, GWL_STYLE, WS_POPUP);

   /* maximize the window */
   ShowWindow(allegro_wnd, SW_MAXIMIZE);

   /* put it in the foreground */
   SetForegroundWindow(allegro_wnd);

   return 0;
}
