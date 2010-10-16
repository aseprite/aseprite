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
 *      3D maths inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_3DMATHS_INL
#define ALLEGRO_3DMATHS_INL

#ifdef __cplusplus
   extern "C" {
#endif


AL_INLINE(fixed, dot_product, (fixed x1, fixed y_1, fixed z1, fixed x2, fixed y2, fixed z2),
{
   return fixmul(x1, x2) + fixmul(y_1, y2) + fixmul(z1, z2);
})


AL_INLINE(float, dot_product_f, (float x1, float y_1, float z1, float x2, float y2, float z2),
{
   return (x1 * x2) + (y_1 * y2) + (z1 * z2);
})


AL_INLINE(void, persp_project, (fixed x, fixed y, fixed z, fixed *xout, fixed *yout),
{
   *xout = fixmul(fixdiv(x, z), _persp_xscale) + _persp_xoffset;
   *yout = fixmul(fixdiv(y, z), _persp_yscale) + _persp_yoffset;
})


AL_INLINE(void, persp_project_f, (float x, float y, float z, float *xout, float *yout),
{
   float z1 = 1.0f / z;
   *xout = ((x * z1) * _persp_xscale_f) + _persp_xoffset_f;
   *yout = ((y * z1) * _persp_yscale_f) + _persp_yoffset_f;
})


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_3DMATHS_INL */


