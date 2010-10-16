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
 *      Windowed BeOS gfx driver based on BDirectWindow.
 *
 *      By Peter Wang.
 *
 *      Optimizations and bugfixes by Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif


#define BE_DRAWING_THREAD_PERIOD    16000
#define BE_DRAWING_THREAD_NAME	    "drawing thread"
#define BE_DRAWING_THREAD_PRIORITY  B_REAL_TIME_DISPLAY_PRIORITY


#define PREFIX_I                "al-bdwin INFO: "
#define PREFIX_W                "al-bdwin WARNING: "
#define PREFIX_E                "al-bdwin ERROR: "

static char driver_desc[256] = EMPTY_STRING;
static rgb_color palette_colors[256];
static unsigned char *cmap = NULL;



/* _be_gfx_direct_window_drawing_thread:
 *  This thread keeps on updating dirty rectangles on the program
 *  window.
 */
static int32 _be_gfx_direct_window_drawing_thread(void *data) 
{
   BeAllegroDirectWindow *w = (BeAllegroDirectWindow *)data;
   GRAPHICS_RECT src_gfx_rect, dest_gfx_rect;

   src_gfx_rect.pitch = w->screen_pitch;
   src_gfx_rect.height = 1;

   while (!w->dying) {
      if (w->connected && w->blitter) {
         clipping_rect *rect;
         uint32 i, j;

         acquire_sem(_be_window_lock);
         w->locker->Lock();
         for (i=0; i<w->screen_height; i++) {
            if (_be_dirty_lines[i]) {
               rect = w->rects;
               for (j=0; j<w->num_rects; j++, rect++)
                  if (((int32)i >= rect->top - w->window.top) &&
                      ((int32)i <= rect->bottom - w->window.top)) {
                     src_gfx_rect.width = rect->right - rect->left + 1;
                     src_gfx_rect.data = (void *)((unsigned long)w->screen_data + 
                        (i * w->screen_pitch) +
                        ((rect->left - w->window.left) * BYTES_PER_PIXEL(w->screen_depth)));
                     dest_gfx_rect.data = (void *)((unsigned long)w->display_data +
                        (i * w->display_pitch) +
                        (rect->left * BYTES_PER_PIXEL(w->display_depth)));
                     dest_gfx_rect.pitch = w->display_pitch;
                     w->blitter(&src_gfx_rect, &dest_gfx_rect);
                  }
               _be_dirty_lines[i] = 0;
            }
         }
         w->locker->Unlock();
      }      

      snooze(BE_DRAWING_THREAD_PERIOD);
   }  

   return B_OK;
}



/* BeAllegroDirectWindow::BeAllegroDirectWindow:
 *  Constructor, creates program window and framebuffer, and starts
 *  the drawing thread.
 */
BeAllegroDirectWindow::BeAllegroDirectWindow(BRect frame, const char *title,
   window_look look, window_feel feel, uint32 flags, uint32 workspaces,
   uint32 v_w, uint32 v_h, uint32 color_depth)
   : BDirectWindow(frame, title, look, feel, flags, workspaces)
{
   BRect rect = Bounds();
   uint32 i;

   _be_allegro_view = new BeAllegroView(rect, "Allegro",
      B_FOLLOW_ALL_SIDES, B_WILL_DRAW, BE_ALLEGRO_VIEW_DIRECT);
   rgb_color color = {0, 0, 0, 0};
   _be_allegro_view->SetViewColor(color);
   AddChild(_be_allegro_view);
   
   screen_pitch = v_w * BYTES_PER_PIXEL(color_depth);
   screen_data = malloc(v_h * screen_pitch);
   screen_depth = color_depth;
   display_depth = 0;

   num_rects = 0;
   rects     = NULL;
   
   connected = false;
   dying     = false;
   locker    = new BLocker();
   blitter   = NULL;
   
   _be_dirty_lines = (int *)malloc(v_h * sizeof(int));
   
   _be_window_lock = create_sem(0, "window lock");
      
   drawing_thread_id = spawn_thread(_be_gfx_direct_window_drawing_thread, BE_DRAWING_THREAD_NAME, 
				    BE_DRAWING_THREAD_PRIORITY, (void *)this);

   resume_thread(drawing_thread_id);
}



/* BeAllegroDirectWindow::~BeAllegroDirectWindow:
 *  Stops drawing thread and frees used memory.
 */
BeAllegroDirectWindow::~BeAllegroDirectWindow()
{
   int32 result;

   dying = true;
   release_sem(_be_window_lock);
   locker->Unlock();
   Sync();
   wait_for_thread(drawing_thread_id, &result);
   drawing_thread_id = -1;
   Hide();

   delete locker;
   delete_sem(_be_window_lock);
   _release_colorconv_blitter(blitter);
   blitter = NULL;
   connected = false;
   _be_focus_count = 0;

   if (rects) {
      free(rects);
      rects = NULL;
   }
   if (_be_dirty_lines) {
      free(_be_dirty_lines);
      _be_dirty_lines = NULL;
   }
   if (screen_data) {
      free(screen_data);
      screen_data = NULL;
   }
}



