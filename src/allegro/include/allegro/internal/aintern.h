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
 *      Some definitions for internal use by the library code.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTERN_H
#define AINTERN_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifdef __cplusplus
   extern "C" {
#endif


/* length in bytes of the cpu_vendor string */
#define _AL_CPU_VENDOR_SIZE 32


/* flag for how many times we have been initialised */
AL_VAR(int, _allegro_count);


/* flag to know whether we are being called by the exit mechanism */
AL_VAR(int, _allegro_in_exit);


/* flag to decide whether to disable the screensaver */
enum {
  NEVER_DISABLED,
  FULLSCREEN_DISABLED,
  ALWAYS_DISABLED
};

AL_VAR(int, _screensaver_policy);


AL_FUNCPTR(int, _al_trace_handler, (AL_CONST char *msg));


/* malloc wrappers */
/* The 4.3 branch uses the following macro names to allow the user to customise
 * the memory management routines.  We don't have that feature in 4.2, but we
 * use the same macros names in order to reduce divergence of the codebases.
 */
#define _AL_MALLOC(SIZE)         (_al_malloc(SIZE))
#define _AL_MALLOC_ATOMIC(SIZE)  (_al_malloc(SIZE))
#define _AL_FREE(PTR)            (_al_free(PTR))
#define _AL_REALLOC(PTR, SIZE)   (_al_realloc(PTR, SIZE))

AL_FUNC(void *, _al_malloc, (size_t size));
AL_FUNC(void, _al_free, (void *mem));
AL_FUNC(void *, _al_realloc, (void *mem, size_t size));
AL_FUNC(char *, _al_strdup, (AL_CONST char *string));
AL_FUNC(char *, _al_ustrdup, (AL_CONST char *string));



/* some Allegro functions need a block of scratch memory */
AL_VAR(void *, _scratch_mem);
AL_VAR(int, _scratch_mem_size);


AL_INLINE(void, _grow_scratch_mem, (int size),
{
   if (size > _scratch_mem_size) {
      size = (size+1023) & 0xFFFFFC00;
      _scratch_mem = _AL_REALLOC(_scratch_mem, size);
      _scratch_mem_size = size;
   }
})


/* list of functions to call at program cleanup */
AL_FUNC(void, _add_exit_func, (AL_METHOD(void, func, (void)), AL_CONST char *desc));
AL_FUNC(void, _remove_exit_func, (AL_METHOD(void, func, (void))));


/* helper structure for talking to Unicode strings */
typedef struct UTYPE_INFO
{
   int id;
   AL_METHOD(int, u_getc, (AL_CONST char *s));
   AL_METHOD(int, u_getx, (char **s));
   AL_METHOD(int, u_setc, (char *s, int c));
   AL_METHOD(int, u_width, (AL_CONST char *s));
   AL_METHOD(int, u_cwidth, (int c));
   AL_METHOD(int, u_isok, (int c));
   int u_width_max;
} UTYPE_INFO;

AL_FUNC(UTYPE_INFO *, _find_utype, (int type));


/* message stuff */
#define ALLEGRO_MESSAGE_SIZE  4096


/* wrappers for implementing disk I/O on different platforms */
AL_FUNC(int, _al_file_isok, (AL_CONST char *filename));
AL_FUNC(uint64_t, _al_file_size_ex, (AL_CONST char *filename));
AL_FUNC(time_t, _al_file_time, (AL_CONST char *filename));
AL_FUNC(int, _al_drive_exists, (int drive));
AL_FUNC(int, _al_getdrive, (void));
AL_FUNC(void, _al_getdcwd, (int drive, char *buf, int size));

AL_FUNC(void, _al_detect_filename_encoding, (void));

/* obsolete; only exists for binary compatibility with 4.2.0 */
AL_FUNC(long, _al_file_size, (AL_CONST char *filename));


/* packfile stuff */
AL_VAR(int, _packfile_filesize);
AL_VAR(int, _packfile_datasize);
AL_VAR(int, _packfile_type);
AL_FUNC(PACKFILE *, _pack_fdopen, (int fd, AL_CONST char *mode));

AL_FUNC(int, _al_lzss_incomplete_state, (AL_CONST LZSS_UNPACK_DATA *dat));


/* config stuff */
void _reload_config(void);


/* various bits of mouse stuff */
AL_FUNC(void, _handle_mouse_input, (void));

AL_VAR(int, _mouse_x);
AL_VAR(int, _mouse_y);
AL_VAR(int, _mouse_z);
AL_VAR(int, _mouse_w);
AL_VAR(int, _mouse_b);
AL_VAR(int, _mouse_on);

AL_VAR(int, _mouse_installed);

AL_VAR(int, _mouse_type);
AL_VAR(BITMAP *, _mouse_screen);
AL_VAR(BITMAP *, _mouse_pointer);


/* various bits of timer stuff */
AL_FUNC(long, _handle_timer_tick, (int interval));

#define MAX_TIMERS      16

/* list of active timer handlers */
typedef struct TIMER_QUEUE
{
   AL_METHOD(void, proc, (void));      /* timer handler functions */
   AL_METHOD(void, param_proc, (void *param));
   void *param;                        /* param for param_proc if used */
   long speed;                         /* timer speed */
   long counter;                       /* counts down to zero=blastoff */
} TIMER_QUEUE;

AL_ARRAY(TIMER_QUEUE, _timer_queue);

AL_VAR(int, _timer_installed);

AL_VAR(int, _timer_use_retrace);
AL_VAR(volatile int, _retrace_hpp_value);

AL_VAR(long, _vsync_speed);


/* various bits of keyboard stuff */
AL_FUNC(void, _handle_key_press, (int keycode, int scancode));
AL_FUNC(void, _handle_key_release, (int scancode));

AL_VAR(int, _keyboard_installed);
AL_ARRAY(AL_CONST char *, _keyboard_common_names);
AL_ARRAY(volatile char, _key);
AL_VAR(volatile int, _key_shifts);


#if (defined ALLEGRO_WINDOWS)

   AL_FUNC(int, _al_win_open, (const char *filename, int mode, int perm));
   AL_FUNC(int, _al_win_unlink, (const char *filename));


   #define _al_open(filename, mode, perm)   _al_win_open(filename, mode, perm)
   #define _al_unlink(filename)             _al_win_unlink(filename)

#else

   #define _al_open(filename, mode, perm)   open(filename, mode, perm)
   #define _al_unlink(filename)             unlink(filename)

#endif


/* text- and font-related stuff */
typedef struct FONT_VTABLE
{
   AL_METHOD(int, font_height, (AL_CONST FONT *f));
   AL_METHOD(int, char_length, (AL_CONST FONT *f, int ch));
   AL_METHOD(int, text_length, (AL_CONST FONT *f, AL_CONST char *text));
   AL_METHOD(int, render_char, (AL_CONST FONT *f, int ch, int fg, int bg, BITMAP *bmp, int x, int y));
   AL_METHOD(void, render, (AL_CONST FONT *f, AL_CONST char *text, int fg, int bg, BITMAP *bmp, int x, int y));
   AL_METHOD(void, destroy, (FONT *f));

   AL_METHOD(int, get_font_ranges, (FONT *f));
   AL_METHOD(int, get_font_range_begin, (FONT *f, int range));
   AL_METHOD(int, get_font_range_end, (FONT *f, int range));
   AL_METHOD(FONT *, extract_font_range, (FONT *f, int begin, int end));
   AL_METHOD(FONT *, merge_fonts, (FONT *f1, FONT *f2));
   AL_METHOD(int, transpose_font, (FONT *f, int drange));
} FONT_VTABLE;

AL_VAR(FONT_VTABLE, _font_vtable_mono);
AL_VAR(FONT_VTABLE *, font_vtable_mono);
AL_VAR(FONT_VTABLE, _font_vtable_color);
AL_VAR(FONT_VTABLE *, font_vtable_color);
AL_VAR(FONT_VTABLE, _font_vtable_trans);
AL_VAR(FONT_VTABLE *, font_vtable_trans);

AL_FUNC(FONT_GLYPH *, _mono_find_glyph, (AL_CONST FONT *f, int ch));
AL_FUNC(BITMAP *, _color_find_glyph, (AL_CONST FONT *f, int ch));

typedef struct FONT_MONO_DATA
{
   int begin, end;                  /* first char and one-past-the-end char */
   FONT_GLYPH **glyphs;             /* our glyphs */
   struct FONT_MONO_DATA *next;     /* linked list structure */
} FONT_MONO_DATA;

typedef struct FONT_COLOR_DATA
{
   int begin, end;                  /* first char and one-past-the-end char */
   BITMAP **bitmaps;                /* our glyphs */
   struct FONT_COLOR_DATA *next;    /* linked list structure */
} FONT_COLOR_DATA;


/* caches and tables for svga bank switching */
AL_VAR(int, _last_bank_1);
AL_VAR(int, _last_bank_2);

AL_VAR(int *, _gfx_bank);

/* bank switching routines (these use a non-C calling convention on i386!) */
AL_FUNC(uintptr_t, _stub_bank_switch, (BITMAP *bmp, int lyne));
AL_FUNC(void, _stub_unbank_switch, (BITMAP *bmp));
AL_FUNC(void, _stub_bank_switch_end, (void));

#ifdef ALLEGRO_GFX_HAS_VGA

AL_FUNC(uintptr_t, _x_bank_switch, (BITMAP *bmp, int lyne));
AL_FUNC(void, _x_unbank_switch, (BITMAP *bmp));
AL_FUNC(void, _x_bank_switch_end, (void));

#endif

#ifdef ALLEGRO_GFX_HAS_VBEAF

AL_FUNC(void, _accel_bank_stub, (void));
AL_FUNC(void, _accel_bank_stub_end, (void));
AL_FUNC(void, _accel_bank_switch, (void));
AL_FUNC(void, _accel_bank_switch_end, (void));

AL_VAR(void *, _accel_driver);

AL_VAR(int, _accel_active);

AL_VAR(void *, _accel_set_bank);
AL_VAR(void *, _accel_idle);

AL_FUNC(void, _fill_vbeaf_libc_exports, (void *ptr));
AL_FUNC(void, _fill_vbeaf_pmode_exports, (void *ptr));

#endif


/* stuff for setting up bitmaps */
AL_FUNC(BITMAP *, _make_bitmap, (int w, int h, uintptr_t addr, GFX_DRIVER *driver, int color_depth, int bpl));
AL_FUNC(void, _sort_out_virtual_width, (int *width, GFX_DRIVER *driver));

AL_FUNC(GFX_VTABLE *, _get_vtable, (int color_depth));

AL_VAR(GFX_VTABLE, _screen_vtable);

AL_VAR(int, _gfx_mode_set_count);

AL_VAR(int, _refresh_rate_request);
AL_FUNC(void, _set_current_refresh_rate, (int rate));

AL_VAR(int, _wait_for_vsync);

AL_VAR(int, _sub_bitmap_id_count);

AL_VAR(int, _screen_split_position);

AL_VAR(int, _safe_gfx_mode_change);

#ifdef ALLEGRO_I386
   #define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)
#else
   #ifdef ALLEGRO_MPW
      /* in Mac 24 bit is a unsigned long */
      #define BYTES_PER_PIXEL(bpp)  (((bpp) <= 8) ? 1                                   \
                                     : (((bpp) <= 16) ? 2               \
                                        : 4))
   #else
      #define BYTES_PER_PIXEL(bpp)  (((bpp) <= 8) ? 1                                   \
                                     : (((bpp) <= 16) ? 2               \
                                        : (((bpp) <= 24) ? 3 : 4)))
   #endif
