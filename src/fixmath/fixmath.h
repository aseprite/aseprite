/*
 * Fixed point type.
 * Based on Allegro library by Shawn Hargreaves.
 */

#ifndef FIXMATH_FIXMATH_H
#define FIXMATH_FIXMATH_H

#include "base/ints.h"
#include <cerrno>

namespace fixmath {

typedef int32_t fixed;

extern const fixed fixtorad_r;
extern const fixed radtofix_r;

extern fixed _cos_tbl[];
extern fixed _tan_tbl[];
extern fixed _acos_tbl[];

fixed fixsqrt(fixed x);
fixed fixhypot(fixed x, fixed y);
fixed fixatan(fixed x);
fixed fixatan2(fixed y, fixed x);

// ftofix and fixtof are used in generic C versions of fixmul and fixdiv
inline fixed ftofix(double x)
{
  if (x > 32767.0) {
    errno = ERANGE;
    return 0x7FFFFFFF;
  }

  if (x < -32767.0) {
    errno = ERANGE;
    return -0x7FFFFFFF;
  }

  return (fixed)(x * 65536.0 + (x < 0 ? -0.5 : 0.5));
}

inline double fixtof(fixed x)
{
  return (double)x / 65536.0;
};

inline fixed fixadd(fixed x, fixed y)
{
  fixed result = x + y;

  if (result >= 0) {
    if ((x < 0) && (y < 0)) {
      errno = ERANGE;
      return -0x7FFFFFFF;
    }
    return result;
  }
  if ((x > 0) && (y > 0)) {
    errno = ERANGE;
    return 0x7FFFFFFF;
  }
  return result;
}

inline fixed fixsub(fixed x, fixed y)
{
  fixed result = x - y;

  if (result >= 0) {
    if ((x < 0) && (y > 0)) {
      errno = ERANGE;
      return -0x7FFFFFFF;
    }
    return result;
  }
  if ((x > 0) && (y < 0)) {
    errno = ERANGE;
    return 0x7FFFFFFF;
  }
  return result;
}

inline fixed fixmul(fixed x, fixed y)
{
  return ftofix(fixtof(x) * fixtof(y));
}

inline fixed fixdiv(fixed x, fixed y)
{
  if (y == 0) {
    errno = ERANGE;
    return (x < 0) ? -0x7FFFFFFF : 0x7FFFFFFF;
  }
  return ftofix(fixtof(x) / fixtof(y));
}

inline int fixfloor(fixed x)
{
  /* (x >> 16) is not portable */
  if (x >= 0)
    return (x >> 16);
  return ~((~x) >> 16);
}

inline int fixceil(fixed x)
{
  if (x > 0x7FFF0000) {
    errno = ERANGE;
    return 0x7FFF;
  }

  return fixfloor(x + 0xFFFF);
}

inline fixed itofix(int x)
{
  return x << 16;
}

inline int fixtoi(fixed x)
{
  return fixfloor(x) + ((x & 0x8000) >> 15);
}

inline fixed fixcos(fixed x)
{
  return _cos_tbl[((x + 0x4000) >> 15) & 0x1FF];
}

inline fixed fixsin(fixed x)
{
  return _cos_tbl[((x - 0x400000 + 0x4000) >> 15) & 0x1FF];
}

inline fixed fixtan(fixed x)
{
  return _tan_tbl[((x + 0x4000) >> 15) & 0xFF];
}

inline fixed fixacos(fixed x)
{
  if ((x < -65536) || (x > 65536)) {
    errno = EDOM;
    return 0;
  }

  return _acos_tbl[(x + 65536 + 127) >> 8];
}

inline fixed fixasin(fixed x)
{
  if ((x < -65536) || (x > 65536)) {
    errno = EDOM;
    return 0;
  }

  return 0x00400000 - _acos_tbl[(x + 65536 + 127) >> 8];
}

} // namespace fixmath

#endif