/* BeAllegroDirectWindow::MessageReceived:
 *  System messages handler.
 */
void BeAllegroDirectWindow::MessageReceived(BMessage *message)
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
         BDirectWindow::MessageReceived(message);
         break;
   }
}



/* BeAllegroDirectWindow::DirectConnected:
 *  Callback for when there are changes the program window must deal
 *  with; this includes first window exposure, system color depth
 *  changes and visible window portions updates.
 */
void BeAllegroDirectWindow::DirectConnected(direct_buffer_info *info)
{
   size_t size;
   char tmp[128];
   
   if (dying) {
      return;
   }

   locker->Lock();
   connected = false;
   
   switch (info->buffer_state & B_DIRECT_MODE_MASK) {
      case B_DIRECT_START:
	    
	 /* fallthrough */
	 
      case B_DIRECT_MODIFY: {
	 uint8 old_display_depth = display_depth;
	 uint32 i;

	 switch (info->pixel_format) {
	    case B_CMAP8:  
	       display_depth = 8;  
	       break;
	    case B_RGB15:  
	    case B_RGBA15: 
	       display_depth = 15; 
	       break;
	    case B_RGB16:  
	       display_depth = 16; 
	       break;
	    case B_RGB32:  
	    case B_RGBA32: 
	       display_depth = 32; 
	       break;
	    default:	      
	       display_depth = 0;  
	       break;
	 }
	 
	 if (!display_depth) {
	    break;  
	 }

	 if (old_display_depth != display_depth) {
	    int i;
	    
	    _release_colorconv_blitter(blitter);
	    blitter = _get_colorconv_blitter(screen_depth, display_depth);
	    if (display_depth == 8) {
	       cmap = _get_colorconv_map();
	       if (screen_depth == 8) {
	          for (i=0; i<256; i++)
	             cmap[i] = BScreen().IndexForColor(palette_colors[i]);
	       }
	       else {
	          for (i=0; i<4096; i++)
	             cmap[i] = BScreen().IndexForColor(((i >> 4) & 0xF0) | (i >> 8),  (i & 0xF0) | ((i >> 4) & 0xF),  (i & 0xF) | ((i & 0xF) << 4));
	       }
	    }
	    /* XXX commented out due to conflicting TRACE in Haiku
	    TRACE(PREFIX_I "Color conversion mode set: %d->%d\n",
		  (int)screen_depth, (int)display_depth);
	    */
	 }
	 			       
	 if (rects) {
	    free(rects);
	 }

	 num_rects = info->clip_list_count;
	 size = num_rects * (sizeof *rects);
	 rects = (clipping_rect *)malloc(num_rects * size);
	 
	 if (rects) {
	    memcpy(rects, info->clip_list, size);
	 }

	 window = info->window_bounds;
	 screen_height = window.bottom - window.top + 1;
	 display_pitch = info->bytes_per_row;
	 display_data = (void *)((unsigned long)info->bits + 
	    (window.top * display_pitch));
     
	 for (i=0; i<screen_height; i++) {
	    _be_dirty_lines[i] = 1;
	 }

	 connected = true;
	 _be_gfx_initialized = true;
	 break;
      }

      case B_DIRECT_STOP:
	 break;
   }
   
   if (screen_depth == 8)
      be_gfx_bdirectwindow_set_palette(_current_palette, 0, 255, 0);

   uszprintf(driver_desc, sizeof(driver_desc), get_config_text("BDirectWindow, %d bit in %s"),
             screen_depth, uconvert_ascii(screen_depth == display_depth ? "matching" : "fast emulation", tmp));

   release_sem(_be_window_lock);
   locker->Unlock();
}



/* BeAllegroDirectWindow::WindowActivated:
 *  Callback for when the window gains/looses focus.
 */
void BeAllegroDirectWindow::WindowActivated(bool active)
{
   _be_change_focus(active);
   BDirectWindow::WindowActivated(active);
}



/* BeAllegroDirectWindow::QuitRequested:
 *  User requested to close the program window.
 */
bool BeAllegroDirectWindow::QuitRequested(void)
{
    return _be_handle_window_close(Title());
}



/* _be_gfx_bdirectwindow_init:
 *  Initializes specified video mode.
 */
