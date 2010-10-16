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
 *      Fullscreen BeOS gfx driver based on BScreen.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif


static char driver_desc[256] = EMPTY_STRING;
static thread_id palette_thread_id = -1;
static sem_id palette_sem = -1;
static rgb_color palette_colors[256];



/* BeAllegroScreen::BeAllegroScreen:
 *  Constructor, switches gfx mode and initializes the screen.
 */
BeAllegroScreen::BeAllegroScreen(const char *title, uint32 space, status_t *error, bool debugging)
   : BWindowScreen(title, space, error, debugging)
{
}



/* BeAllegroScreen::ScreenConnected:
 *  Callback for when the user switches in or out of the workspace.
 */
void BeAllegroScreen::ScreenConnected(bool connected)
{
   if(connected) {
      _be_change_focus(connected);      
      release_sem(_be_fullscreen_lock);
   }
   else {
      acquire_sem(_be_fullscreen_lock);
      _be_change_focus(connected);
   }
}



/* BeAllegroScreen::QuitRequested:
 *  User requested to close the program window.
 */
bool BeAllegroScreen::QuitRequested(void)
{
   return _be_handle_window_close(Title());
}



/* BeAllegroScreen::MessageReceived:
 *  System messages handler.
 */
void BeAllegroScreen::MessageReceived(BMessage *message)
{
   switch (message->what) {
      case B_SIMPLE_DATA:
         break;
      
      case B_MOUSE_WHEEL_CHANGED:
         float dy; 
         message->FindFloat("be:wheel_delta_y", &dy);
         _be_mouse_z += ((int)dy > 0 ? -1 : 1);
         break;

      default:
         BWindowScreen::MessageReceived(message);
         break;
   }
}



/* find_gfx_mode:
 *  Searches given mode and returns the corresponding entry in the modes
 *  table. Returns -1 if mode is unavailable.
 */
static inline uint32 find_gfx_mode(int w, int h, int d)
{
   int index = 0;
   while (_be_mode_table[index].d > 0) {
      if ((_be_mode_table[index].w == w) &&
          (_be_mode_table[index].h == h) &&
          (_be_mode_table[index].d == d))
         return _be_mode_table[index].mode;
      index++;
   }

   return (uint32)-1;
}



/* be_sort_out_virtual_resolution:
 *  Computes and initializes framebuffer layout for given virtual
 *  resolution.
 */
static inline bool be_sort_out_virtual_resolution(int w, int h, int *v_w, int *v_h, int color_depth)
{
   int32 try_v_w;
   int32 try_v_h;
   // Possible VRAM amounts. Are these always powers of 2?
   int ram_count[] = { 256, 128, 64, 32, 16, 8, 4, 2, 1, 0 };
   int i;

   if (*v_w == 0)
      try_v_w = MIN(w, 32767);
   else
      try_v_w = MIN(*v_w, 32767);
   try_v_h = *v_h;

   if (*v_h == 0) {
      for (i = 0; ram_count[i]; i++) {
	 try_v_h = (1024 * 1024 * ram_count[i]) / (try_v_w * BYTES_PER_PIXEL(color_depth));
	 /* Evil hack: under BeOS R5 SetFrameBuffer() should work with any
	  * int32 width and height parameters, but actually it crashes if
	  * one of them exceeds the boundaries of a signed short variable.
	  */
	 try_v_h = MIN(try_v_h, 32767);

	 if (_be_allegro_screen->SetFrameBuffer(try_v_w, try_v_h) == B_OK) {
	    *v_w = try_v_w;
	    *v_h = try_v_h;
	    return true;
         }
      }
      try_v_h = h;
   }

   if (_be_allegro_screen->SetFrameBuffer(try_v_w, try_v_h) == B_OK) {
      *v_w = try_v_w;
      *v_h = try_v_h;
      return true;
   }
   else {
      return false;
   }
}



/* palette_updater_thread:
 *  This small thread is used to update the palette in fullscreen mode. It may seem
 *  unnecessary as a direct call to SetColorList would do it, but calling directly
 *  this function from the main thread has proven to be a major bottleneck, so
 *  calling it from a separated thread is a good thing.
 */
