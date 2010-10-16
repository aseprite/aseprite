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
 *      BeOS BWindow/BBitmap windowed driver.
 *
 *      By Angelo Mottola.
 *      
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif

#define UPDATER_PERIOD      16000


static unsigned char cmap[256];
static char driver_desc[256] = EMPTY_STRING;



/* window_updater:
 *  Thread doing the dirty work: keeps updating dirty lines on the window.
 */
static int32 window_updater(void *data)
{
   BeAllegroWindow *w = (BeAllegroWindow *)data;
   BRect bounds, update_rect;
   int line, start, end, h, i, j;
   unsigned char *src, *dest;
   
   bounds = w->Bounds();
   
   h = (int)bounds.bottom + 1;
   update_rect.left = 0;
   update_rect.right = bounds.right;
   
   while (!w->dying) {
      acquire_sem(_be_window_lock);
      w->Lock();
      line = 0;
      while (line < h) {
         while ((line < h) && (!_be_dirty_lines[line]))
            line++;
         if (line >= h)
            break;
         start = line;
         while ((line < h) && (_be_dirty_lines[line])) {
            _be_dirty_lines[line] = 0;
            line++;
         }
         update_rect.top = start;
         update_rect.bottom = line - 1;
         if (_be_allegro_window->screen_depth == 8) {
            src = (uint8 *)w->buffer->Bits() + (start * w->buffer->BytesPerRow());
            dest = (uint8 *)w->aux_buffer->Bits() + (start * w->aux_buffer->BytesPerRow());
            for (i=start; i<line; i++) {
               for (j=0; j<_be_allegro_window->screen_width; j++)
                  dest[j] = cmap[src[j]];
               src += w->buffer->BytesPerRow();
               dest += w->aux_buffer->BytesPerRow();
            }
            _be_allegro_view->DrawBitmapAsync(w->aux_buffer, update_rect, update_rect);
         }
         else
            _be_allegro_view->DrawBitmapAsync(w->buffer, update_rect, update_rect);
      }
      _be_allegro_view->Sync();
      w->Unlock();
      snooze(UPDATER_PERIOD);
   }
   
   return B_OK;
}



/* be_gfx_bwindow_acquire:
 *  Locks the screen bitmap.
 */
extern "C" void be_gfx_bwindow_acquire(struct BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
   }
}



/* be_gfx_bwindow_release:
 *  Unlocks the screen bitmap.
 */
extern "C" void be_gfx_bwindow_release(struct BITMAP *bmp)
{
   if (bmp->id & BMP_ID_LOCKED) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      release_sem(_be_window_lock);
   }
}



#ifdef ALLEGRO_NO_ASM

/* _be_gfx_bwindow_read_write_bank:
 *  Returns new line and marks it as dirty.
 */
extern "C" uintptr_t _be_gfx_bwindow_read_write_bank(BITMAP *bmp, int line)
{
   if (!bmp->id & BMP_ID_LOCKED) {
      bmp->id |= (BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
   _be_dirty_lines[bmp->y_ofs + line] = 1;
   return (unsigned long)(bmp->line[line]);
}



/* _be_gfx_bwindow_unwrite_bank:
 *  If necessary, unlocks bitmap and makes the drawing thread
 *  to update.
 */
extern "C" void _be_gfx_bwindow_unwrite_bank(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
      release_sem(_be_window_lock);
   }
}

#endif



/* BeAllegroWindow::BeAllegroWindow:
 *  Constructor, creates the window and the BBitmap framebuffer,
 *  and starts the drawing thread.
 */
BeAllegroWindow::BeAllegroWindow(BRect frame, const char *title,
   window_look look, window_feel feel, uint32 flags, uint32 workspaces,
   uint32 v_w, uint32 v_h, uint32 color_depth)
   : BWindow(frame, title, look, feel, flags, workspaces)
{
   BRect rect = Bounds();
   uint32 i;
   color_space space = B_NO_COLOR_SPACE;

   _be_allegro_view = new BeAllegroView(rect, "Allegro",
      B_FOLLOW_ALL_SIDES, B_WILL_DRAW, 0);
   rgb_color color = {0, 0, 0, 0};
   _be_allegro_view->SetViewColor(color);
   AddChild(_be_allegro_view);
   
   switch (color_depth) {
      case 8:  space = B_CMAP8; break;
      case 15: space = B_RGB15; break;
      case 16: space = B_RGB16; break;
      case 32: space = B_RGB32; break;
   }
   buffer = new BBitmap(rect, space);
   if (color_depth == 8)
      aux_buffer = new BBitmap(rect, B_CMAP8);
   else
      aux_buffer = NULL;
   dying = false;
   screen_width = (int)rect.right + 1;
   screen_height = (int)rect.bottom + 1;
   screen_depth = color_depth;
   
   _be_dirty_lines = (int *)calloc(v_h, sizeof(int));
   
   _be_window_lock = create_sem(0, "window lock");
   
   drawing_thread_id = spawn_thread(window_updater, "window updater",
      B_REAL_TIME_DISPLAY_PRIORITY, (void *)this);
   resume_thread(drawing_thread_id);
}



/* BeAllegroWindow::~BeAllegroWindow:
 *  Stops the drawing thread and frees used memory.
 */
BeAllegroWindow::~BeAllegroWindow()
{
   int32 result;

   dying = true;
   delete_sem(_be_window_lock);
   Unlock();
   wait_for_thread(drawing_thread_id, &result);
   drawing_thread_id = -1;
   Hide();

   _be_focus_count = 0;
   
   if (buffer)
      delete buffer;
   buffer = NULL;
   if (aux_buffer)
      delete aux_buffer;
   aux_buffer = NULL;
   
   if (_be_dirty_lines) {
      free(_be_dirty_lines);
      _be_dirty_lines = NULL;
   }
}



/* BeAllegroWindow::MessageReceived:
 *  System messages handler.
 */
void BeAllegroWindow::MessageReceived(BMessage *message)
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
         BWindow::MessageReceived(message);
         break;
   }
}