static struct BITMAP *_be_gfx_bdirectwindow_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   int bpp;
   char tmp[128];

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

   if ((!v_w) && (!v_h)) {
      v_w = w;
      v_h = h;
   }

   if ((w != v_w) || (h != v_h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported virtual resolution"));
      return NULL;
   }
   
   set_display_switch_mode(SWITCH_PAUSE);

   _be_allegro_direct_window = new BeAllegroDirectWindow(BRect(0, 0, w-1, h-1), wnd_title,
			      B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			      B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
			      B_CURRENT_WORKSPACE, v_w, v_h, color_depth);
   _be_window = _be_allegro_direct_window;

   if (!_be_allegro_direct_window->SupportsWindowMode()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Windowed mode not supported"));
      goto cleanup;
   }

   _be_mouse_view = new BView(_be_allegro_direct_window->Bounds(),
			     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   _be_allegro_direct_window->Lock();
   _be_allegro_direct_window->AddChild(_be_mouse_view);
   _be_allegro_direct_window->Unlock();

   _be_mouse_window = _be_allegro_direct_window;
   _be_mouse_window_mode = true;

   release_sem(_be_mouse_view_attached);

   _be_gfx_initialized = false;
   
   _be_allegro_direct_window->MoveTo(6, 25);
   _be_allegro_direct_window->Show();

   bpp = (color_depth + 7) / 8;

   drv->w       = w;
   drv->h       = h;
   drv->linear	= TRUE;
   drv->vid_mem	= bpp * v_w * v_h;
   
   bmp = _make_bitmap(v_w, v_h, (unsigned long)_be_allegro_direct_window->screen_data, drv, 
		      color_depth, v_w * bpp);
  
   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }

#ifdef ALLEGRO_NO_ASM
   bmp->read_bank = _be_gfx_bwindow_read_write_bank;
   bmp->write_bank = _be_gfx_bwindow_read_write_bank;
   _screen_vtable.unwrite_bank = _be_gfx_bwindow_unwrite_bank;
#else
   bmp->read_bank = _be_gfx_bwindow_read_write_bank_asm;
   bmp->write_bank = _be_gfx_bwindow_read_write_bank_asm;
   _screen_vtable.unwrite_bank = _be_gfx_bwindow_unwrite_bank_asm;
#endif

   _screen_vtable.acquire = be_gfx_bdirectwindow_acquire;
   _screen_vtable.release = be_gfx_bdirectwindow_release;

   _be_gfx_set_truecolor_shifts();

   /* Give time to connect the screen and do the first update */
   snooze(50000);
   while (!_be_gfx_initialized);

   uszprintf(driver_desc, sizeof(driver_desc), get_config_text("BDirectWindow object, %d bit in %s"),
             _be_allegro_direct_window->screen_depth,
             uconvert_ascii(_be_allegro_direct_window->screen_depth == _be_allegro_direct_window->display_depth ?
                            "matching" : "fast emulation", tmp));
   drv->desc = driver_desc;
   
   return bmp;

   cleanup: {
      be_gfx_bdirectwindow_exit(NULL);
      return NULL;
   }
}



extern "C" struct BITMAP *be_gfx_bdirectwindow_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return _be_gfx_bdirectwindow_init(&gfx_beos_bdirectwindow, w, h, v_w, v_h, color_depth);
}



/* be_gfx_bdirectwindow_exit:
 *  Shuts down driver.
 */
extern "C" void be_gfx_bdirectwindow_exit(struct BITMAP *bmp)
{
   (void)bmp;

   _be_gfx_initialized = false;

   if (_be_allegro_direct_window) {
      if (_be_mouse_view_attached < 1) {
	 acquire_sem(_be_mouse_view_attached);
      }

      _be_allegro_direct_window->Lock();
      _be_allegro_direct_window->Quit();

      _be_allegro_direct_window = NULL;
      _be_window = NULL;
   }
   _be_mouse_window   = NULL;	    
   _be_mouse_view     = NULL;
}



/* be_gfx_bdirectwindow_acquire:
 *  Locks the screen bitmap.
 */
extern "C" void be_gfx_bdirectwindow_acquire(struct BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      _be_allegro_direct_window->locker->Lock();
      bmp->id |= BMP_ID_LOCKED;
   }
}



/* be_gfx_bdirectwindow_release:
 *  Unlocks the screen bitmap.
 */
extern "C" void be_gfx_bdirectwindow_release(struct BITMAP *bmp)
{
   if (bmp->id & BMP_ID_LOCKED) {
      bmp->id &= ~BMP_ID_LOCKED;
      _be_allegro_direct_window->locker->Unlock();
      release_sem(_be_window_lock);
   }
}



/* be_gfx_bdirectwindow_set_palette:
 *  Changes the internal palette and warns the drawing thread to
 *  reflect the change.
 */
extern "C" void be_gfx_bdirectwindow_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;

   if (!_be_allegro_direct_window->blitter)
      return;
   
   if (vsync) {
      be_gfx_vsync();
   }
   
   _be_allegro_direct_window->locker->Lock();
   _set_colorconv_palette(p, from, to);
   if ((_be_allegro_direct_window->display_depth == 8) &&
       (_be_allegro_direct_window->screen_depth == 8)) {
      for (i=from; i<=to; i++) {
         palette_colors[i].red   = _rgb_scale_6[p[i].r];
         palette_colors[i].green = _rgb_scale_6[p[i].g];
         palette_colors[i].blue  = _rgb_scale_6[p[i].b];
      }
      for (i=0; i<256; i++)
         cmap[i] = BScreen().IndexForColor(palette_colors[i]);
   }
   for (i=0; i<(int)_be_allegro_direct_window->screen_height; i++)
      _be_dirty_lines[i] = 1;
   release_sem(_be_window_lock);

   _be_allegro_direct_window->locker->Unlock();
}

