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
 *      Vector and matrix manipulation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <math.h>

#include "allegro.h"



#define FLOATSINCOS(x, s, c)  _AL_SINCOS((x) * AL_PI / 128.0, s ,c)
#define floattan(x)           tan((x) * AL_PI / 128.0)



MATRIX identity_matrix = 
{
   {
      /* 3x3 identity */
      { 1<<16, 0,     0     },
      { 0,     1<<16, 0     },
      { 0,     0,     1<<16 },
   },

   /* zero translation */
   { 0, 0, 0 }
};



MATRIX_f identity_matrix_f = 
{
   {
      /* 3x3 identity */
      { 1.0, 0.0, 0.0 },
      { 0.0, 1.0, 0.0 },
      { 0.0, 0.0, 1.0 },
   },

   /* zero translation */
   { 0.0, 0.0, 0.0 }
};



/* get_translation_matrix:
 *  Constructs a 3d translation matrix. When applied to the vector 
 *  (vx, vy, vx), this will produce (vx+x, vy+y, vz+z).
 */
void get_translation_matrix(MATRIX *m, fixed x, fixed y, fixed z)
{
   ASSERT(m);
   *m = identity_matrix;

   m->t[0] = x;
   m->t[1] = y;
   m->t[2] = z;
}



/* get_translation_matrix_f:
 *  Floating point version of get_translation_matrix().
 */
void get_translation_matrix_f(MATRIX_f *m, float x, float y, float z)
{
   ASSERT(m);
   *m = identity_matrix_f;

   m->t[0] = x;
   m->t[1] = y;
   m->t[2] = z;
}



/* get_scaling_matrix:
 *  Constructs a 3d scaling matrix. When applied to the vector 
 *  (vx, vy, vx), this will produce (vx*x, vy*y, vz*z).
 */
void get_scaling_matrix(MATRIX *m, fixed x, fixed y, fixed z)
{
   ASSERT(m);
   *m = identity_matrix;

   m->v[0][0] = x;
   m->v[1][1] = y;
   m->v[2][2] = z;
}



/* get_scaling_matrix_f:
 *  Floating point version of get_scaling_matrix().
 */
void get_scaling_matrix_f(MATRIX_f *m, float x, float y, float z)
{
   ASSERT(m);
   *m = identity_matrix_f;

   m->v[0][0] = x;
   m->v[1][1] = y;
   m->v[2][2] = z;
}



/* get_x_rotate_matrix:
 *  Constructs a 3d transformation matrix, which will rotate points around 
 *  the x axis by the specified amount (given in the Allegro fixed point, 
 *  256 degrees to a circle format).
 */
void get_x_rotate_matrix(MATRIX *m, fixed r)
{
   fixed c = fixcos(r);
   fixed s = fixsin(r);
   ASSERT(m);

   *m = identity_matrix;

   m->v[1][1] = c;
   m->v[1][2] = -s;

   m->v[2][1] = s;
   m->v[2][2] = c;
}



/* get_x_rotate_matrix_f:
 *  Floating point version of get_x_rotate_matrix().
 */
void get_x_rotate_matrix_f(MATRIX_f *m, float r)
{
   float c, s;
   ASSERT(m);

   FLOATSINCOS(r, s, c);
   *m = identity_matrix_f;

   m->v[1][1] = c;
   m->v[1][2] = -s;

   m->v[2][1] = s;
   m->v[2][2] = c;
}



/* get_y_rotate_matrix:
 *  Constructs a 3d transformation matrix, which will rotate points around 
 *  the y axis by the specified amount (given in the Allegro fixed point, 
 *  256 degrees to a circle format).
 */
void get_y_rotate_matrix(MATRIX *m, fixed r)
{
   fixed c = fixcos(r);
   fixed s = fixsin(r);
   ASSERT(m);

   *m = identity_matrix;

   m->v[0][0] = c;
   m->v[0][2] = s;

   m->v[2][0] = -s;
   m->v[2][2] = c;
}



/* get_y_rotate_matrix_f:
 *  Floating point version of get_y_rotate_matrix().
 */
void get_y_rotate_matrix_f(MATRIX_f *m, float r)
{
   float c, s;
   ASSERT(m);

   FLOATSINCOS(r, s, c);
   *m = identity_matrix_f;

   m->v[0][0] = c;
   m->v[0][2] = s;

   m->v[2][0] = -s;
   m->v[2][2] = c;
}