static int32 palette_updater_thread(void *data) 
{
   BeAllegroScreen *s = (BeAllegroScreen *)data;
   
   while (_be_gfx_initialized) {
      acquire_sem(palette_sem);
      s->SetColorList(palette_colors, 0, 255);
   }
   return B_OK;
}



/* be_gfx_bwindowscreen_fetch_mode_list:
 *  Builds the list of available video modes.
 */
extern "C" struct GFX_MODE_LIST *be_gfx_bwindowscreen_fetch_mode_list(void)
{
   int j, be_mode, num_modes = 0;
   uint32 i, count;
   display_mode *mode;
   GFX_MODE_LIST *mode_list;
   bool already_there;
   
   if (BScreen().GetModeList(&mode, &count) != B_OK)
      return NULL;
   
   mode_list = (GFX_MODE_LIST *)malloc(sizeof(GFX_MODE_LIST));
   if (!mode_list) {
      free(mode);
      return NULL;
   }
   mode_list->mode = NULL;
   
   for (i=0; i<count; i++) {
      be_mode = 0;
      while (_be_mode_table[be_mode].d > 0) {
         if ((mode[i].virtual_width == _be_mode_table[be_mode].w) &&
             (mode[i].virtual_height == _be_mode_table[be_mode].h) &&
             (mode[i].space == _be_mode_table[be_mode].space))
            break;
         be_mode++;
      }
      if (_be_mode_table[be_mode].d == -1)
         continue;

      already_there = false;
      for (j=0; j<num_modes; j++) {
         if ((mode_list->mode[j].width == _be_mode_table[be_mode].w) &&
             (mode_list->mode[j].height == _be_mode_table[be_mode].h) &&
             (mode_list->mode[j].bpp == _be_mode_table[be_mode].d)) {
            already_there = true;
            break;
         }
      }
      if (!already_there) {
         mode_list->mode = (GFX_MODE *)realloc(mode_list->mode, sizeof(GFX_MODE) * (num_modes + 1));
         if (!mode_list->mode) {
            free(mode);
            return NULL;
         }
         mode_list->mode[num_modes].width = _be_mode_table[be_mode].w;
         mode_list->mode[num_modes].height = _be_mode_table[be_mode].h;
         mode_list->mode[num_modes].bpp = _be_mode_table[be_mode].d;
         num_modes++;
      }
   }
   mode_list->mode = (GFX_MODE *)realloc(mode_list->mode, sizeof(GFX_MODE) * (num_modes + 1));
   if (!mode_list->mode) {
      free(mode);
      return NULL;
   }
   mode_list->mode[num_modes].width = 0;
   mode_list->mode[num_modes].height = 0;
   mode_list->mode[num_modes].bpp = 0;
   mode_list->num_modes = num_modes;
   
   free(mode);
   
   return mode_list;
}



/* _be_gfx_bwindowscreen_init:
 *  Initializes the driver for given gfx mode.
 */
static struct BITMAP *_be_gfx_bwindowscreen_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth, bool accel)
{
   BITMAP             *bmp;
   status_t            error;
   uint32              mode;
   graphics_card_info *gfx_card;
   frame_buffer_info  *fbuffer;
   accelerant_device_info info;
   char tmp1[128], tmp2[128];

