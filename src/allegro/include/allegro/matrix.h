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
 *      Matrix math routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_MATRIX_H
#define ALLEGRO_MATRIX_H

#include "base.h"
#include "fixed.h"
#include "fmaths.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct MATRIX            /* transformation matrix (fixed point) */
{
   fixed v[3][3];                /* scaling and rotation */
   fixed t[3];                   /* translation */
} MATRIX;


typedef struct MATRIX_f          /* transformation matrix (floating point) */
{
   float v[3][3];                /* scaling and rotation */
   float t[3];                   /* translation */
} MATRIX_f;


AL_VAR(MATRIX, identity_matrix);
AL_VAR(MATRIX_f, identity_matrix_f);

AL_FUNC(void, get_translation_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, get_translation_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, get_scaling_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, get_scaling_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, get_x_rotate_matrix, (MATRIX *m, fixed r));
AL_FUNC(void, get_x_rotate_matrix_f, (MATRIX_f *m, float r));

AL_FUNC(void, get_y_rotate_matrix, (MATRIX *m, fixed r));
AL_FUNC(void, get_y_rotate_matrix_f, (MATRIX_f *m, float r));

AL_FUNC(void, get_z_rotate_matrix, (MATRIX *m, fixed r));
AL_FUNC(void, get_z_rotate_matrix_f, (MATRIX_f *m, float r));

AL_FUNC(void, get_rotation_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, get_rotation_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, get_align_matrix, (MATRIX *m, fixed xfront, fixed yfront, fixed zfront, fixed xup, fixed yup, fixed zup));
AL_FUNC(void, get_align_matrix_f, (MATRIX_f *m, float xfront, float yfront, float zfront, float xup, float yup, float zup));

AL_FUNC(void, get_vector_rotation_matrix, (MATRIX *m, fixed x, fixed y, fixed z, fixed a));
AL_FUNC(void, get_vector_rotation_matrix_f, (MATRIX_f *m, float x, float y, float z, float a));

AL_FUNC(void, get_transformation_matrix, (MATRIX *m, fixed scale, fixed xrot, fixed yrot, fixed zrot, fixed x, fixed y, fixed z));
AL_FUNC(void, get_transformation_matrix_f, (MATRIX_f *m, float scale, float xrot, float yrot, float zrot, float x, float y, float z));

AL_FUNC(void, get_camera_matrix, (MATRIX *m, fixed x, fixed y, fixed z, fixed xfront, fixed yfront, fixed zfront, fixed xup, fixed yup, fixed zup, fixed fov, fixed aspect));
AL_FUNC(void, get_camera_matrix_f, (MATRIX_f *m, float x, float y, float z, float xfront, float yfront, float zfront, float xup, float yup, float zup, float fov, float aspect));

AL_FUNC(void, qtranslate_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, qtranslate_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, qscale_matrix, (MATRIX *m, fixed scale));
AL_FUNC(void, qscale_matrix_f, (MATRIX_f *m, float scale));

AL_FUNC(void, matrix_mul, (AL_CONST MATRIX *m1, AL_CONST MATRIX *m2, MATRIX *out));
AL_FUNC(void, matrix_mul_f, (AL_CONST MATRIX_f *m1, AL_CONST MATRIX_f *m2, MATRIX_f *out));

AL_FUNC(void, apply_matrix_f, (AL_CONST MATRIX_f *m, float x, float y, float z, float *xout, float *yout, float *zout));

#ifdef __cplusplus
   }
#endif

#include "inline/matrix.inl"

#endif          /* ifndef ALLEGRO_MATRIX_H */