#endif

AL_FUNC(int, _color_load_depth, (int depth, int hasalpha));

AL_VAR(int, _color_conv);

AL_FUNC(BITMAP *, _fixup_loaded_bitmap, (BITMAP *bmp, PALETTE pal, int bpp));

AL_FUNC(int, _bitmap_has_alpha, (BITMAP *bmp));

/* default truecolor pixel format */
#define DEFAULT_RGB_R_SHIFT_15  0
#define DEFAULT_RGB_G_SHIFT_15  5
#define DEFAULT_RGB_B_SHIFT_15  10
#define DEFAULT_RGB_R_SHIFT_16  0
#define DEFAULT_RGB_G_SHIFT_16  5
#define DEFAULT_RGB_B_SHIFT_16  11
#define DEFAULT_RGB_R_SHIFT_24  0
#define DEFAULT_RGB_G_SHIFT_24  8
#define DEFAULT_RGB_B_SHIFT_24  16
#define DEFAULT_RGB_R_SHIFT_32  0
#define DEFAULT_RGB_G_SHIFT_32  8
#define DEFAULT_RGB_B_SHIFT_32  16
#define DEFAULT_RGB_A_SHIFT_32  24


/* display switching support */
AL_FUNC(void, _switch_in, (void));
AL_FUNC(void, _switch_out, (void));

