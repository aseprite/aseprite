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
 *      3D oriented math routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_3DMATHS_H
#define ALLEGRO_3DMATHS_H

#include "base.h"
#include "fixed.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct QUAT;
struct MATRIX_f;

AL_FUNC(fixed, vector_length, (fixed x, fixed y, fixed z));
AL_FUNC(float, vector_length_f, (float x, float y, float z));

AL_FUNC(void, normalize_vector, (fixed *x, fixed *y, fixed *z));
AL_FUNC(void, normalize_vector_f, (float *x, float *y, float *z));

AL_FUNC(void, cross_product, (fixed x1, fixed y_1, fixed z1, fixed x2, fixed y2, fixed z2, fixed *xout, fixed *yout, fixed *zout));
AL_FUNC(void, cross_product_f, (float x1, float y_1, float z1, float x2, float y2, float z2, float *xout, float *yout, float *zout));

AL_VAR(fixed, _persp_xscale);
AL_VAR(fixed, _persp_yscale);
AL_VAR(fixed, _persp_xoffset);
AL_VAR(fixed, _persp_yoffset);

AL_VAR(float, _persp_xscale_f);
AL_VAR(float, _persp_yscale_f);
AL_VAR(float, _persp_xoffset_f);
AL_VAR(float, _persp_yoffset_f);

AL_FUNC(void, set_projection_viewport, (int x, int y, int w, int h));

AL_FUNC(void, quat_to_matrix, (AL_CONST struct QUAT *q, struct MATRIX_f *m));
AL_FUNC(void, matrix_to_quat, (AL_CONST struct MATRIX_f *m, struct QUAT *q));

#ifdef __cplusplus
   }
#endif

#include "inline/3dmaths.inl"

#endif          /* ifndef ALLEGRO_3DMATHS_H */


