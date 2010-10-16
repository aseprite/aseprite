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
 *      Quaternion manipulation routines.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */


#include <math.h>

#include "allegro.h"



#define FLOATSINCOS(x, s, c)  _AL_SINCOS((x) * AL_PI / 128.0, s ,c)


#define EPSILON (0.001)



QUAT identity_quat = { 1.0, 0.0, 0.0, 0.0 };



/* quat_mul:
 *  Multiplies two quaternions, storing the result in out. The resulting
 *  quaternion will have the same effect as the combination of p and q,
 *  ie. when applied to a point, (point * out) = ((point * p) * q). Any
 *  number of rotations can be concatenated in this way. Note that quaternion
 *  multiplication is not commutative, ie. quat_mul(p, q) != quat_mul(q, p).
 */
void quat_mul(AL_CONST QUAT *p, AL_CONST QUAT *q, QUAT *out)
{
   QUAT temp;
   ASSERT(p);
   ASSERT(q);
   ASSERT(out);

   /* qp = ww' - v.v' + vxv' + wv' + w'v */

   if (p == out) {
      temp = *p;
      p = &temp;
   }
   else if (q == out) {
      temp = *q;
      q = &temp;
   }

   /* w" = ww' - xx' - yy' - zz' */
   out->w = (p->w * q->w) -
	    (p->x * q->x) -
	    (p->y * q->y) -
	    (p->z * q->z);

   /* x" = wx' + xw' + yz' - zy' */
   out->x = (p->w * q->x) +
	    (p->x * q->w) +
	    (p->y * q->z) -
	    (p->z * q->y);

   /* y" = wy' + yw' + zx' - xz' */
   out->y = (p->w * q->y) +
	    (p->y * q->w) +
	    (p->z * q->x) -
	    (p->x * q->z);

   /* z" = wz' + zw' + xy' - yx' */
   out->z = (p->w * q->z) +
	    (p->z * q->w) +
	    (p->x * q->y) -
	    (p->y * q->x);
}



/* get_x_rotate_quat:
 *  Construct X axis rotation quaternions, storing them in q. When applied to
 *  a point, these quaternions will rotate it about the X axis by the
 *  specified angle (given in binary, 256 degrees to a circle format).
 */
void get_x_rotate_quat(QUAT *q, float r)
{
   ASSERT(q);

   FLOATSINCOS(r/2, q->x, q->w);
   q->y = 0;
   q->z = 0;
}



/* get_y_rotate_quat:
 *  Construct Y axis rotation quaternions, storing them in q. When applied to
 *  a point, these quaternions will rotate it about the Y axis by the
 *  specified angle (given in binary, 256 degrees to a circle format).
 */
void get_y_rotate_quat(QUAT *q, float r)
{
   ASSERT(q);

   FLOATSINCOS(r/2, q->y, q->w);
   q->x = 0;
   q->z = 0;
}



/* get_z_rotate_quat:
 *  Construct Z axis rotation quaternions, storing them in q. When applied to
 *  a point, these quaternions will rotate it about the Z axis by the
 *  specified angle (given in binary, 256 degrees to a circle format).
 */
void get_z_rotate_quat(QUAT *q, float r)
{
   ASSERT(q);

   FLOATSINCOS(r/2, q->z, q->w);
   q->x = 0;
   q->y = 0;
}



/* get_rotation_quat:
 *  Constructs a quaternion which will rotate points around all three axis by
 *  the specified amounts (given in binary, 256 degrees to a circle format).
 */
void get_rotation_quat(QUAT *q, float x, float y, float z)
{
   float sx;
   float sy;
   float sz;
   float cx;
   float cy;
   float cz;
   float cycz;
   float sysz;

   ASSERT(q);

   FLOATSINCOS(x/2, sx, cx);
   FLOATSINCOS(y/2, sy, cy);
   FLOATSINCOS(z/2, sz, cz);

   sysz = sy * sz;
   cycz = cy * cz;

   q->w = (cx * cycz) + (sx * sysz);
   q->x = (sx * cycz) - (cx * sysz);
   q->y = (cx * sy * cz) + (sx * cy * sz);
   q->z = (cx * cy * sz) - (sx * sy * cz);
}



/* get_vector_rotation_quat:
 *  Constructs a quaternion which will rotate points around the specified
 *  x,y,z vector by the specified angle (given in binary, 256 degrees to a
 *  circle format).
 */
void get_vector_rotation_quat(QUAT *q, float x, float y, float z, float a)
{
   float l;
   float s;

   ASSERT(q);

   l = vector_length_f(x, y, z);

   /* NOTE: Passing (x, y, z) = (0, 0, 0) will cause l to equal 0 which will
    *       cause a divide-by-zero exception. Rotating something about the
    *       zero vector undefined.
    */
   ASSERT(l != 0);

   x /= l;
   y /= l;
   z /= l;

   FLOATSINCOS(a/2, s, q->w);
   q->x = s * x;
   q->y = s * y;
   q->z = s * z;
}



