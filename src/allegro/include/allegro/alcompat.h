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
 *      Backward compatibility stuff.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_COMPAT_H
#define ALLEGRO_COMPAT_H

#ifdef __cplusplus
   extern "C" {
#endif


#ifndef ALLEGRO_SRC

   #ifndef ALLEGRO_NO_CLEAR_BITMAP_ALIAS
      #if (defined ALLEGRO_GCC)
         static __attribute__((unused)) __inline__ void clear(BITMAP *bmp)
         {
            clear_bitmap(bmp);
         }
      #else
         static INLINE void clear(BITMAP *bmp)
         {
            clear_bitmap(bmp);
         }
      #endif
   #endif

   #ifndef ALLEGRO_NO_FIX_ALIASES
      AL_ALIAS(fixed fadd(fixed x, fixed y), fixadd(x, y))
      AL_ALIAS(fixed fsub(fixed x, fixed y), fixsub(x, y))
      AL_ALIAS(fixed fmul(fixed x, fixed y), fixmul(x, y))
      AL_ALIAS(fixed fdiv(fixed x, fixed y), fixdiv(x, y))
      AL_ALIAS(int fceil(fixed x), fixceil(x))
      AL_ALIAS(int ffloor(fixed x), fixfloor(x))
      AL_ALIAS(fixed fcos(fixed x), fixcos(x))
      AL_ALIAS(fixed fsin(fixed x), fixsin(x))
      AL_ALIAS(fixed ftan(fixed x), fixtan(x))
      AL_ALIAS(fixed facos(fixed x), fixacos(x))
      AL_ALIAS(fixed fasin(fixed x), fixasin(x))
      AL_ALIAS(fixed fatan(fixed x), fixatan(x))
      AL_ALIAS(fixed fatan2(fixed y, fixed x), fixatan2(y, x))
      AL_ALIAS(fixed fsqrt(fixed x), fixsqrt(x))
      AL_ALIAS(fixed fhypot(fixed x, fixed y), fixhypot(x, y))
   #endif

#endif  /* !defined ALLEGRO_SRC */


#define KB_NORMAL       1
#define KB_EXTENDED     2

#define SEND_MESSAGE    object_message

#define cpu_fpu         (cpu_capabilities & CPU_FPU)
#define cpu_mmx         (cpu_capabilities & CPU_MMX)
#define cpu_3dnow       (cpu_capabilities & CPU_3DNOW)
#define cpu_cpuid       (cpu_capabilities & CPU_ID)

#define joy_x           (joy[0].stick[0].axis[0].pos)
#define joy_y           (joy[0].stick[0].axis[1].pos)
#define joy_left        (joy[0].stick[0].axis[0].d1)
#define joy_right       (joy[0].stick[0].axis[0].d2)
#define joy_up          (joy[0].stick[0].axis[1].d1)
#define joy_down        (joy[0].stick[0].axis[1].d2)
#define joy_b1          (joy[0].button[0].b)
#define joy_b2          (joy[0].button[1].b)
#define joy_b3          (joy[0].button[2].b)
#define joy_b4          (joy[0].button[3].b)
#define joy_b5          (joy[0].button[4].b)
#define joy_b6          (joy[0].button[5].b)
#define joy_b7          (joy[0].button[6].b)
#define joy_b8          (joy[0].button[7].b)

#define joy2_x          (joy[1].stick[0].axis[0].pos)
#define joy2_y          (joy[1].stick[0].axis[1].pos)
#define joy2_left       (joy[1].stick[0].axis[0].d1)
#define joy2_right      (joy[1].stick[0].axis[0].d2)
#define joy2_up         (joy[1].stick[0].axis[1].d1)
#define joy2_down       (joy[1].stick[0].axis[1].d2)
#define joy2_b1         (joy[1].button[0].b)
#define joy2_b2         (joy[1].button[1].b)

#define joy_throttle    (joy[0].stick[2].axis[0].pos)

#define joy_hat         ((joy[0].stick[1].axis[0].d1) ? 1 :             \
                           ((joy[0].stick[1].axis[0].d2) ? 3 :          \
                              ((joy[0].stick[1].axis[1].d1) ? 4 :       \
                                 ((joy[0].stick[1].axis[1].d2) ? 2 :    \
                                    0))))

#define JOY_HAT_CENTRE        0
#define JOY_HAT_CENTER        0
#define JOY_HAT_LEFT          1
#define JOY_HAT_DOWN          2
#define JOY_HAT_RIGHT         3
#define JOY_HAT_UP            4

