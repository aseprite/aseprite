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
 *      Definitions for internal use by the BeOS configuration.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */


#include "bealleg.h"

#ifdef __cplusplus
extern status_t       ignore_result;

extern volatile int32 focus_count;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define WND_TITLE_SIZE  128

AL_ARRAY(char, wnd_title);

int  be_key_init(void);
void be_key_exit(void);
void be_key_set_leds(int leds);
void be_key_set_rate(int delay, int repeat);
void be_key_wait_for_input(void);
void be_key_stop_waiting_for_input(void);
void be_key_suspend(void);
void be_key_resume(void);

int  be_sys_init(void);
void be_sys_exit(void);
void _be_sys_get_executable_name(char *output, int size);
void be_sys_get_executable_name(char *output, int size);
int be_sys_find_resource(char *dest, AL_CONST char *resource, int size);
void be_sys_set_window_title(AL_CONST char *name);
int be_sys_set_close_button_callback(void (*proc)(void));
void be_sys_message(AL_CONST char *msg);
int be_sys_set_display_switch_mode(int mode);
int be_sys_desktop_color_depth(void);
int be_sys_get_desktop_resolution(int *width, int *height);
void be_sys_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);
void be_sys_yield_timeslice(void);
void *be_sys_create_mutex(void);
void be_sys_destroy_mutex(void *handle);
void be_sys_lock_mutex(void *handle);
void be_sys_unlock_mutex(void *handle);
void be_sys_suspend(void);
void be_sys_resume(void);
void be_main_suspend(void);
void be_main_resume(void);

struct BITMAP *be_gfx_bwindowscreen_accel_init(int w, int h, int v_w, int v_h, int color_depth);
struct BITMAP *be_gfx_bwindowscreen_init(int w, int h, int v_w, int v_h, int color_depth);
void be_gfx_bwindowscreen_exit(struct BITMAP *b);
void be_gfx_bwindowscreen_acquire(struct BITMAP *b);
void be_gfx_bwindowscreen_release(struct BITMAP *b);
void be_gfx_bwindowscreen_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync);
int be_gfx_bwindowscreen_scroll(int x, int y);
int be_gfx_bwindowscreen_request_scroll(int x, int y);
int be_gfx_bwindowscreen_poll_scroll(void);
int be_gfx_bwindowscreen_request_video_bitmap(struct BITMAP *bitmap);
void be_gfx_vsync(void);
struct GFX_MODE_LIST *be_gfx_bwindowscreen_fetch_mode_list(void);
void be_gfx_bwindowscreen_accelerate(int color_depth);
#ifdef ALLEGRO_NO_ASM
uintptr_t be_gfx_bwindowscreen_read_write_bank(BITMAP *bmp, int lyne);
void be_gfx_bwindowscreen_unwrite_bank(BITMAP *bmp);
#else
uintptr_t _be_gfx_bwindowscreen_read_write_bank_asm(BITMAP *bmp, int lyne);
void _be_gfx_bwindowscreen_unwrite_bank_asm(BITMAP *bmp);
#endif

struct BITMAP *be_gfx_bdirectwindow_init(int w, int h, int v_w, int v_h, int color_depth);
void be_gfx_bdirectwindow_exit(struct BITMAP *b);
void be_gfx_bdirectwindow_acquire(struct BITMAP *bmp);
void be_gfx_bdirectwindow_release(struct BITMAP *bmp);
void be_gfx_bdirectwindow_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync);

struct BITMAP *be_gfx_bwindow_init(int w, int h, int v_w, int v_h, int color_depth);
void be_gfx_bwindow_exit(struct BITMAP *b);
void be_gfx_bwindow_acquire(struct BITMAP *bmp);
void be_gfx_bwindow_release(struct BITMAP *bmp);
void be_gfx_bwindow_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync);

#ifdef ALLEGRO_NO_ASM
void _be_gfx_bwindow_unwrite_bank(BITMAP *bmp);
uintptr_t _be_gfx_bwindow_read_write_bank(BITMAP *bmp, int lyne);
#else
void _be_gfx_bwindow_unwrite_bank_asm(BITMAP *bmp);
uintptr_t _be_gfx_bwindow_read_write_bank_asm(BITMAP *bmp, int lyne);
#endif

struct BITMAP *be_gfx_overlay_init(int w, int h, int v_w, int v_h, int color_depth);
void be_gfx_overlay_exit(struct BITMAP *b);

int  be_time_init(void);
void be_time_exit(void);
void be_time_rest(unsigned int tyme, AL_METHOD(void, callback, (void)));
void be_time_suspend(void);
void be_time_resume(void);

int be_mouse_init(void);
void be_mouse_exit(void);
void be_mouse_position(int x, int y);
void be_mouse_set_range(int x1, int y_1, int x2, int y2);
void be_mouse_set_speed(int xspeed, int yspeed);
void be_mouse_get_mickeys(int *mickeyx, int *mickeyy);

int be_joy_init(void);
void be_joy_exit(void);
int be_joy_poll(void);

int be_sound_detect(int input);
int be_sound_init(int input, int voices);
void be_sound_exit(int input);
void *be_sound_lock_voice(int voice, int start, int end);
void be_sound_unlock_voice(int voice);
int be_sound_buffer_size(void);
int be_sound_set_mixer_volume(int volume);
int be_sound_get_mixer_volume(void);
void be_sound_suspend(void);
void be_sound_resume(void);

int be_midi_detect(int input);
int be_midi_init(int input, int voices);
void be_midi_exit(int input);
int be_midi_set_mixer_volume(int volume);
int be_midi_get_mixer_volume(void);
void be_midi_key_on(int inst, int note, int bend, int vol, int pan);
void be_midi_key_off(int voice);
void be_midi_set_volume(int voice, int vol);
void be_midi_set_pitch(int voice, int note, int bend);
void be_midi_set_pan(int voice, int pan);

#ifdef __cplusplus
}
#endif