/* get_z_rotate_matrix:
 *  Constructs a 3d transformation matrix, which will rotate points around 
 *  the z axis by the specified amount (given in the Allegro fixed point, 
 *  256 degrees to a circle format).
 */
void get_z_rotate_matrix(MATRIX *m, fixed r)
{
   fixed c = fixcos(r);
   fixed s = fixsin(r);
   ASSERT(m);

   *m = identity_matrix;

   m->v[0][0] = c;
   m->v[0][1] = -s;

   m->v[1][0] = s;
   m->v[1][1] = c;
}



/* get_z_rotate_matrix_f:
 *  Floating point version of get_z_rotate_matrix().
 */
void get_z_rotate_matrix_f(MATRIX_f *m, float r)
{
   float c, s;
   ASSERT(m);

   FLOATSINCOS(r, s, c);
   *m = identity_matrix_f;

   m->v[0][0] = c;
   m->v[0][1] = -s;

   m->v[1][0] = s;
   m->v[1][1] = c;
}



/* magical formulae for constructing rotation matrices */
#define MAKE_ROTATION(x, y, z)                  \
   fixed sin_x = fixsin(x);                     \
   fixed cos_x = fixcos(x);                     \
						\
   fixed sin_y = fixsin(y);                     \
   fixed cos_y = fixcos(y);                     \
						\
   fixed sin_z = fixsin(z);                     \
   fixed cos_z = fixcos(z);                     \
						\
   fixed sinx_siny = fixmul(sin_x, sin_y);      \
   fixed cosx_siny = fixmul(cos_x, sin_y);



#define MAKE_ROTATION_f(x, y, z)                \
   float sin_x, cos_x;				\
   float sin_y, cos_y;				\
   float sin_z, cos_z;				\
   float sinx_siny, cosx_siny;			\
						\
   FLOATSINCOS(x, sin_x, cos_x);		\
   FLOATSINCOS(y, sin_y, cos_y);		\
   FLOATSINCOS(z, sin_z, cos_z);		\
						\
   sinx_siny = sin_x * sin_y;			\
   cosx_siny = cos_x * sin_y;



#define R00 (fixmul(cos_y, cos_z))
#define R10 (fixmul(sinx_siny, cos_z) - fixmul(cos_x, sin_z))
#define R20 (fixmul(cosx_siny, cos_z) + fixmul(sin_x, sin_z))

#define R01 (fixmul(cos_y, sin_z))
#define R11 (fixmul(sinx_siny, sin_z) + fixmul(cos_x, cos_z))
#define R21 (fixmul(cosx_siny, sin_z) - fixmul(sin_x, cos_z))

#define R02 (-sin_y)
#define R12 (fixmul(sin_x, cos_y))
#define R22 (fixmul(cos_x, cos_y))



#define R00_f (cos_y * cos_z)
#define R10_f ((sinx_siny * cos_z) - (cos_x * sin_z))
#define R20_f ((cosx_siny * cos_z) + (sin_x * sin_z))

#define R01_f (cos_y * sin_z)
#define R11_f ((sinx_siny * sin_z) + (cos_x * cos_z))
#define R21_f ((cosx_siny * sin_z) - (sin_x * cos_z))

#define R02_f (-sin_y)
#define R12_f (sin_x * cos_y)
#define R22_f (cos_x * cos_y)



/* get_rotation_matrix:
 *  Constructs a 3d transformation matrix, which will rotate points around
 *  all three axis by the specified amounts (given in the Allegro fixed 
 *  point, 256 degrees to a circle format).
 */
void get_rotation_matrix(MATRIX *m, fixed x, fixed y, fixed z)
{
   MAKE_ROTATION(x, y, z);
   ASSERT(m);

   m->v[0][0] = R00;
   m->v[0][1] = R01;
   m->v[0][2] = R02;

   m->v[1][0] = R10;
   m->v[1][1] = R11;
   m->v[1][2] = R12;

   m->v[2][0] = R20;
   m->v[2][1] = R21;
   m->v[2][2] = R22;

   m->t[0] = m->t[1] = m->t[2] = 0;
}



/* get_rotation_matrix_f:
 *  Floating point version of get_rotation_matrix().
 */
void get_rotation_matrix_f(MATRIX_f *m, float x, float y, float z)
{
   MAKE_ROTATION_f(x, y, z);
   ASSERT(m);

   m->v[0][0] = R00_f;
   m->v[0][1] = R01_f;
   m->v[0][2] = R02_f;

   m->v[1][0] = R10_f;
   m->v[1][1] = R11_f;
   m->v[1][2] = R12_f;

   m->v[2][0] = R20_f;
   m->v[2][1] = R21_f;
   m->v[2][2] = R22_f;

   m->t[0] = m->t[1] = m->t[2] = 0;
}



