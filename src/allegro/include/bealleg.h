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
 *      The Be Allegro Header File.
 *
 *      Aren't you glad you used the BeOS header file?
 *      Don't you wish everybody did?
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#ifndef BE_ALLEGRO_H
#define BE_ALLEGRO_H

/* There is namespace cross-talk between the Be API and
   Allegro, lets use my favorite namespace detergent.
   "Kludge!" Now with 25% more hack. */

#ifdef __cplusplus
# define color_map    al_color_map
# define drawing_mode al_drawing_mode
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifdef __cplusplus
# undef color_map
# undef drawing_mode
# undef MIN
# undef MAX
# undef TRACE
# undef ASSERT

#ifdef DEBUGMODE
# define AL_ASSERT(condition)  { if (!(condition)) al_assert(__FILE__, __LINE__); }
#else
# define AL_ASSERT(condition)
#endif

#ifndef SCAN_DEPEND
   #include <Be.h>
#endif

#define BE_ALLEGRO_VIEW_DIRECT    1
#define BE_ALLEGRO_VIEW_OVERLAY   2

#define LINE8_HOOK_NUM    3
#define LINE16_HOOK_NUM  12
#define LINE32_HOOK_NUM   4
#define ARRAY8_HOOK_NUM   8
#define ARRAY32_HOOK_NUM  9
#define RECT8_HOOK_NUM    5
#define RECT16_HOOK_NUM  16
#define RECT32_HOOK_NUM   6
#define INVERT_HOOK_NUM  11
#define BLIT_HOOK_NUM     7
#define SYNC_HOOK_NUM    10



typedef int32 (*LINE8_HOOK)(int32 startX, int32 endX, int32 startY,
   int32 endY, uint8 colorIndex, bool clipToRect, int16 clipLeft,
   int16 clipTop, int16 clipRight, int16 clipBottom);

typedef int32 (*ARRAY8_HOOK)(indexed_color_line *array,
   int32 numItems, bool clipToRect, int16 top, int16 right,
   int16 bottom, int16 left);

typedef int32 (*LINE16_HOOK)(int32 startX, int32 endX, int32 startY,
   int32 endY, uint16 color, bool clipToRect, int16 clipLeft,
   int16 clipTop, int16 clipRight, int16 clipBottom);

typedef int32 (*LINE32_HOOK)(int32 startX, int32 endX, int32 startY,
   int32 endY, uint32 color, bool clipToRect, int16 clipLeft,
   int16 clipTop, int16 clipRight, int16 clipBottom);

typedef int32 (*ARRAY32_HOOK)(rgb_color_line *array,
   int32 numItems, bool clipToRect, int16 top, int16 right,
   int16 bottom, int16 left);

typedef int32 (*RECT8_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom, uint8 colorIndex);

typedef int32 (*RECT16_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom, uint16 color);

typedef int32 (*RECT32_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom, uint32 color);

typedef int32 (*INVERT_HOOK)(int32 left, int32 top, int32 right,
   int32 bottom);

typedef int32 (*BLIT_HOOK)(int32 sourceX, int32 sourceY,
   int32 destinationX, int32 destinationY, int32 width,
   int32 height);

typedef int32 (*SYNC_HOOK)(void);



typedef struct HOOKS {
   LINE8_HOOK   draw_line_8;
   LINE16_HOOK  draw_line_16;
   LINE32_HOOK  draw_line_32;
   ARRAY8_HOOK  draw_array_8;
   ARRAY32_HOOK draw_array_32;
   RECT8_HOOK   draw_rect_8;
   RECT16_HOOK  draw_rect_16;
   RECT32_HOOK  draw_rect_32;
   INVERT_HOOK  invert_rect;
   BLIT_HOOK    blit;
   SYNC_HOOK    sync;
} HOOKS;



typedef struct {
   int    d, w, h;
   uint32 mode;
   uint32 space;
} BE_MODE_TABLE;



class BeAllegroView
   : public BView
{
   public:
      BeAllegroView(BRect, const char *, uint32, uint32, int);
      
      void MessageReceived(BMessage *);
      void Draw(BRect);
      
   private:
      int flags;
};



