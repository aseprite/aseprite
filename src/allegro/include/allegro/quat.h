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
 *      Quaternion routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_QUAT_H
#define ALLEGRO_QUAT_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct QUAT
{
     float w, x, y, z;
} QUAT;


AL_VAR(QUAT, identity_quat);

AL_FUNC(void, quat_mul, (AL_CONST QUAT *p, AL_CONST QUAT *q, QUAT *out));
AL_FUNC(void, get_x_rotate_quat, (QUAT *q, float r));
AL_FUNC(void, get_y_rotate_quat, (QUAT *q, float r));
AL_FUNC(void, get_z_rotate_quat, (QUAT *q, float r));
AL_FUNC(void, get_rotation_quat, (QUAT *q, float x, float y, float z));
AL_FUNC(void, get_vector_rotation_quat, (QUAT *q, float x, float y, float z, float a));

AL_FUNC(void, apply_quat, (AL_CONST QUAT *q, float x, float y, float z, float *xout, float *yout, float *zout));
AL_FUNC(void, quat_slerp, (AL_CONST QUAT *from, AL_CONST QUAT *to, float t, QUAT *out, int how));

#define QUAT_SHORT   0
#define QUAT_LONG    1
#define QUAT_CW      2
#define QUAT_CCW     3
#define QUAT_USER    4

#define quat_interpolate(from, to, t, out)   quat_slerp((from), (to), (t), (out), QUAT_SHORT)

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_QUAT_H */


