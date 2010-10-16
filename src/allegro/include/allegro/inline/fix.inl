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
 *      Fix class inline functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FIX_INL
#define ALLEGRO_FIX_INL

#ifdef __cplusplus


inline  fix operator +  (const fix x, const fix y)    { fix t;  t.v = x.v + y.v;        return t; }
inline  fix operator +  (const fix x, const int y)    { fix t;  t.v = x.v + itofix(y);  return t; }
inline  fix operator +  (const int x, const fix y)    { fix t;  t.v = itofix(x) + y.v;  return t; }
inline  fix operator +  (const fix x, const long y)   { fix t;  t.v = x.v + itofix(y);  return t; }
inline  fix operator +  (const long x, const fix y)   { fix t;  t.v = itofix(x) + y.v;  return t; }
inline  fix operator +  (const fix x, const float y)  { fix t;  t.v = x.v + ftofix(y);  return t; }
inline  fix operator +  (const float x, const fix y)  { fix t;  t.v = ftofix(x) + y.v;  return t; }
inline  fix operator +  (const fix x, const double y) { fix t;  t.v = x.v + ftofix(y);  return t; }
inline  fix operator +  (const double x, const fix y) { fix t;  t.v = ftofix(x) + y.v;  return t; }

inline  fix operator -  (const fix x, const fix y)    { fix t;  t.v = x.v - y.v;        return t; }
inline  fix operator -  (const fix x, const int y)    { fix t;  t.v = x.v - itofix(y);  return t; }
inline  fix operator -  (const int x, const fix y)    { fix t;  t.v = itofix(x) - y.v;  return t; }
inline  fix operator -  (const fix x, const long y)   { fix t;  t.v = x.v - itofix(y);  return t; }
inline  fix operator -  (const long x, const fix y)   { fix t;  t.v = itofix(x) - y.v;  return t; }
inline  fix operator -  (const fix x, const float y)  { fix t;  t.v = x.v - ftofix(y);  return t; }
inline  fix operator -  (const float x, const fix y)  { fix t;  t.v = ftofix(x) - y.v;  return t; }
inline  fix operator -  (const fix x, const double y) { fix t;  t.v = x.v - ftofix(y);  return t; }
inline  fix operator -  (const double x, const fix y) { fix t;  t.v = ftofix(x) - y.v;  return t; }

inline  fix operator *  (const fix x, const fix y)    { fix t;  t.v = fixmul(x.v, y.v);         return t; }
inline  fix operator *  (const fix x, const int y)    { fix t;  t.v = x.v * y;                  return t; }
inline  fix operator *  (const int x, const fix y)    { fix t;  t.v = x * y.v;                  return t; }
inline  fix operator *  (const fix x, const long y)   { fix t;  t.v = x.v * y;                  return t; }
inline  fix operator *  (const long x, const fix y)   { fix t;  t.v = x * y.v;                  return t; }
inline  fix operator *  (const fix x, const float y)  { fix t;  t.v = ftofix(fixtof(x.v) * y);  return t; }
inline  fix operator *  (const float x, const fix y)  { fix t;  t.v = ftofix(x * fixtof(y.v));  return t; }
inline  fix operator *  (const fix x, const double y) { fix t;  t.v = ftofix(fixtof(x.v) * y);  return t; }
inline  fix operator *  (const double x, const fix y) { fix t;  t.v = ftofix(x * fixtof(y.v));  return t; }

inline  fix operator /  (const fix x, const fix y)    { fix t;  t.v = fixdiv(x.v, y.v);         return t; }
inline  fix operator /  (const fix x, const int y)    { fix t;  t.v = x.v / y;                  return t; }
inline  fix operator /  (const int x, const fix y)    { fix t;  t.v = fixdiv(itofix(x), y.v);   return t; }
inline  fix operator /  (const fix x, const long y)   { fix t;  t.v = x.v / y;                  return t; }
inline  fix operator /  (const long x, const fix y)   { fix t;  t.v = fixdiv(itofix(x), y.v);   return t; }
inline  fix operator /  (const fix x, const float y)  { fix t;  t.v = ftofix(fixtof(x.v) / y);  return t; }
inline  fix operator /  (const float x, const fix y)  { fix t;  t.v = ftofix(x / fixtof(y.v));  return t; }
inline  fix operator /  (const fix x, const double y) { fix t;  t.v = ftofix(fixtof(x.v) / y);  return t; }
inline  fix operator /  (const double x, const fix y) { fix t;  t.v = ftofix(x / fixtof(y.v));  return t; }

inline  fix operator << (const fix x, const int y)    { fix t;  t.v = x.v << y;   return t; }
inline  fix operator >> (const fix x, const int y)    { fix t;  t.v = x.v >> y;   return t; }