   if (1
#ifdef ALLEGRO_COLOR8
       && (color_depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (color_depth != 15)
       && (color_depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (color_depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (color_depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return NULL;
   }

   if ((w == 0) && (h == 0)) {
      w = 640;
      h = 480;
   }
   mode = find_gfx_mode(w, h, color_depth);

   if (mode == (uint32)-1) {
      goto cleanup;
   }

   _be_fullscreen_lock = create_sem(0, "screen lock");

   if (_be_fullscreen_lock < 0) {
      goto cleanup;
   }
   
   _be_lock_count = 0;

   set_display_switch_mode(SWITCH_AMNESIA);
   
   _be_allegro_screen = new BeAllegroScreen(wnd_title, mode, &error, false);
   _be_window = _be_allegro_screen;

   if(error != B_OK) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto cleanup;
   }

   _be_mouse_view = new BView(_be_allegro_screen->Bounds(),
     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   _be_allegro_screen->Lock();
   _be_allegro_screen->AddChild(_be_mouse_view);
   _be_allegro_screen->Unlock();

   _be_mouse_window = _be_allegro_screen;
   _be_mouse_window_mode = false;
   _mouse_on = TRUE;

   release_sem(_be_mouse_view_attached);

   palette_sem = create_sem(0, "palette sem");
   if (palette_sem < 0) {
      goto cleanup;
   }
      
   _be_gfx_initialized = false;
   _be_allegro_screen->Show();
   acquire_sem(_be_fullscreen_lock);

   gfx_card = _be_allegro_screen->CardInfo();

   if (!be_sort_out_virtual_resolution(w, h, &v_w, &v_h, color_depth)) {
      v_w = w;
      v_h = h;
   }
   
   /* BWindowScreen sets refresh rate at 60 Hz by default */
   _set_current_refresh_rate(60);

   fbuffer = _be_allegro_screen->FrameBufferInfo();

   drv->w       = w;
   drv->h       = h;
   drv->linear  = TRUE;
   drv->vid_mem = v_w * v_h * BYTES_PER_PIXEL(color_depth);
   
   bmp = _make_bitmap(fbuffer->width, fbuffer->height,
            (unsigned long)gfx_card->frame_buffer, drv,
            color_depth, fbuffer->bytes_per_row);

   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }

   _be_sync_func = NULL;
   if (accel) {
      be_gfx_bwindowscreen_accelerate(color_depth);
   }

#ifdef ALLEGRO_NO_ASM
   if (gfx_capabilities) {
      bmp->write_bank = be_gfx_bwindowscreen_read_write_bank;
      bmp->read_bank  = be_gfx_bwindowscreen_read_write_bank;
      _screen_vtable.unwrite_bank = be_gfx_bwindowscreen_unwrite_bank;
   }
#else
   if (gfx_capabilities) {
      bmp->write_bank = _be_gfx_bwindowscreen_read_write_bank_asm;
      bmp->read_bank  = _be_gfx_bwindowscreen_read_write_bank_asm;
      _screen_vtable.unwrite_bank = _be_gfx_bwindowscreen_unwrite_bank_asm;
   }
#endif
   gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;

   _screen_vtable.acquire      = be_gfx_bwindowscreen_acquire;
   _screen_vtable.release      = be_gfx_bwindowscreen_release;

   _be_gfx_set_truecolor_shifts();
   if (BScreen().GetDeviceInfo(&info) == B_OK)
      uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("BWindowScreen object (%s)", tmp1),
                uconvert_ascii(info.name, tmp2));
   else
      ustrzcpy(driver_desc, sizeof(driver_desc), uconvert_ascii("BWindowScreen object", tmp1));
   drv->desc = driver_desc;

   _be_gfx_initialized = true;
   
   palette_thread_id = spawn_thread(palette_updater_thread, "palette updater", 
				    B_DISPLAY_PRIORITY, (void *)_be_allegro_screen);
   resume_thread(palette_thread_id);

   be_app->HideCursor();
   
   release_sem(_be_fullscreen_lock);
   while (!_be_focus_count);

   return bmp;

   cleanup: {
      be_gfx_bwindowscreen_exit(NULL);
      return NULL;
   }
}



extern "C" struct BITMAP *be_gfx_bwindowscreen_accel_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return _be_gfx_bwindowscreen_init(&gfx_beos_bwindowscreen_accel, w, h, v_w, v_h, color_depth, true);
}



extern "C" struct BITMAP *be_gfx_bwindowscreen_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return _be_gfx_bwindowscreen_init(&gfx_beos_bwindowscreen, w, h, v_w, v_h, color_depth, false);
}



/* be_gfx_bwindowscreen_exit:
 *  Shuts down the driver.
 */
extern "C" void be_gfx_bwindowscreen_exit(struct BITMAP *bmp)
{
   status_t ret_value;
   (void)bmp;

   if (_be_fullscreen_lock > 0) {
      delete_sem(_be_fullscreen_lock);
      _be_fullscreen_lock = -1;
   }
   _be_gfx_initialized = false;
   
   if (palette_thread_id >= 0) {
      release_sem(palette_sem);
      wait_for_thread(palette_thread_id, &ret_value);
      palette_thread_id = -1;
   }
   if (palette_sem >= 0) {
      delete_sem(palette_sem);
      palette_sem = -1;
   }
   
   if (_be_allegro_screen != NULL) {
      if (_be_mouse_view_attached < 1) {
	 acquire_sem(_be_mouse_view_attached);
      }

      _be_allegro_screen->Lock();
      _be_allegro_screen->Quit();

      _be_allegro_screen = NULL;
      _be_window = NULL;
   }
   
   be_app->ShowCursor();

   _be_mouse_window   = NULL;
   _be_mouse_view     = NULL;

   _be_focus_count = 0;
   _be_lock_count = 0;
}



