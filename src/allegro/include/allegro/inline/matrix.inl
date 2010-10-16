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
 *      Matrix math inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_MATRIX_INL
#define ALLEGRO_MATRIX_INL

#ifdef __cplusplus
   extern "C" {
#endif


#define CALC_ROW(n)     (fixmul(x, m->v[n][0]) +      \
                         fixmul(y, m->v[n][1]) +      \
                         fixmul(z, m->v[n][2]) +      \
                         m->t[n])

AL_INLINE(void, apply_matrix, (MATRIX *m, fixed x, fixed y, fixed z, fixed *xout, fixed *yout, fixed *zout),
{
   *xout = CALC_ROW(0);
   *yout = CALC_ROW(1);
   *zout = CALC_ROW(2);
})

#undef CALC_ROW


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_MATRIX_INL */