/* quat_to_matrix:
 * Constructs a rotation matrix from a quaternion.
 */
void quat_to_matrix(AL_CONST QUAT *q, MATRIX_f *m)
{
   float ww;
   float xx;
   float yy;
   float zz;
   float wx;
   float wy;
   float wz;
   float xy;
   float xz;
   float yz;
   ASSERT(q);
   ASSERT(m);

  /* This is the layout for converting the values in a quaternion to a
   * matrix.
   *
   *  | ww + xx - yy - zz       2xy + 2wz             2xz - 2wy     |
   *  |     2xy - 2wz       ww - xx + yy - zz         2yz - 2wx     |
   *  |     2xz + 2wy           2yz - 2wx         ww + xx - yy - zz |
   */

   ww = q->w * q->w;
   xx = q->x * q->x;
   yy = q->y * q->y;
   zz = q->z * q->z;
   wx = q->w * q->x * 2;
   wy = q->w * q->y * 2;
   wz = q->w * q->z * 2;
   xy = q->x * q->y * 2;
   xz = q->x * q->z * 2;
   yz = q->y * q->z * 2;

   m->v[0][0] = ww + xx - yy - zz;
   m->v[1][0] = xy - wz;
   m->v[2][0] = xz + wy;

   m->v[0][1] = xy + wz;
   m->v[1][1] = ww - xx + yy - zz;
   m->v[2][1] = yz - wx;

   m->v[0][2] = xz - wy;
   m->v[1][2] = yz + wx;
   m->v[2][2] = ww - xx - yy + zz;

   m->t[0] = 0.0;
   m->t[1] = 0.0;
   m->t[2] = 0.0;
}



/* matrix_to_quat:
 *  Constructs a quaternion from a rotation matrix. Translation is discarded
 *  during the conversion. Use get_align_matrix_f if the matrix is not
 *  orthonormalized, because strange things may happen otherwise.
 */
void matrix_to_quat(AL_CONST MATRIX_f *m, QUAT *q)
{
   float trace = m->v[0][0] + m->v[1][1] + m->v[2][2] + 1.0f;

   if (trace > EPSILON) {
      float s = 0.5f / (float)sqrt(trace);
      q->w = 0.25f / s;
      q->x = (m->v[2][1] - m->v[1][2]) * s;
      q->y = (m->v[0][2] - m->v[2][0]) * s;
      q->z = (m->v[1][0] - m->v[0][1]) * s;
   }
   else {
      if (m->v[0][0] > m->v[1][1] && m->v[0][0] > m->v[2][2]) {
         float s = 2.0f * (float)sqrt(1.0f + m->v[0][0] - m->v[1][1] - m->v[2][2]);
         q->x = 0.25f * s;
         q->y = (m->v[0][1] + m->v[1][0]) / s;
         q->z = (m->v[0][2] + m->v[2][0]) / s;
         q->w = (m->v[1][2] - m->v[2][1]) / s;
      }
      else if (m->v[1][1] > m->v[2][2]) {
         float s = 2.0f * (float)sqrt(1.0f + m->v[1][1] - m->v[0][0] - m->v[2][2]);
         q->x = (m->v[0][1] + m->v[1][0]) / s;
         q->y = 0.25f * s;
         q->z = (m->v[1][2] + m->v[2][1]) / s;
         q->w = (m->v[0][2] - m->v[2][0]) / s;
      }
      else {
         float s = 2.0f * (float)sqrt(1.0f + m->v[2][2] - m->v[0][0] - m->v[1][1]);
         q->x = (m->v[0][2] + m->v[2][0]) / s;
         q->y = (m->v[1][2] + m->v[2][1]) / s;
         q->z = 0.25f * s;
         q->w = (m->v[0][1] - m->v[1][0]) / s;
      }
   }
}



/* quat_conjugate:
 *  A quaternion conjugate is analogous to a complex number conjugate, just
 *  negate the imaginary part
 */
static INLINE void quat_conjugate(AL_CONST QUAT *q, QUAT *out)
{
   ASSERT(q);
   ASSERT(out);

   /* q^* = w - x - y - z */
   out->w =  (q->w);
   out->x = -(q->x);
   out->y = -(q->y);
   out->z = -(q->z);
}



/* quat_normal:
 *  A quaternion normal is the sum of the squares of the components.
 */
static INLINE float quat_normal(AL_CONST QUAT *q)
{
   ASSERT(q);

   /* N(q) = ww + xx + yy + zz */
   return ((q->w * q->w) + (q->x * q->x) + (q->y * q->y) + (q->z * q->z));
}