/* get_align_matrix:
 *  Aligns a matrix along an arbitrary coordinate system.
 */
void get_align_matrix(MATRIX *m, fixed xfront, fixed yfront, fixed zfront, fixed xup, fixed yup, fixed zup)
{
   fixed xright, yright, zright;
   ASSERT(m);

   xfront = -xfront;
   yfront = -yfront;
   zfront = -zfront;

   normalize_vector(&xfront, &yfront, &zfront);
   cross_product(xup, yup, zup, xfront, yfront, zfront, &xright, &yright, &zright);
   normalize_vector(&xright, &yright, &zright);
   cross_product(xfront, yfront, zfront, xright, yright, zright, &xup, &yup, &zup);
   /* No need to normalize up here, since right and front are perpendicular and normalized. */

   m->v[0][0] = xright; 
   m->v[0][1] = xup; 
   m->v[0][2] = xfront; 

   m->v[1][0] = yright;
   m->v[1][1] = yup;
   m->v[1][2] = yfront;

   m->v[2][0] = zright;
   m->v[2][1] = zup;
   m->v[2][2] = zfront;

   m->t[0] = m->t[1] = m->t[2] = 0;
}



/* get_align_matrix_f:
 *  Floating point version of get_align_matrix().
 */
void get_align_matrix_f(MATRIX_f *m, float xfront, float yfront, float zfront, float xup, float yup, float zup)
{
   float xright, yright, zright;
   ASSERT(m);

   xfront = -xfront;
   yfront = -yfront;
   zfront = -zfront;

   normalize_vector_f(&xfront, &yfront, &zfront);
   cross_product_f(xup, yup, zup, xfront, yfront, zfront, &xright, &yright, &zright);
   normalize_vector_f(&xright, &yright, &zright);
   cross_product_f(xfront, yfront, zfront, xright, yright, zright, &xup, &yup, &zup);
   /* No need to normalize up here, since right and front are perpendicular and normalized. */

   m->v[0][0] = xright; 
   m->v[0][1] = xup; 
   m->v[0][2] = xfront; 

   m->v[1][0] = yright;
   m->v[1][1] = yup;
   m->v[1][2] = yfront;

   m->v[2][0] = zright;
   m->v[2][1] = zup;
   m->v[2][2] = zfront;

   m->t[0] = m->t[1] = m->t[2] = 0;
}



/* get_vector_rotation_matrix:
 *  Constructs a 3d transformation matrix, which will rotate points around
 *  the specified x,y,z vector by the specified angle (given in the Allegro 
 *  fixed point, 256 degrees to a circle format), in a clockwise direction.
 */
void get_vector_rotation_matrix(MATRIX *m, fixed x, fixed y, fixed z, fixed a)
{
   MATRIX_f rotation;
   int i, j;
   ASSERT(m);

   get_vector_rotation_matrix_f(&rotation, fixtof(x), fixtof(y), fixtof(z), fixtof(a));

   for (i=0; i<3; i++)
      for (j=0; j<3; j++)
	 m->v[i][j] = ftofix(rotation.v[i][j]);

   m->t[0] = m->t[1] = m->t[2] = 0;
}



/* get_vector_rotation_matrix_f:
 *  Floating point version of get_vector_rotation_matrix().
 */
void get_vector_rotation_matrix_f(MATRIX_f *m, float x, float y, float z, float a)
{
   float c, s, cc;
   ASSERT(m);

   FLOATSINCOS(a, s, c);
   cc = 1 - c;
   normalize_vector_f(&x, &y, &z);

   m->v[0][0] = (cc * x * x) + c;
   m->v[0][1] = (cc * x * y) + (z * s);
   m->v[0][2] = (cc * x * z) - (y * s);

   m->v[1][0] = (cc * x * y) - (z * s);
   m->v[1][1] = (cc * y * y) + c;
   m->v[1][2] = (cc * z * y) + (x * s);

   m->v[2][0] = (cc * x * z) + (y * s);
   m->v[2][1] = (cc * y * z) - (x * s);
   m->v[2][2] = (cc * z * z) + c;

   m->t[0] = m->t[1] = m->t[2] = 0;
}