inline  int operator == (const fix x, const fix y)    { return (x.v == y.v);       }
inline  int operator == (const fix x, const int y)    { return (x.v == itofix(y)); }
inline  int operator == (const int x, const fix y)    { return (itofix(x) == y.v); }
inline  int operator == (const fix x, const long y)   { return (x.v == itofix(y)); }
inline  int operator == (const long x, const fix y)   { return (itofix(x) == y.v); }
inline  int operator == (const fix x, const float y)  { return (x.v == ftofix(y)); }
inline  int operator == (const float x, const fix y)  { return (ftofix(x) == y.v); }
inline  int operator == (const fix x, const double y) { return (x.v == ftofix(y)); }
inline  int operator == (const double x, const fix y) { return (ftofix(x) == y.v); }

inline  int operator != (const fix x, const fix y)    { return (x.v != y.v);       }
inline  int operator != (const fix x, const int y)    { return (x.v != itofix(y)); }
inline  int operator != (const int x, const fix y)    { return (itofix(x) != y.v); }
inline  int operator != (const fix x, const long y)   { return (x.v != itofix(y)); }
inline  int operator != (const long x, const fix y)   { return (itofix(x) != y.v); }
inline  int operator != (const fix x, const float y)  { return (x.v != ftofix(y)); }
inline  int operator != (const float x, const fix y)  { return (ftofix(x) != y.v); }
inline  int operator != (const fix x, const double y) { return (x.v != ftofix(y)); }
inline  int operator != (const double x, const fix y) { return (ftofix(x) != y.v); }

inline  int operator <  (const fix x, const fix y)    { return (x.v < y.v);        }
inline  int operator <  (const fix x, const int y)    { return (x.v < itofix(y));  }
inline  int operator <  (const int x, const fix y)    { return (itofix(x) < y.v);  }
inline  int operator <  (const fix x, const long y)   { return (x.v < itofix(y));  }
inline  int operator <  (const long x, const fix y)   { return (itofix(x) < y.v);  }
inline  int operator <  (const fix x, const float y)  { return (x.v < ftofix(y));  }
inline  int operator <  (const float x, const fix y)  { return (ftofix(x) < y.v);  }
inline  int operator <  (const fix x, const double y) { return (x.v < ftofix(y));  }
inline  int operator <  (const double x, const fix y) { return (ftofix(x) < y.v);  }

inline  int operator >  (const fix x, const fix y)    { return (x.v > y.v);        }
inline  int operator >  (const fix x, const int y)    { return (x.v > itofix(y));  }
inline  int operator >  (const int x, const fix y)    { return (itofix(x) > y.v);  }
inline  int operator >  (const fix x, const long y)   { return (x.v > itofix(y));  }
inline  int operator >  (const long x, const fix y)   { return (itofix(x) > y.v);  }
inline  int operator >  (const fix x, const float y)  { return (x.v > ftofix(y));  }
inline  int operator >  (const float x, const fix y)  { return (ftofix(x) > y.v);  }
inline  int operator >  (const fix x, const double y) { return (x.v > ftofix(y));  }
inline  int operator >  (const double x, const fix y) { return (ftofix(x) > y.v);  }

inline  int operator <= (const fix x, const fix y)    { return (x.v <= y.v);       }
inline  int operator <= (const fix x, const int y)    { return (x.v <= itofix(y)); }
inline  int operator <= (const int x, const fix y)    { return (itofix(x) <= y.v); }
inline  int operator <= (const fix x, const long y)   { return (x.v <= itofix(y)); }
inline  int operator <= (const long x, const fix y)   { return (itofix(x) <= y.v); }
inline  int operator <= (const fix x, const float y)  { return (x.v <= ftofix(y)); }
inline  int operator <= (const float x, const fix y)  { return (ftofix(x) <= y.v); }
inline  int operator <= (const fix x, const double y) { return (x.v <= ftofix(y)); }
inline  int operator <= (const double x, const fix y) { return (ftofix(x) <= y.v); }

inline  int operator >= (const fix x, const fix y)    { return (x.v >= y.v);       }
inline  int operator >= (const fix x, const int y)    { return (x.v >= itofix(y)); }
inline  int operator >= (const int x, const fix y)    { return (itofix(x) >= y.v); }
inline  int operator >= (const fix x, const long y)   { return (x.v >= itofix(y)); }
inline  int operator >= (const long x, const fix y)   { return (itofix(x) >= y.v); }
inline  int operator >= (const fix x, const float y)  { return (x.v >= ftofix(y)); }
inline  int operator >= (const float x, const fix y)  { return (ftofix(x) >= y.v); }
inline  int operator >= (const fix x, const double y) { return (x.v >= ftofix(y)); }
inline  int operator >= (const double x, const fix y) { return (ftofix(x) >= y.v); }