AL_FUNC(void, _register_switch_bitmap, (BITMAP *bmp, BITMAP *parent));
AL_FUNC(void, _unregister_switch_bitmap, (BITMAP *bmp));
AL_FUNC(void, _save_switch_state, (int switch_mode));
AL_FUNC(void, _restore_switch_state, (void));

AL_VAR(int, _dispsw_status);


/* current drawing mode */
AL_VAR(ALLEGRO_TLS int, _drawing_mode);
AL_VAR(ALLEGRO_TLS BITMAP *, _drawing_pattern);
AL_VAR(ALLEGRO_TLS int, _drawing_x_anchor);
AL_VAR(ALLEGRO_TLS int, _drawing_y_anchor);
AL_VAR(ALLEGRO_TLS unsigned int, _drawing_x_mask);
AL_VAR(ALLEGRO_TLS unsigned int, _drawing_y_mask);

AL_FUNCPTR(int *, _palette_expansion_table, (int bpp));

AL_VAR(int, _color_depth);

AL_VAR(int, _current_palette_changed);
AL_VAR(PALETTE, _prev_current_palette);
AL_VAR(int, _got_prev_current_palette);

AL_ARRAY(int, _palette_color8);
AL_ARRAY(int, _palette_color15);
AL_ARRAY(int, _palette_color16);
AL_ARRAY(int, _palette_color24);
AL_ARRAY(int, _palette_color32);

