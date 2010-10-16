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
 *      BeOS fullscreen overlay gfx driver.
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


static char driver_desc[256] = EMPTY_STRING;
static display_mode old_display_mode;



/* BeAllegroOverlay::BeAllegroOverlay:
 *  Constructor, creates the window and the overlay bitmap to be used
 *  as framebuffer.
 */
BeAllegroOverlay::BeAllegroOverlay(BRect frame, const char *title,
   window_look look, window_feel feel, uint32 flags, uint32 workspaces,
   uint32 v_w, uint32 v_h, uint32 color_depth)
   : BWindow(frame, title, look, feel, flags, workspaces)
{
   BRect rect = Bounds();
   color_space space = B_NO_COLOR_SPACE;

   _be_allegro_view = new BeAllegroView(rect, "Allegro",
      B_FOLLOW_ALL_SIDES, B_WILL_DRAW, BE_ALLEGRO_VIEW_OVERLAY);
   AddChild(_be_allegro_view);
   
   switch (color_depth) {
      case 15: space = B_RGB15; break;
      case 16: space = B_RGB16; break;
      case 32: space = B_RGB32; break;
   }
   buffer = new BBitmap(rect,
      B_BITMAP_IS_CONTIGUOUS |
      B_BITMAP_WILL_OVERLAY |
      B_BITMAP_RESERVE_OVERLAY_CHANNEL,
      space);
}



/* BeAllegroOverlay::BeAllegroOverlay:
 *  Deletes the overlay bitmap.
 */
BeAllegroOverlay::~BeAllegroOverlay()
{
   Hide();
   _be_focus_count = 0;
   
   if (buffer)
      delete buffer;
   buffer = NULL;
}



/* BeAllegroOverlay::MessageReceived:
 *  System messages handler.
 */
void BeAllegroOverlay::MessageReceived(BMessage *message)
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



/* BeAllegroOverlay::WindowActivated:
 *  Callback for when the window gains/looses focus.
 */
void BeAllegroOverlay::WindowActivated(bool active)
{
   _be_change_focus(active);
   BWindow::WindowActivated(active);
}



/* BeAllegroOverlay::QuitRequested:
 *  User requested to close the program window.
 */
bool BeAllegroOverlay::QuitRequested(void)
{
    return _be_handle_window_close(Title());
}



/* is_overlay_supported:
 *  Checks if the card can support hardware scaling from given
 *  source to dest rectangles.
 */
static bool is_overlay_supported(BRect *src, BRect *dest)
{
   overlay_restrictions restrictions;
   float src_w, src_h, dest_w, dest_h;
   
   if (_be_allegro_overlay->buffer->GetOverlayRestrictions(&restrictions) != B_OK)
      return false;
   
   src_w = src->right - src->left + 1;
   src_h = src->bottom - src->top + 1;
   dest_w = dest->right - dest->left + 1;
   dest_h = dest->bottom - dest->top + 1;
   
   return ((src_w * restrictions.min_width_scale <= dest_w) &&
           (src_w * restrictions.max_width_scale >= dest_w) &&
           (src_h * restrictions.min_height_scale <= dest_h) &&
           (src_h * restrictions.max_height_scale >= dest_h));
}



/* be_gfx_overlay_init:
 *  Sets up overlay video mode.
 */
extern "C" struct BITMAP *be_gfx_overlay_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   BRect src, dest;
   int i;
   
   if (1
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
   
   BScreen().GetMode(&old_display_mode);
   for (i=0; _be_mode_table[i].d > 0; i++) {
      if ((_be_mode_table[i].d == color_depth) &&
          (_be_mode_table[i].w >= w) &&
          (_be_mode_table[i].h >= h))
         break;
   }
   if ((_be_mode_table[i].d <= 0) ||
       (set_screen_space(0, _be_mode_table[i].mode, false) != B_OK)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto cleanup;
   }
   
   src = BRect(0, 0, w - 1, h - 1);
   dest = BScreen().Frame();
   
   _be_allegro_overlay = new BeAllegroOverlay(src, wnd_title,
			      B_NO_BORDER_WINDOW_LOOK,
			      B_NORMAL_WINDOW_FEEL,
			      B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
			      B_CURRENT_WORKSPACE, v_w, v_h, color_depth);
   _be_window = _be_allegro_overlay;
   
   if (!_be_allegro_overlay->buffer) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }
   if ((_be_allegro_overlay->buffer->InitCheck() != B_OK) ||
       (!_be_allegro_overlay->buffer->IsValid()) ||
       (!is_overlay_supported(&src, &dest))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Overlays not supported"));
      goto cleanup;
   }
   
   _be_mouse_view = new BView(_be_allegro_overlay->Bounds(),
			     "allegro mouse view", B_FOLLOW_ALL_SIDES, 0);
   _be_allegro_overlay->Lock();
   _be_allegro_overlay->AddChild(_be_mouse_view);
   _be_allegro_overlay->Unlock();

   _be_mouse_window = _be_allegro_overlay;
   _be_mouse_window_mode = true;

   release_sem(_be_mouse_view_attached);
   
   _be_allegro_view->SetViewOverlay(_be_allegro_overlay->buffer,
      src, dest, &_be_allegro_overlay->color_key, B_FOLLOW_ALL,
      B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
   _be_allegro_view->ClearViewOverlay();
   
   _be_allegro_overlay->ResizeTo(dest.right + 1, dest.bottom + 1);
   _be_allegro_overlay->MoveTo(0, 0);
   _be_allegro_overlay->Show();

   gfx_beos_overlay.w       = w;
   gfx_beos_overlay.h       = h;
   gfx_beos_overlay.linear  = TRUE;
   gfx_beos_overlay.vid_mem = _be_allegro_overlay->buffer->BitsLength();
   
   bmp = _make_bitmap(v_w, v_h, (unsigned long)_be_allegro_overlay->buffer->Bits(),
              &gfx_beos_overlay, color_depth,
		      _be_allegro_overlay->buffer->BytesPerRow());
  
   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      goto cleanup;
   }
   
   _be_gfx_set_truecolor_shifts();

   uszprintf(driver_desc, sizeof(driver_desc), get_config_text("BWindow object, %d bit BBitmap framebuffer (overlay)"),
             color_depth);
   gfx_beos_overlay.desc = driver_desc;
   
   _be_gfx_initialized = true;
   
   return bmp;

cleanup:
   be_gfx_overlay_exit(NULL);
   return NULL;
}



/* be_gfx_overlay_exit:
 *  Shuts down the driver.
 */
extern "C" void be_gfx_overlay_exit(struct BITMAP *bmp)
{
   _be_gfx_initialized = false;
   
   if (_be_allegro_overlay) {
      if (_be_mouse_view_attached < 1) {
         acquire_sem(_be_mouse_view_attached);
      }

      _be_allegro_overlay->Lock();
      _be_allegro_overlay->Quit();

      _be_allegro_overlay = NULL;
      _be_window = NULL;
   }
   _be_mouse_window   = NULL;	    
   _be_mouse_view     = NULL;
   
   BScreen().SetMode(&old_display_mode);
}