/* get_transformation_matrix:
 *  Constructs a 3d transformation matrix, which will rotate points around
 *  all three axis by the specified amounts (given in the Allegro fixed 
 *  point, 256 degrees to a circle format), scale the result by the
 *  specified amount (itofix(1) for no change of scale), and then translate
 *  to the requested x, y, z position.
 */
void get_transformation_matrix(MATRIX *m, fixed scale, fixed xrot, fixed yrot, fixed zrot, fixed x, fixed y, fixed z)
{
   MAKE_ROTATION(xrot, yrot, zrot);
   ASSERT(m);

   m->v[0][0] = fixmul(R00, scale);
   m->v[0][1] = fixmul(R01, scale);
   m->v[0][2] = fixmul(R02, scale);

   m->v[1][0] = fixmul(R10, scale);
   m->v[1][1] = fixmul(R11, scale);
   m->v[1][2] = fixmul(R12, scale);

   m->v[2][0] = fixmul(R20, scale);
   m->v[2][1] = fixmul(R21, scale);
   m->v[2][2] = fixmul(R22, scale);

   m->t[0] = x;
   m->t[1] = y;
   m->t[2] = z;
}



/* get_transformation_matrix_f:
 *  Floating point version of get_transformation_matrix().
 */
void get_transformation_matrix_f(MATRIX_f *m, float scale, float xrot, float yrot, float zrot, float x, float y, float z)
{
   MAKE_ROTATION_f(xrot, yrot, zrot);
   ASSERT(m);

   m->v[0][0] = R00_f * scale;
   m->v[0][1] = R01_f * scale;
   m->v[0][2] = R02_f * scale;

   m->v[1][0] = R10_f * scale;
   m->v[1][1] = R11_f * scale;
   m->v[1][2] = R12_f * scale;

   m->v[2][0] = R20_f * scale;
   m->v[2][1] = R21_f * scale;
   m->v[2][2] = R22_f * scale;

   m->t[0] = x;
   m->t[1] = y;
   m->t[2] = z;
}



/* get_camera_matrix: 
 *  Constructs a camera matrix for translating world-space objects into
 *  a normalised view space, ready for the perspective projection. The
 *  x, y, and z parameters specify the camera position, xfront, yfront,
 *  and zfront is an 'in front' vector specifying which way the camera
 *  is facing (this can be any length: normalisation is not required),
 *  and xup, yup, and zup is the 'up' direction vector. Up is really only
 *  a 1.5d vector, since the front vector only leaves one degree of freedom
 *  for which way up to put the image, but it is simplest to specify it
 *  as a full 3d direction even though a lot of the information in it is
 *  discarded. The fov parameter specifies the field of view (ie. width
 *  of the camera focus) in fixed point, 256 degrees to the circle format.
 *  For typical projections, a field of view in the region 32-48 will work
 *  well. Finally, the aspect ratio is used to scale the Y dimensions of
 *  the image relative to the X axis, so you can use it to correct for
 *  the proportions of the output image (set it to 1 for no scaling).
 */
void get_camera_matrix(MATRIX *m, fixed x, fixed y, fixed z, fixed xfront, fixed yfront, fixed zfront, fixed xup, fixed yup, fixed zup, fixed fov, fixed aspect)
{
   MATRIX_f camera;
   int i, j;
   ASSERT(m);

   get_camera_matrix_f(&camera,
		       fixtof(x), fixtof(y), fixtof(z), 
		       fixtof(xfront), fixtof(yfront), fixtof(zfront), 
		       fixtof(xup), fixtof(yup), fixtof(zup), 
		       fixtof(fov), fixtof(aspect));

   for (i=0; i<3; i++) {
      for (j=0; j<3; j++)
	 m->v[i][j] = ftofix(camera.v[i][j]);

      m->t[i] = ftofix(camera.t[i]);
   }
}



/* get_camera_matrix_f: 
 *  Floating point version of get_camera_matrix().
 */