/* truecolor blending functions */
AL_VAR(BLENDER_FUNC, _blender_func15);
AL_VAR(BLENDER_FUNC, _blender_func16);
AL_VAR(BLENDER_FUNC, _blender_func24);
AL_VAR(BLENDER_FUNC, _blender_func32);

AL_VAR(BLENDER_FUNC, _blender_func15x);
AL_VAR(BLENDER_FUNC, _blender_func16x);
AL_VAR(BLENDER_FUNC, _blender_func24x);

AL_VAR(int, _blender_col_15);
AL_VAR(int, _blender_col_16);
AL_VAR(int, _blender_col_24);
AL_VAR(int, _blender_col_32);

AL_VAR(int, _blender_alpha);

AL_FUNC(unsigned long, _blender_black, (unsigned long x, unsigned long y, unsigned long n));

#ifdef ALLEGRO_COLOR16

AL_FUNC(unsigned long, _blender_trans15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen15, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(unsigned long, _blender_trans16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen16, (unsigned long x, unsigned long y, unsigned long n));

#endif

#if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

AL_FUNC(unsigned long, _blender_trans24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen24, (unsigned long x, unsigned long y, unsigned long n));

#endif

AL_FUNC(unsigned long, _blender_alpha15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha32, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(unsigned long, _blender_write_alpha, (unsigned long x, unsigned long y, unsigned long n));


/* graphics drawing routines */
AL_FUNC(void, _normal_line, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
AL_FUNC(void, _fast_line, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
AL_FUNC(void, _normal_rectfill, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));

#ifdef ALLEGRO_COLOR8

AL_FUNC(int,  _linear_getpixel8, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel8, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline8, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline8, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex8, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_sprite_v_flip8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite8, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character8, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_draw_glyph8, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit8, (BITMAP *source,BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward8, (BITMAP *source,BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit8, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color8, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _linear_putpixel15, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline15, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline15, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_trans_sprite15, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite15, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite15, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));

AL_FUNC(int,  _linear_getpixel16, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel16, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline16, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline16, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex16, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_256_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character16, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_draw_glyph16, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit16, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward16, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit16, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color16, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(int,  _linear_getpixel24, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel24, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline24, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline24, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex24, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_256_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character24, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_draw_glyph24, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit24, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward24, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit24, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color24, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(int,  _linear_getpixel32, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel32, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline32, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline32, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex32, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_256_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character32, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_draw_glyph32, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit32, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward32, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit32, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color32, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_GFX_HAS_VGA

AL_FUNC(int,  _x_getpixel, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _x_putpixel, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _x_vline, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _x_hline, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _x_draw_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_sprite_h_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_sprite_vh_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_trans_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_lit_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _x_draw_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _x_draw_trans_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _x_draw_lit_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _x_draw_character, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _x_draw_glyph, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg));
AL_FUNC(void, _x_blit_from_memory, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit_to_memory, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit_forward, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit_backward, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_masked_blit, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_clear_to_color, (BITMAP *bitmap, int color));

#endif


/* color conversion routines */
typedef struct GRAPHICS_RECT {
   int width;
   int height;
   int pitch;
   void *data;
} GRAPHICS_RECT;

typedef void (COLORCONV_BLITTER_FUNC)(GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect);

AL_FUNC(COLORCONV_BLITTER_FUNC *, _get_colorconv_blitter, (int from_depth, int to_depth));
AL_FUNC(void, _release_colorconv_blitter, (COLORCONV_BLITTER_FUNC *blitter));
AL_FUNC(void, _set_colorconv_palette, (AL_CONST struct RGB *p, int from, int to));
AL_FUNC(unsigned char *, _get_colorconv_map, (void));

#ifdef ALLEGRO_COLOR8

AL_FUNC(void, _colorconv_blit_8_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _colorconv_blit_15_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_15_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_15_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_15_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

AL_FUNC(void, _colorconv_blit_16_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_16_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_16_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_16_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(void, _colorconv_blit_24_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_24_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_24_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_24_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(void, _colorconv_blit_32_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_32_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_32_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_32_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif


/* color copy routines */
#ifndef ALLEGRO_NO_COLORCOPY

#ifdef ALLEGRO_COLOR16
AL_FUNC(void, _colorcopy_blit_15_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorcopy_blit_16_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
#endif

#ifdef ALLEGRO_COLOR24
AL_FUNC(void, _colorcopy_blit_24_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
#endif

#ifdef ALLEGRO_COLOR32
AL_FUNC(void, _colorcopy_blit_32_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
#endif

#endif


/* generic color conversion blitter */
AL_FUNC(void, _blit_between_formats, (BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h));


/* asm helper for stretch_blit() */
#ifndef SCAN_EXPORT
AL_FUNC(void, _do_stretch, (BITMAP *source, BITMAP *dest, void *drawer, int sx, fixed sy, fixed syd, int dx, int dy, int dh, int color_depth));
#endif


/* lower level functions for rotation */
AL_FUNC(void, _parallelogram_map, (BITMAP *bmp, BITMAP *spr, fixed xs[4], fixed ys[4], void (*draw_scanline)(BITMAP *bmp, BITMAP *spr, fixed l_bmp_x, int bmp_y, fixed r_bmp_x, fixed l_spr_x, fixed l_spr_y, fixed spr_dx, fixed spr_dy), int sub_pixel_accuracy));
AL_FUNC(void, _parallelogram_map_standard, (BITMAP *bmp, BITMAP *sprite, fixed xs[4], fixed ys[4]));
AL_FUNC(void, _rotate_scale_flip_coordinates, (fixed w, fixed h, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale_x, fixed scale_y, int h_flip, int v_flip, fixed xs[4], fixed ys[4]));
AL_FUNC(void, _pivot_scaled_sprite_flip, (struct BITMAP *bmp, struct BITMAP *sprite, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale, int v_flip));


/* number of fractional bits used by the polygon rasteriser */
#define POLYGON_FIX_SHIFT     18


/* bitfield specifying which polygon attributes need interpolating */
#define INTERP_FLAT           1      /* no interpolation */
#define INTERP_1COL           2      /* gcol or alpha */
#define INTERP_3COL           4      /* grgb */
#define INTERP_FIX_UV         8      /* atex */
#define INTERP_Z              16     /* always in scene3d */
#define INTERP_FLOAT_UV       32     /* ptex */
#define OPT_FLOAT_UV_TO_FIX   64     /* translate ptex to atex */
#define COLOR_TO_RGB          128    /* grgb to gcol for truecolor */
#define INTERP_ZBUF           256    /* z-buffered */
#define INTERP_THRU           512    /* any kind of transparent */
#define INTERP_NOSOLID        1024   /* non-solid modes for 8-bit flat */
#define INTERP_BLEND          2048   /* lit for truecolor */
#define INTERP_TRANS          4096   /* trans for truecolor */


/* information for polygon scanline fillers */
typedef struct POLYGON_SEGMENT
{
   fixed u, v, du, dv;              /* fixed point u/v coordinates */
   fixed c, dc;                     /* single color gouraud shade values */
   fixed r, g, b, dr, dg, db;       /* RGB gouraud shade values */
   float z, dz;                     /* polygon depth (1/z) */
   float fu, fv, dfu, dfv;          /* floating point u/v coordinates */
   unsigned char *texture;          /* the texture map */
   int umask, vmask, vshift;        /* texture map size information */
   int seg;                         /* destination bitmap selector */
   uintptr_t zbuf_addr;             /* Z-buffer address */
   uintptr_t read_addr;             /* reading address for transparency modes */
} POLYGON_SEGMENT;


/* prototype for the scanline filler functions */
typedef AL_METHOD(void, SCANLINE_FILLER, (uintptr_t addr, int w, POLYGON_SEGMENT *info));


/* an active polygon edge */
typedef struct POLYGON_EDGE
{
   int top;                         /* top y position */
   int bottom;                      /* bottom y position */
   fixed x, dx;                     /* fixed point x position and gradient */
   fixed w;                         /* width of line segment */
   POLYGON_SEGMENT dat;             /* texture/gouraud information */
   struct POLYGON_EDGE *prev;       /* doubly linked list */
   struct POLYGON_EDGE *next;
   struct POLYGON_INFO *poly;       /* father polygon */
} POLYGON_EDGE;


typedef struct POLYGON_INFO         /* a polygon waiting rendering */
{
   struct POLYGON_INFO *next, *prev;/* double linked list */
   int inside;                      /* flag for "scanlining" */
   int flags;                       /* INTERP_* flags */
   int color;                       /* vtx[0]->c */
   float a, b, c;                   /* plane's coefficients -a/d, -b/d, -c/d */
   int dmode;                       /* drawing mode */
   BITMAP *dpat;                    /* drawing pattern */
   int xanchor, yanchor;            /* for dpat */
   int alpha;                       /* blender alpha */
   int b15, b16, b24, b32;          /* blender colors */
   COLOR_MAP *cmap;                 /* trans color map */
   SCANLINE_FILLER drawer;          /* scanline drawing functions */
   SCANLINE_FILLER alt_drawer;
   POLYGON_EDGE *left_edge;         /* true edges used in interpolation */
   POLYGON_EDGE *right_edge;
   POLYGON_SEGMENT info;            /* base information for scanline functions */
} POLYGON_INFO;


/* global variable for z-buffer */
AL_VAR(BITMAP *, _zbuffer);


/* polygon helper functions */
AL_VAR(SCANLINE_FILLER, _optim_alternative_drawer);
AL_FUNC(POLYGON_EDGE *, _add_edge, (POLYGON_EDGE *list, POLYGON_EDGE *edge, int sort_by_x));
AL_FUNC(POLYGON_EDGE *, _remove_edge, (POLYGON_EDGE *list, POLYGON_EDGE *edge));
AL_FUNC(int, _fill_3d_edge_structure, (POLYGON_EDGE *edge, AL_CONST V3D *v1, AL_CONST V3D *v2, int flags, BITMAP *bmp));
AL_FUNC(int, _fill_3d_edge_structure_f, (POLYGON_EDGE *edge, AL_CONST V3D_f *v1, AL_CONST V3D_f *v2, int flags, BITMAP *bmp));
AL_FUNC(SCANLINE_FILLER, _get_scanline_filler, (int type, int *flags, POLYGON_SEGMENT *info, BITMAP *texture, BITMAP *bmp));
AL_FUNC(void, _clip_polygon_segment, (POLYGON_SEGMENT *info, fixed gap, int flags));
AL_FUNC(void, _clip_polygon_segment_f, (POLYGON_SEGMENT *info, int gap, int flags));


/* polygon scanline filler functions */
AL_FUNC(void, _poly_scanline_dummy, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#ifdef ALLEGRO_COLOR8

AL_FUNC(void, _poly_scanline_gcol8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_grgb8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#ifndef SCAN_EXPORT
AL_FUNC(void, _poly_scanline_grgb8x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
#endif

AL_FUNC(void, _poly_zbuf_flat8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_gcol8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_grgb8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_lit8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_trans8, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _poly_scanline_grgb15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#ifndef SCAN_EXPORT
AL_FUNC(void, _poly_scanline_grgb15x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit15x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit15x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit15x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit15x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit15d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit15d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
#endif

AL_FUNC(void, _poly_zbuf_grgb15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_lit15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_trans15, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#ifndef SCAN_EXPORT
AL_FUNC(void, _poly_scanline_grgb16x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit16x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit16x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit16x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit16x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit16d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit16d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
#endif

AL_FUNC(void, _poly_zbuf_flat16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_grgb16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_lit16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_trans16, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(void, _poly_scanline_grgb24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#ifndef SCAN_EXPORT
AL_FUNC(void, _poly_scanline_grgb24x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit24x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit24x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit24x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit24x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit24d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit24d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
#endif

AL_FUNC(void, _poly_zbuf_flat24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_grgb24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_lit24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_trans24, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(void, _poly_scanline_grgb32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#ifndef SCAN_EXPORT
AL_FUNC(void, _poly_scanline_grgb32x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit32x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit32x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit32x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit32x, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit32d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit32d, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
#endif

AL_FUNC(void, _poly_zbuf_flat32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_grgb32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_lit32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_atex_mask_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_zbuf_ptex_mask_trans32, (uintptr_t addr, int w, POLYGON_SEGMENT *info));

#endif


/* datafile ID's for compatibility with the old datafile format */
#define V1_DAT_MAGIC             0x616C6C2EL

#define V1_DAT_DATA              0
#define V1_DAT_FONT              1
#define V1_DAT_BITMAP_16         2
#define V1_DAT_BITMAP_256        3
#define V1_DAT_SPRITE_16         4
#define V1_DAT_SPRITE_256        5
#define V1_DAT_PALETTE_16        6
#define V1_DAT_PALETTE_256       7
#define V1_DAT_FONT_8x8          8
#define V1_DAT_FONT_PROP         9
#define V1_DAT_BITMAP            10
#define V1_DAT_PALETTE           11
#define V1_DAT_RLE_SPRITE        14
#define V1_DAT_FLI               15
#define V1_DAT_C_SPRITE          16
#define V1_DAT_XC_SPRITE         17

#define OLD_FONT_SIZE            95
#define LESS_OLD_FONT_SIZE       224


/* datafile object loading functions */
AL_FUNC(void, _unload_datafile_object, (DATAFILE *dat));


/* information about a datafile object */
typedef struct DATAFILE_TYPE
{
   int type;
   AL_METHOD(void *, load, (PACKFILE *f, long size));
   AL_METHOD(void, destroy, (void *));
} DATAFILE_TYPE;


#define MAX_DATAFILE_TYPES    32

AL_ARRAY(DATAFILE_TYPE, _datafile_type);

AL_VAR(int, _compile_sprites);

AL_FUNC(void, _construct_datafile, (DATAFILE *data));


/* for readbmp.c */
AL_FUNC(void, _register_bitmap_file_type_init, (void));

/* for readfont.c */
AL_FUNC(void, _register_font_file_type_init, (void));


/* for module linking system; see comment in allegro.c */
struct _AL_LINKER_MOUSE
{
   AL_METHOD(void, set_mouse_etc, (void));
   AL_METHOD(void, show_mouse, (BITMAP *));
   BITMAP **mouse_screen_ptr;
};

AL_VAR(struct _AL_LINKER_MOUSE *, _al_linker_mouse);


/* dynamic driver lists */
AL_FUNC(_DRIVER_INFO *, _create_driver_list, (void));
AL_FUNC(void, _destroy_driver_list, (_DRIVER_INFO *drvlist));
AL_FUNC(void, _driver_list_append_driver, (_DRIVER_INFO **drvlist, int id, void *driver, int autodetect));
AL_FUNC(void, _driver_list_prepend_driver, (_DRIVER_INFO **drvlist, int id, void *driver, int autodetect));
AL_FUNC(void, _driver_list_append_list, (_DRIVER_INFO **drvlist, _DRIVER_INFO *srclist));


/* various libc stuff */
AL_FUNC(void *, _al_sane_realloc, (void *ptr, size_t size));
AL_FUNC(char *, _al_sane_strncpy, (char *dest, const char *src, size_t n));


#define _AL_RAND_MAX  0xFFFF
AL_FUNC(void, _al_srand, (int seed));
AL_FUNC(int, _al_rand, (void));


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef AINTERN_H */