class BeAllegroWindow
   : public BWindow
{
   public:
      BeAllegroWindow(BRect, const char *, window_look, window_feel, uint32, uint32, uint32, uint32, uint32);
     ~BeAllegroWindow();

      void MessageReceived(BMessage *);
      bool QuitRequested(void);
      void WindowActivated(bool);
      
      int screen_depth;
      int screen_width;
      int screen_height;
      BBitmap *buffer;
      BBitmap *aux_buffer;
      bool dying;
      thread_id	drawing_thread_id;
};



class BeAllegroDirectWindow
   : public BDirectWindow
{
   public:
      BeAllegroDirectWindow(BRect, const char *, window_look, window_feel, uint32, uint32, uint32, uint32, uint32);
     ~BeAllegroDirectWindow();

      void DirectConnected(direct_buffer_info *);
      void MessageReceived(BMessage *);
      bool QuitRequested(void);
      void WindowActivated(bool);

      void *screen_data;	  
      uint32 screen_pitch;
      uint32 screen_height;
      uint32 screen_depth;

      void *display_data;
      uint32 display_pitch;
      uint32 display_depth;

      uint32 num_rects;
      clipping_rect *rects;
      clipping_rect window;
      COLORCONV_BLITTER_FUNC *blitter;

      bool connected;
      bool dying;
      BLocker *locker;
      thread_id	drawing_thread_id;
};



class BeAllegroScreen
   : public BWindowScreen
{
   public:
      BeAllegroScreen(const char *title, uint32 space, status_t *error, bool debugging = false);

      void MessageReceived(BMessage *);
      bool QuitRequested(void);
      void ScreenConnected(bool connected);
};



class BeAllegroOverlay
   : public BWindow
{
   public:
      BeAllegroOverlay(BRect, const char *, window_look, window_feel, uint32, uint32, uint32, uint32, uint32);
     ~BeAllegroOverlay();

      void MessageReceived(BMessage *);
      bool QuitRequested(void);
      void WindowActivated(bool);
      
      BBitmap *buffer;
      rgb_color color_key;
};



class BeAllegroApp
   : public BApplication
{
   public:
      BeAllegroApp(const char *signature);

      bool QuitRequested(void);
      void ReadyToRun(void);
};



AL_VAR(BeAllegroApp, *_be_allegro_app);
AL_VAR(BeAllegroWindow, *_be_allegro_window);
AL_VAR(BeAllegroDirectWindow, *_be_allegro_direct_window);
AL_VAR(BeAllegroView, *_be_allegro_view);
AL_VAR(BeAllegroScreen, *_be_allegro_screen);
AL_VAR(BeAllegroOverlay, *_be_allegro_overlay);
AL_VAR(BWindow, *_be_window);
AL_VAR(BMidiSynth, *_be_midisynth);
AL_VAR(sem_id, _be_sound_stream_lock);
AL_VAR(sem_id, _be_fullscreen_lock);
AL_VAR(sem_id, _be_window_lock);
AL_VAR(sem_id, _be_mouse_view_attached);
AL_VAR(BWindow, *_be_mouse_window);
AL_VAR(BView, *_be_mouse_view);
AL_VAR(int, _be_mouse_z);
AL_VAR(bool, _be_mouse_window_mode);
AL_VAR(HOOKS, _be_hooks);
AL_VAR(int, _be_switch_mode);
AL_VAR(volatile int, _be_lock_count);
AL_VAR(volatile int, _be_focus_count);
AL_VAR(volatile bool, _be_gfx_initialized);
AL_VAR(int, *_be_dirty_lines);

AL_ARRAY(AL_CONST BE_MODE_TABLE, _be_mode_table);


extern int32 (*_be_sync_func)();
extern void (*_be_window_close_hook)();

void _be_terminate(thread_id caller, bool exit_caller);
void _be_gfx_set_truecolor_shifts();
void _be_change_focus(bool active);
bool _be_handle_window_close(const char *title);



#endif /* ifdef C++ */



#endif /* ifndef BE_ALLEGRO_H */