#ifdef ALLEGRO_NO_ASM

/* be_gfx_bwindowscreen_read_write_bank:
 *  Returns new line and synchronizes framebuffer if needed.
 */
extern "C" uintptr_t be_gfx_bwindowscreen_read_write_bank(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      _be_sync_func();
      bmp->id |= (BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
   return (unsigned long)(bmp->line[line]);
}



/* be_gfx_bwindowscreen_unwrite_bank:
 *  Unlocks bitmap if necessary.
 */
extern "C" void be_gfx_bwindowscreen_unwrite_bank(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
}

#endif



/* be_gfx_bwindowscreen_acquire:
 *  Locks specified video bitmap.
 */
extern "C" void be_gfx_bwindowscreen_acquire(struct BITMAP *bmp)
{
   if (_be_lock_count == 0) {
      acquire_sem(_be_fullscreen_lock);
      bmp->id |= BMP_ID_LOCKED;
   }
   if (_be_sync_func)
      _be_sync_func();
   _be_lock_count++;
}



/* be_gfx_bwindowscreen_release:
 *  Unlocks specified video bitmap.
 */
extern "C" void be_gfx_bwindowscreen_release(struct BITMAP *bmp)
{
   _be_lock_count--;

   if (_be_lock_count == 0) {
      bmp->id &= ~BMP_ID_LOCKED;
      release_sem(_be_fullscreen_lock);
   }
}



/* be_gfx_bwindowscreen_set_palette:
 *  Sets the palette colors and notices the palette updater thread
 *  about the change.
 */
extern "C" void be_gfx_bwindowscreen_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   if (vsync)
      be_gfx_vsync();

   for(int index = from; index <= to; index++) {
      palette_colors[index].red   = _rgb_scale_6[p[index].r];
      palette_colors[index].green = _rgb_scale_6[p[index].g];
      palette_colors[index].blue  = _rgb_scale_6[p[index].b];
      palette_colors[index].alpha = 255;
   }
   /* Update palette! */
   release_sem(palette_sem);
}



/* be_gfx_bwindowscreen_scroll:
 *  Scrolls the visible viewport.
 */
extern "C" int be_gfx_bwindowscreen_scroll(int x, int y)
{
   int rv;
   
   acquire_screen();
   
   if (_be_allegro_screen->MoveDisplayArea(x, y) != B_ERROR) {
      rv = 0;
   }
   else {
      rv = 1;
   }

   release_screen();
   
   if (_wait_for_vsync)
      be_gfx_vsync();   

   return rv;
}



/* be_gfx_bwindowscreen_poll_scroll:
 *  Returns true if there are pending scrolling requests left.
 */
extern "C" int be_gfx_bwindowscreen_poll_scroll(void)
{
   return (BScreen(_be_allegro_screen).WaitForRetrace(0) == B_ERROR ? TRUE : FALSE);
}



/* be_gfx_bwindowscreen_request_scroll:
 *  Starts a screen scroll but doesn't wait for the retrace.
 */
extern "C" int be_gfx_bwindowscreen_request_scroll(int x, int y)
{
   acquire_screen();
   _be_allegro_screen->MoveDisplayArea(x, y);
   release_screen();
   
   return 0;
}



/* be_gfx_bwindowscreen_request_video_bitmap:
 *  Page flips to display specified bitmap, but doesn't wait for retrace.
 */
extern "C" int be_gfx_bwindowscreen_request_video_bitmap(struct BITMAP *bmp)
{
   int rv;
   
   acquire_screen();
   rv = _be_allegro_screen->MoveDisplayArea(bmp->x_ofs, bmp->y_ofs) == B_ERROR ? 1 : 0;
   release_screen();

   return rv;
}