void get_camera_matrix_f(MATRIX_f *m, float x, float y, float z, float xfront, float yfront, float zfront, float xup, float yup, float zup, float fov, float aspect)
{
   MATRIX_f camera, scale;
   float xside, yside, zside, width, d;
   ASSERT(m);

   /* make 'in-front' into a unit vector, and negate it */
   normalize_vector_f(&xfront, &yfront, &zfront);
   xfront = -xfront;
   yfront = -yfront;
   zfront = -zfront;

   /* make sure 'up' is at right angles to 'in-front', and normalize */
   d = dot_product_f(xup, yup, zup, xfront, yfront, zfront);
   xup -= d * xfront; 
   yup -= d * yfront; 
   zup -= d * zfront;
   normalize_vector_f(&xup, &yup, &zup);

   /* calculate the 'sideways' vector */
   cross_product_f(xup, yup, zup, xfront, yfront, zfront, &xside, &yside, &zside);

   /* set matrix rotation parameters */
   camera.v[0][0] = xside; 
   camera.v[0][1] = yside;
   camera.v[0][2] = zside;

   camera.v[1][0] = xup; 
   camera.v[1][1] = yup;
   camera.v[1][2] = zup;

   camera.v[2][0] = xfront; 
   camera.v[2][1] = yfront;
   camera.v[2][2] = zfront;

   /* set matrix translation parameters */
   camera.t[0] = -(x*xside  + y*yside  + z*zside);
   camera.t[1] = -(x*xup    + y*yup    + z*zup);
   camera.t[2] = -(x*xfront + y*yfront + z*zfront);

   /* construct a scaling matrix to deal with aspect ratio and FOV */
   width = floattan(64.0 - fov/2);
   get_scaling_matrix_f(&scale, width, -aspect*width, -1.0);

   /* combine the camera and scaling matrices */
   matrix_mul_f(&camera, &scale, m);
}



/* qtranslate_matrix:
 *  Adds a position offset to an existing matrix.
 */
void qtranslate_matrix(MATRIX *m, fixed x, fixed y, fixed z)
{
   ASSERT(m);
   m->t[0] += x;
   m->t[1] += y;
   m->t[2] += z;
}



/* qtranslate_matrix_f:
 *  Floating point version of qtranslate_matrix().
 */
void qtranslate_matrix_f(MATRIX_f *m, float x, float y, float z)
{
   ASSERT(m);
   m->t[0] += x;
   m->t[1] += y;
   m->t[2] += z;
}



/* qscale_matrix:
 *  Adds a scaling factor to an existing matrix.
 */
void qscale_matrix(MATRIX *m, fixed scale)
{
   int i, j;
   ASSERT(m);

   for (i=0; i<3; i++)
      for (j=0; j<3; j++)
	 m->v[i][j] = fixmul(m->v[i][j], scale);
}



/* qscale_matrix_f:
 *  Floating point version of qscale_matrix().
 */
void qscale_matrix_f(MATRIX_f *m, float scale)
{
   int i, j;
   ASSERT(m);

   for (i=0; i<3; i++)
      for (j=0; j<3; j++)
	 m->v[i][j] *= scale;
}



/* matrix_mul:
 *  Multiplies two matrices, storing the result in out (this must be
 *  different from the two input matrices). The resulting matrix will
 *  have the same effect as the combination of m1 and m2, ie. when
 *  applied to a vector v, (v * out) = ((v * m1) * m2). Any number of
 *  transformations can be concatenated in this way.
 */
void matrix_mul(AL_CONST MATRIX *m1, AL_CONST MATRIX *m2, MATRIX *out)
{
   MATRIX temp;
   int i, j;
   ASSERT(m1);
   ASSERT(m2);
   ASSERT(out);

   if (m1 == out) {
      temp = *m1;
      m1 = &temp;
   }
   else if (m2 == out) {
      temp = *m2;
      m2 = &temp;
   }

   for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
	 out->v[i][j] = fixmul(m1->v[0][j], m2->v[i][0]) +
			fixmul(m1->v[1][j], m2->v[i][1]) +
			fixmul(m1->v[2][j], m2->v[i][2]);
      }

      out->t[i] = fixmul(m1->t[0], m2->v[i][0]) +
		  fixmul(m1->t[1], m2->v[i][1]) +
		  fixmul(m1->t[2], m2->v[i][2]) +
		  m2->t[i];
   } 
}



/* matrix_mul_f:
 *  Floating point version of matrix_mul().
 */
void matrix_mul_f(AL_CONST MATRIX_f *m1, AL_CONST MATRIX_f *m2, MATRIX_f *out)
{
   MATRIX_f temp;
   int i, j;
   ASSERT(m1);
   ASSERT(m2);
   ASSERT(out);

   if (m1 == out) {
      temp = *m1;
      m1 = &temp;
   }
   else if (m2 == out) {
      temp = *m2;
      m2 = &temp;
   }

   for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
	 out->v[i][j] = (m1->v[0][j] * m2->v[i][0]) +
			(m1->v[1][j] * m2->v[i][1]) +
			(m1->v[2][j] * m2->v[i][2]);
      }

      out->t[i] = (m1->t[0] * m2->v[i][0]) +
		  (m1->t[1] * m2->v[i][1]) +
		  (m1->t[2] * m2->v[i][2]) +
		  m2->t[i];
   }
}



