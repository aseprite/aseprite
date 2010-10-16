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
 *      Common gfx subsystem routines.
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


#define SUSPEND_ALL_THREADS()		\
{									\
   be_key_suspend();				\
   be_sound_suspend();				\
   be_time_suspend();				\
   be_sys_suspend();				\
   be_main_suspend();				\
}

#define RESUME_ALL_THREADS()		\
{									\
   be_key_resume();					\
   be_sound_resume();				\
   be_time_resume();				\
   be_sys_resume();					\
   be_main_resume();				\
}



AL_CONST BE_MODE_TABLE _be_mode_table[] = {
   { 8,    640,  400, B_8_BIT_640x400,    B_CMAP8 },
   { 8,    640,  480, B_8_BIT_640x480,    B_CMAP8 },
   { 8,    800,  600, B_8_BIT_800x600,    B_CMAP8 },
   { 8,   1024,  768, B_8_BIT_1024x768,   B_CMAP8 },
   { 8,   1152,  900, B_8_BIT_1152x900,   B_CMAP8 },
   { 8,   1280, 1024, B_8_BIT_1280x1024,  B_CMAP8 },
   { 8,   1600, 1200, B_8_BIT_1600x1200,  B_CMAP8 },
   { 15,   640,  480, B_15_BIT_640x480,   B_RGB15 },
   { 15,   800,  600, B_15_BIT_800x600,   B_RGB15 },
   { 15,  1024,  768, B_15_BIT_1024x768,  B_RGB15 },
   { 15,  1152,  900, B_15_BIT_1152x900,  B_RGB15 },
   { 15,  1280, 1024, B_15_BIT_1280x1024, B_RGB15 },
   { 15,  1600, 1200, B_15_BIT_1600x1200, B_RGB15 },
   { 16,   640,  480, B_16_BIT_640x480,   B_RGB16 },
   { 16,   800,  600, B_16_BIT_800x600,   B_RGB16 },
   { 16,  1024,  768, B_16_BIT_1024x768,  B_RGB16 },
   { 16,  1152,  900, B_16_BIT_1152x900,  B_RGB16 },
   { 16,  1280, 1024, B_16_BIT_1280x1024, B_RGB16 },
   { 16,  1600, 1200, B_16_BIT_1600x1200, B_RGB16 },
   { 32,   640,  480, B_32_BIT_640x480,   B_RGB32 },
   { 32,   800,  600, B_32_BIT_800x600,   B_RGB32 },
   { 32,  1024,  768, B_32_BIT_1024x768,  B_RGB32 },
   { 32,  1152,  900, B_32_BIT_1152x900,  B_RGB32 },
   { 32,  1280, 1024, B_32_BIT_1280x1024, B_RGB32 },
   { 32,  1600, 1200, B_32_BIT_1600x1200, B_RGB32 },
   { -1,     0,    0,                  0,       0 }
};

sem_id _be_fullscreen_lock = -1;
sem_id _be_window_lock = -1;
volatile int _be_lock_count  = 0;
int *_be_dirty_lines = NULL;
int _be_mouse_z = 0;
void (*_be_window_close_hook)() = NULL;
volatile bool _be_gfx_initialized = false;

BeAllegroView         *_be_allegro_view = NULL;
BeAllegroScreen       *_be_allegro_screen = NULL; 
BeAllegroWindow       *_be_allegro_window = NULL;
BeAllegroDirectWindow *_be_allegro_direct_window = NULL;
BeAllegroOverlay      *_be_allegro_overlay = NULL;
BWindow               *_be_window = NULL;

static int refresh_rate = 60;



/* BeAllegroView::BeAllegroView:
 */
BeAllegroView::BeAllegroView(BRect frame, const char *name,
   uint32 resizingMode, uint32 flags, int f)
   : BView(frame, name, resizingMode, flags)
{
   this->flags = f;
}



/* BeAllegroView::MessageReceived:
 */
void BeAllegroView::MessageReceived(BMessage *message)
{
   switch (message->what) {
      case B_SIMPLE_DATA:
         break;

      default:
         BView::MessageReceived(message);
         break;
   }
}



void BeAllegroView::Draw(BRect update_rect)
{
   if (flags & BE_ALLEGRO_VIEW_OVERLAY) {
      SetHighColor(_be_allegro_overlay->color_key);
      FillRect(update_rect);
      return;
   }
   
   if ((flags & BE_ALLEGRO_VIEW_DIRECT) || (!_be_allegro_window))
      return;
   
   if ((!_be_focus_count) && 
       ((_be_switch_mode == SWITCH_AMNESIA) ||
        (_be_switch_mode == SWITCH_BACKAMNESIA)))
      return;
   
   if (_be_allegro_window->screen_depth == 8)
      DrawBitmap(_be_allegro_window->aux_buffer, update_rect, update_rect);
   else
      DrawBitmap(_be_allegro_window->buffer, update_rect, update_rect);
}



void _be_gfx_set_truecolor_shifts()
{
   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   
   _rgb_r_shift_16 = 11;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;
   
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   
   _rgb_a_shift_32 = 24;
   _rgb_r_shift_32 = 16; 
   _rgb_g_shift_32 = 8; 
   _rgb_b_shift_32 = 0;
}



void _be_change_focus(bool active)
{
   int i;
   
   if (_be_focus_count < 0)
      _be_focus_count = 0;
      
   if (active) {
      _be_focus_count++;
      if (_be_gfx_initialized) {
         switch (_be_switch_mode) {
            case SWITCH_AMNESIA:
            case SWITCH_BACKAMNESIA:
               if ((_be_allegro_direct_window) &&
                   (_be_allegro_direct_window->drawing_thread_id > 0)) {
                  resume_thread(_be_allegro_direct_window->drawing_thread_id);
               }
               if (_be_switch_mode == SWITCH_BACKAMNESIA)
                  break;
            case SWITCH_PAUSE:
               RESUME_ALL_THREADS();
               break;
         }
         _switch_in();
      }
   }
   else {
      _be_focus_count--;
      if (_be_gfx_initialized) {
         _switch_out();
         switch (_be_switch_mode) {
            case SWITCH_AMNESIA:
            case SWITCH_BACKAMNESIA:
               if ((_be_allegro_direct_window) &&
                   (_be_allegro_direct_window->drawing_thread_id > 0)) {
                  suspend_thread(_be_allegro_direct_window->drawing_thread_id);
               }
               if (_be_switch_mode == SWITCH_BACKAMNESIA)
                  break;
            case SWITCH_PAUSE:
               if (_be_midisynth)
                  _be_midisynth->AllNotesOff(false);
               SUSPEND_ALL_THREADS();
               break;
         }
      }
   }
}



bool _be_handle_window_close(const char *title)
{
   if (_be_window_close_hook)
      _be_window_close_hook();

   return false;
}



extern "C" void be_gfx_vsync(void)
{
   if(BScreen(_be_window).WaitForRetrace() != B_OK) {
      if (_timer_installed) {
         int start_count;
         
         start_count = retrace_count;

         while (start_count == retrace_count) {
         }
      }
      else {
         snooze (500000 / refresh_rate);
      }
   }
}

