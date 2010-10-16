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
 *      Color inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_COLOR_INL
#define ALLEGRO_COLOR_INL

#ifdef __cplusplus
   extern "C" {
#endif

AL_INLINE(int, makecol15, (int r, int g, int b),
{
   return (((r >> 3) << _rgb_r_shift_15) |
           ((g >> 3) << _rgb_g_shift_15) |
           ((b >> 3) << _rgb_b_shift_15));
})


AL_INLINE(int, makecol16, (int r, int g, int b),
{
   return (((r >> 3) << _rgb_r_shift_16) |
           ((g >> 2) << _rgb_g_shift_16) |
           ((b >> 3) << _rgb_b_shift_16));
})


AL_INLINE(int, makecol24, (int r, int g, int b),
{
   return ((r << _rgb_r_shift_24) |
           (g << _rgb_g_shift_24) |
           (b << _rgb_b_shift_24));
})


AL_INLINE(int, makecol32, (int r, int g, int b),
{
   return ((r << _rgb_r_shift_32) |
           (g << _rgb_g_shift_32) |
           (b << _rgb_b_shift_32));
})


AL_INLINE(int, makeacol32, (int r, int g, int b, int a),
{
   return ((r << _rgb_r_shift_32) |
           (g << _rgb_g_shift_32) |
           (b << _rgb_b_shift_32) |
           (a << _rgb_a_shift_32));
})


AL_INLINE(int, getr8, (int c),
{
   return _rgb_scale_6[(int)_current_palette[c].r];
})


AL_INLINE(int, getg8, (int c),
{
   return _rgb_scale_6[(int)_current_palette[c].g];
})


AL_INLINE(int, getb8, (int c),
{
   return _rgb_scale_6[(int)_current_palette[c].b];
})


AL_INLINE(int, getr15, (int c),
{
   return _rgb_scale_5[(c >> _rgb_r_shift_15) & 0x1F];
})


AL_INLINE(int, getg15, (int c),
{
   return _rgb_scale_5[(c >> _rgb_g_shift_15) & 0x1F];
})


AL_INLINE(int, getb15, (int c),
{
   return _rgb_scale_5[(c >> _rgb_b_shift_15) & 0x1F];
})


AL_INLINE(int, getr16, (int c),
{
   return _rgb_scale_5[(c >> _rgb_r_shift_16) & 0x1F];
})


AL_INLINE(int, getg16, (int c),
{
   return _rgb_scale_6[(c >> _rgb_g_shift_16) & 0x3F];
})


AL_INLINE(int, getb16, (int c),
{
   return _rgb_scale_5[(c >> _rgb_b_shift_16) & 0x1F];
})


AL_INLINE(int, getr24, (int c),
{
   return ((c >> _rgb_r_shift_24) & 0xFF);
})


AL_INLINE(int, getg24, (int c),
{
   return ((c >> _rgb_g_shift_24) & 0xFF);
})


AL_INLINE(int, getb24, (int c),
{
   return ((c >> _rgb_b_shift_24) & 0xFF);
})


AL_INLINE(int, getr32, (int c),
{
   return ((c >> _rgb_r_shift_32) & 0xFF);
})


AL_INLINE(int, getg32, (int c),
{
   return ((c >> _rgb_g_shift_32) & 0xFF);
})


AL_INLINE(int, getb32, (int c),
{
   return ((c >> _rgb_b_shift_32) & 0xFF);
})


AL_INLINE(int, geta32, (int c),
{
   return ((c >> _rgb_a_shift_32) & 0xFF);
})


#ifndef ALLEGRO_DOS

AL_INLINE(void, _set_color, (int idx, AL_CONST RGB *p),
{
   set_color(idx, p);
})

#endif


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_COLOR_INL */