inline  fix sqrt(fix x)          { fix t;  t.v = fixsqrt(x.v);        return t; }
inline  fix cos(fix x)           { fix t;  t.v = fixcos(x.v);         return t; }
inline  fix sin(fix x)           { fix t;  t.v = fixsin(x.v);         return t; }
inline  fix tan(fix x)           { fix t;  t.v = fixtan(x.v);         return t; }
inline  fix acos(fix x)          { fix t;  t.v = fixacos(x.v);        return t; }
inline  fix asin(fix x)          { fix t;  t.v = fixasin(x.v);        return t; }
inline  fix atan(fix x)          { fix t;  t.v = fixatan(x.v);        return t; }
inline  fix atan2(fix x, fix y)  { fix t;  t.v = fixatan2(x.v, y.v);  return t; }


inline void get_translation_matrix(MATRIX *m, fix x, fix y, fix z)
{
   get_translation_matrix(m, x.v, y.v, z.v);
}


inline void get_scaling_matrix(MATRIX *m, fix x, fix y, fix z)
{
   get_scaling_matrix(m, x.v, y.v, z.v);
}


inline void get_x_rotate_matrix(MATRIX *m, fix r)
{
   get_x_rotate_matrix(m, r.v);
}


inline void get_y_rotate_matrix(MATRIX *m, fix r)
{
   get_y_rotate_matrix(m, r.v);
}


inline void get_z_rotate_matrix(MATRIX *m, fix r)
{
   get_z_rotate_matrix(m, r.v);
}


inline void get_rotation_matrix(MATRIX *m, fix x, fix y, fix z)
{
   get_rotation_matrix(m, x.v, y.v, z.v);
}


inline void get_align_matrix(MATRIX *m, fix xfront, fix yfront, fix zfront, fix xup, fix yup, fix zup)
{
   get_align_matrix(m, xfront.v, yfront.v, zfront.v, xup.v, yup.v, zup.v);
}


inline void get_vector_rotation_matrix(MATRIX *m, fix x, fix y, fix z, fix a)
{
   get_vector_rotation_matrix(m, x.v, y.v, z.v, a.v);
}


inline void get_transformation_matrix(MATRIX *m, fix scale, fix xrot, fix yrot, fix zrot, fix x, fix y, fix z)
{
   get_transformation_matrix(m, scale.v, xrot.v, yrot.v, zrot.v, x.v, y.v, z.v);
}


inline void get_camera_matrix(MATRIX *m, fix x, fix y, fix z, fix xfront, fix yfront, fix zfront, fix xup, fix yup, fix zup, fix fov, fix aspect)
{
   get_camera_matrix(m, x.v, y.v, z.v, xfront.v, yfront.v, zfront.v, xup.v, yup.v, zup.v, fov.v, aspect.v);
}


inline void qtranslate_matrix(MATRIX *m, fix x, fix y, fix z)
{
   qtranslate_matrix(m, x.v, y.v, z.v);
}


inline void qscale_matrix(MATRIX *m, fix scale)
{
   qscale_matrix(m, scale.v);
}


inline fix vector_length(fix x, fix y, fix z)
{
   fix t;
   t.v = vector_length(x.v, y.v, z.v);
   return t;
}


inline void normalize_vector(fix *x, fix *y, fix *z)
{
   normalize_vector(&x->v, &y->v, &z->v);
}


inline void cross_product(fix x1, fix y_1, fix z1, fix x2, fix y2, fix z2, fix *xout, fix *yout, fix *zout)
{
   cross_product(x1.v, y_1.v, z1.v, x2.v, y2.v, z2.v, &xout->v, &yout->v, &zout->v);
}


inline fix dot_product(fix x1, fix y_1, fix z1, fix x2, fix y2, fix z2)
{
   fix t;
   t.v = dot_product(x1.v, y_1.v, z1.v, x2.v, y2.v, z2.v);
   return t;
}


inline void apply_matrix(MATRIX *m, fix x, fix y, fix z, fix *xout, fix *yout, fix *zout)
{
   apply_matrix(m, x.v, y.v, z.v, &xout->v, &yout->v, &zout->v);
}


inline void persp_project(fix x, fix y, fix z, fix *xout, fix *yout)
{
   persp_project(x.v, y.v, z.v, &xout->v, &yout->v);
}


#endif          /* ifdef __cplusplus */

#endif          /* ifndef ALLEGRO_FIX_INL */