/* BeAllegroWindow::WindowActivated:
 *  Callback for when the window gains/looses focus.
 */
void BeAllegroWindow::WindowActivated(bool active)
{
   _be_change_focus(active);
   if (active) {
      _be_allegro_window->Lock();
      for (int i=0; i<_be_allegro_window->screen_height; i++)
         _be_dirty_lines[i] = 1;
      _be_allegro_window->Unlock();
      release_sem(_be_window_lock);
   }
   BWindow::WindowActivated(active);
}



/* BeAllegroWindow::QuitRequested:
 *  User requested to close the program window.
 */
bool BeAllegroWindow::QuitRequested(void)
{
    return _be_handle_window_close(Title());
}



/* be_gfx_bwindow_init:
 *  Initializes specified video mode.
 */
extern "C" struct BITMAP *be_gfx_bwindow_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   
   if (1
#ifdef ALLEGRO_COLOR8
       && (color_depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (color_depth != 15)
       && (color_depth != 16)
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

   _be_allegro_window = new BeAllegroWindow(BRect(0, 0, w-1, h-1), wnd_title,
			      B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			      B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
			      B_CURRENT_WORKSPACE, v_w, v_h, color_depth);
   _be_window = _be_allegro_window;
   if (!_be_allegro_window->buffer->IsValid() ||
       ((color_depth == 8) && (!_be_allegro_window->aux_buffer->IsValid()))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }
   
   _be_mouse_view = new BView(_be_allegro_window->Bounds(),
			     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   _be_allegro_window->Lock();
   _be_allegro_window->AddChild(_be_mouse_view);
   _be_allegro_window->Unlock();

   _be_mouse_window = _be_allegro_window;
   _be_mouse_window_mode = true;

   release_sem(_be_mouse_view_attached);
   
   _be_allegro_window->MoveTo(6, 25);
   _be_allegro_window->Show();

   gfx_beos_bwindow.w       = w;
   gfx_beos_bwindow.h       = h;
   gfx_beos_bwindow.linear  = TRUE;
   gfx_beos_bwindow.vid_mem = _be_allegro_window->buffer->BitsLength();
   
   bmp = _make_bitmap(v_w, v_h, (unsigned long)_be_allegro_window->buffer->Bits(),
              &gfx_beos_bwindow, color_depth,
		      _be_allegro_window->buffer->BytesPerRow());
  
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
   
   _screen_vtable.acquire = be_gfx_bwindow_acquire;
   _screen_vtable.release = be_gfx_bwindow_release;
   
   _be_gfx_set_truecolor_shifts();

   uszprintf(driver_desc, sizeof(driver_desc), get_config_text("BWindow object, %d bit BBitmap framebuffer"),
             color_depth);
   gfx_beos_bwindow.desc = driver_desc;
   
   snooze(50000);
   _be_gfx_initialized = true;
   
   return bmp;

cleanup:
   be_gfx_bwindow_exit(NULL);
   return NULL;
}



/* be_gfx_bwindow_exit:
 *  Shuts down the driver.
 */
extern "C" void be_gfx_bwindow_exit(struct BITMAP *bmp)
{
   _be_gfx_initialized = false;

   if (_be_allegro_window) {
      if (_be_mouse_view_attached < 1) {
         acquire_sem(_be_mouse_view_attached);
      }

      _be_allegro_window->Lock();
      _be_allegro_window->Quit();

      _be_allegro_window = NULL;
      _be_window = NULL;
   }
   _be_mouse_window   = NULL;	    
   _be_mouse_view     = NULL;
}



/* be_gfx_bwindow_set_palette:
 *  Sets the internal color map to reflect the given palette, and
 *  makes the drawing thread to update the window.
 */
extern "C" void be_gfx_bwindow_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;
   
   if (vsync)
      be_gfx_vsync();
   
   if (_be_allegro_window->screen_depth != 8)
      return;
   
   _be_allegro_window->Lock();
   for (i=from; i<=to; i++)
      cmap[i] = BScreen().IndexForColor(p[i].r << 2, p[i].g << 2, p[i].b << 2);
   for (i=0; i<_be_allegro_window->screen_height; i++)
      _be_dirty_lines[i] = 1;
   _be_allegro_window->Unlock();
   release_sem(_be_window_lock);
}