AL_FUNC_DEPRECATED(int, initialise_joystick, (void));


/* in case you want to spell 'palette' as 'pallete' */
#define PALLETE                        PALETTE
#define black_pallete                  black_palette
#define desktop_pallete                desktop_palette
#define set_pallete                    set_palette
#define get_pallete                    get_palette
#define set_pallete_range              set_palette_range
#define get_pallete_range              get_palette_range
#define fli_pallete                    fli_palette
#define pallete_color                  palette_color
#define DAT_PALLETE                    DAT_PALETTE
#define select_pallete                 select_palette
#define unselect_pallete               unselect_palette
#define generate_332_pallete           generate_332_palette
#define generate_optimised_pallete     generate_optimised_palette


/* a pretty vague name */
#define fix_filename_path              canonicalize_filename


/* the good old file selector */
#define OLD_FILESEL_WIDTH   -1
#define OLD_FILESEL_HEIGHT  -1

AL_INLINE_DEPRECATED(int, file_select, (AL_CONST char *message, char *path, AL_CONST char *ext),
{
   return file_select_ex(message, path, ext, 1024, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT);
})


/* the old (and broken!) file enumeration function */
AL_FUNC_DEPRECATED(int, for_each_file, (AL_CONST char *name, int attrib, AL_METHOD(void, callback, (AL_CONST char *filename, int attrib, int param)), int param));
/* long is 32-bit only on some systems, and we want to list DVDs! */
AL_FUNC_DEPRECATED(long, file_size, (AL_CONST char *filename));


/* the old state-based textout functions */
AL_VAR(int, _textmode);
AL_FUNC_DEPRECATED(int, text_mode, (int mode));

AL_INLINE_DEPRECATED(void, textout, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color),
{
   textout_ex(bmp, f, str, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(void, textout_centre, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color),
{
   textout_centre_ex(bmp, f, str, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(void, textout_right, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color),
{
   textout_right_ex(bmp, f, str, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(void, textout_justify, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color),
{
   textout_justify_ex(bmp, f, str, x1, x2, y, diff, color, _textmode);
})

AL_PRINTFUNC_DEPRECATED(void, textprintf, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC_DEPRECATED(void, textprintf_centre, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC_DEPRECATED(void, textprintf_right, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC_DEPRECATED(void, textprintf_justify, (struct BITMAP *bmp, AL_CONST FONT *f, int x1, int x2, int y, int diff, int color, AL_CONST char *format, ...), 8, 9);

AL_INLINE_DEPRECATED(void, draw_character, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{
   draw_character_ex(bmp, sprite, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(int, gui_textout, (struct BITMAP *bmp, AL_CONST char *s, int x, int y, int color, int centre),
{
   return gui_textout_ex(bmp, s, x, y, color, _textmode, centre);
})


/* the old close button functions */
AL_INLINE_DEPRECATED(int, set_window_close_button, (int enable),
{
   (void)enable;
   return 0;
})

AL_INLINE_DEPRECATED(void, set_window_close_hook, (void (*proc)(void)),
{
   set_close_button_callback(proc);
})


/* the weird old clipping API */
AL_FUNC_DEPRECATED(void, set_clip, (BITMAP *bitmap, int x1, int y_1, int x2, int y2));


/* unnecessary, can use rest(0) */
AL_INLINE_DEPRECATED(void, yield_timeslice, (void),
{
   ASSERT(system_driver);

   if (system_driver->yield_timeslice)
      system_driver->yield_timeslice();
})


/* DOS-ish monitor retrace ideas that don't work elsewhere */
AL_FUNCPTR(void, retrace_proc, (void));


/* Those were never documented, but we need to keep them for DLL compatibility,
 * and to be on the safe side also let's keep them work regardless.
 */
AL_INLINE_DEPRECATED(void, set_file_encoding, (int encoding),
{
   set_filename_encoding(encoding);
})
AL_INLINE_DEPRECATED(int, get_file_encoding, (void),
{
   return get_filename_encoding();
})


#ifdef ALLEGRO_SRC
   AL_FUNC(int,  timer_can_simulate_retrace, (void));
   AL_FUNC(void, timer_simulate_retrace, (int enable));
#else
   AL_FUNC_DEPRECATED(int,  timer_can_simulate_retrace, (void));
   AL_FUNC_DEPRECATED(void, timer_simulate_retrace, (int enable));
#endif
AL_FUNC_DEPRECATED(int,  timer_is_using_retrace, (void));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_COMPAT_H */