/* vector_length: 
 *  Computes the length of a vector, using the son of the squaw...
 */
fixed vector_length(fixed x, fixed y, fixed z)
{
   x >>= 8;
   y >>= 8;
   z >>= 8;

   return (fixsqrt(fixmul(x,x) + fixmul(y,y) + fixmul(z,z)) << 8);
}



/* vector_lengthf: 
 *  Floating point version of vector_length().
 */
float vector_length_f(float x, float y, float z)
{
   return sqrt(x*x + y*y + z*z);
}



/* normalize_vector: 
 *  Converts the specified vector to a unit vector, which has the same
 *  orientation but a length of one.
 */
void normalize_vector(fixed *x, fixed *y, fixed *z)
{
   fixed length = vector_length(*x, *y, *z);

   *x = fixdiv(*x, length);
   *y = fixdiv(*y, length);
   *z = fixdiv(*z, length);
}



/* normalize_vectorf: 
 *  Floating point version of normalize_vector().
 */
void normalize_vector_f(float *x, float *y, float *z)
{
   float length = 1.0 / vector_length_f(*x, *y, *z);

   *x *= length;
   *y *= length;
   *z *= length;
}



/* cross_product:
 *  Calculates the cross product of two vectors.
 */
void cross_product(fixed x1, fixed y1, fixed z1, fixed x2, fixed y2, fixed z2, fixed *xout, fixed *yout, fixed *zout)
{
   ASSERT(xout);
   ASSERT(yout);
   ASSERT(zout);

   *xout = fixmul(y1, z2) - fixmul(z1, y2);
   *yout = fixmul(z1, x2) - fixmul(x1, z2);
   *zout = fixmul(x1, y2) - fixmul(y1, x2);
}



/* cross_productf:
 *  Floating point version of cross_product().
 */
void cross_product_f(float x1, float y1, float z1, float x2, float y2, float z2, float *xout, float *yout, float *zout)
{
   ASSERT(xout);
   ASSERT(yout);
   ASSERT(zout);

   *xout = (y1 * z2) - (z1 * y2);
   *yout = (z1 * x2) - (x1 * z2);
   *zout = (x1 * y2) - (y1 * x2);
}



/* polygon_z_normal:
 *  Helper function for backface culling: returns the z component of the
 *  normal vector to the polygon formed from the three vertices.
 */
fixed polygon_z_normal(AL_CONST V3D *v1, AL_CONST V3D *v2, AL_CONST V3D *v3)
{
   ASSERT(v1);
   ASSERT(v2);
   ASSERT(v3);
   return (fixmul(v2->x-v1->x, v3->y-v2->y) - fixmul(v3->x-v2->x, v2->y-v1->y));
}



/* polygon_z_normal_f:
 *  Floating point version of polygon_z_normal().
 */
float polygon_z_normal_f(AL_CONST V3D_f *v1, AL_CONST V3D_f *v2, AL_CONST V3D_f *v3)
{
   ASSERT(v1);
   ASSERT(v2);
   ASSERT(v3);
   return ((v2->x-v1->x) * (v3->y-v2->y)) - ((v3->x-v2->x) * (v2->y-v1->y));
}



/* scaling factors for the perspective projection */
fixed _persp_xscale = 160 << 16;
fixed _persp_yscale = 100 << 16;
fixed _persp_xoffset = 160 << 16;
fixed _persp_yoffset = 100 << 16;

float _persp_xscale_f = 160.0;
float _persp_yscale_f = 100.0;
float _persp_xoffset_f = 160.0;
float _persp_yoffset_f = 100.0;



/* set_projection_viewport:
 *  Sets the viewport used to scale the output of the persp_project() 
 *  function.
 */
void set_projection_viewport(int x, int y, int w, int h)
{
   ASSERT(w > 0);
   ASSERT(h > 0);
   
   _persp_xscale = itofix(w/2);
   _persp_yscale = itofix(h/2);
   _persp_xoffset = itofix(x + w/2);
   _persp_yoffset = itofix(y + h/2);

   _persp_xscale_f = w/2;
   _persp_yscale_f = h/2;
   _persp_xoffset_f = x + w/2;
   _persp_yoffset_f = y + h/2;
}