/* quat_inverse:
 *  A quaternion inverse is the conjugate divided by the normal.
 */
static INLINE void quat_inverse(AL_CONST QUAT *q, QUAT *out)
{
   QUAT  con;
   float norm;
   ASSERT(q);
   ASSERT(out);

   /* q^-1 = q^* / N(q) */

   quat_conjugate(q, &con);
   norm = quat_normal(q);

   /* NOTE: If the normal is 0 then a divide-by-zero exception will occur, we
    *       let this happen because the inverse of a zero quaternion is
    *       undefined
    */
   ASSERT(norm != 0);

   out->w = con.w / norm;
   out->x = con.x / norm;
   out->y = con.y / norm;
   out->z = con.z / norm;
}



/* apply_quat:
 *  Multiplies the point (x, y, z) by the quaternion q, storing the result in
 *  (*xout, *yout, *zout). This is quite a bit slower than apply_matrix_f.
 *  So, only use it if you only need to translate a handful of points.
 *  Otherwise it is much more efficient to call quat_to_matrix then use
 *  apply_matrix_f.
 */
void apply_quat(AL_CONST QUAT *q, float x, float y, float z, float *xout, float *yout, float *zout)
{
   QUAT v;
   QUAT i;
   QUAT t;
   ASSERT(q);
   ASSERT(xout);
   ASSERT(yout);
   ASSERT(zout);

   /* v' = q * v * q^-1 */

   v.w = 0;
   v.x = x;
   v.y = y;
   v.z = z;

   /* NOTE: Rotating about a zero quaternion is undefined. This can be shown
    *       by the fact that the inverse of a zero quaternion is undefined
    *       and therefore causes an exception below.
    */
   ASSERT(!((q->x == 0.0) && (q->y == 0.0) && (q->z == 0.0) && (q->w == 0.0)));

   quat_inverse(q, &i);
   quat_mul(&i, &v, &t);
   quat_mul(&t,  q, &v);

   *xout = v.x;
   *yout = v.y;
   *zout = v.z;
}



/* quat_slerp:
 *  Constructs a quaternion that represents a rotation between 'from' and
 *  'to'. The argument 't' can be anything between 0 and 1 and represents
 *  where between from and to the result will be.  0 returns 'from', 1
 *  returns 'to', and 0.5 will return a rotation exactly in between. The
 *  result is copied to 'out'.
 *
 *  The variable 'how' can be any one of the following:
 *
 *      QUAT_SHORT - This equivalent quat_interpolate, the rotation will
 *                   be less than 180 degrees
 *      QUAT_LONG  - rotation will be greater than 180 degrees
 *      QUAT_CW    - rotation will be clockwise when viewed from above
 *      QUAT_CCW   - rotation will be counterclockwise when viewed
 *                   from above
 *      QUAT_USER  - the quaternions are interpolated exactly as given
 */
void quat_slerp(AL_CONST QUAT *from, AL_CONST QUAT *to, float t, QUAT *out, int how)
{
   QUAT   to2;
   double angle;
   double cos_angle;
   double scale_from;
   double scale_to;
   double sin_angle;
   ASSERT(from);
   ASSERT(to);
   ASSERT(out);

   cos_angle = (from->x * to->x) +
	       (from->y * to->y) +
	       (from->z * to->z) +
	       (from->w * to->w);

   if (((how == QUAT_SHORT) && (cos_angle < 0.0)) ||
       ((how == QUAT_LONG)  && (cos_angle > 0.0)) ||
       ((how == QUAT_CW)    && (from->w > to->w)) ||
       ((how == QUAT_CCW)   && (from->w < to->w))) {
      cos_angle = -cos_angle;
      to2.w     = -to->w;
      to2.x     = -to->x;
      to2.y     = -to->y;
      to2.z     = -to->z;
   }
   else {
      to2.w = to->w;
      to2.x = to->x;
      to2.y = to->y;
      to2.z = to->z;
   }

   if ((1.0 - ABS(cos_angle)) > EPSILON) {
      /* spherical linear interpolation (SLERP) */
      angle = acos(cos_angle);
      sin_angle  = sin(angle);
      scale_from = sin((1.0 - t) * angle) / sin_angle;
      scale_to   = sin(t         * angle) / sin_angle;
   }
   else {
      /* to prevent divide-by-zero, resort to linear interpolation */
      scale_from = 1.0 - t;
      scale_to   = t;
   }

   out->w = (float)((scale_from * from->w) + (scale_to * to2.w));
   out->x = (float)((scale_from * from->x) + (scale_to * to2.x));
   out->y = (float)((scale_from * from->y) + (scale_to * to2.y));
   out->z = (float)((scale_from * from->z) + (scale_to * to2.z));
}



