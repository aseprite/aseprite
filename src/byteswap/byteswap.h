/* Macros to swap the order of bytes in integer values.
   Copyright (C) 1997,1998,2000,2001,2002,2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _PORTABLEXDR_BYTESWAP_H
#define _PORTABLEXDR_BYTESWAP_H 1

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef __bswap_constant_16

/* Swap bytes in 16 bit value.  */
#define __bswap_constant_16(x) \
     ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8))

#ifdef __GNUC__
# define __bswap_16(x) \
    (__extension__							      \
     ({ unsigned short int __bsx = (x); __bswap_constant_16 (__bsx); }))
#else
static __inline unsigned short int
__bswap_16 (unsigned short int __bsx)
{
  return __bswap_constant_16 (__bsx);
}
#endif

/* Swap bytes in 32 bit value.  */
#define __bswap_constant_32(x) \
     ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |	      \
      (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))

#ifdef __GNUC__
# define __bswap_32(x) \
  (__extension__							      \
   ({ register unsigned int __bsx = (x); __bswap_constant_32 (__bsx); }))
#else
static __inline unsigned int
__bswap_32 (unsigned int __bsx)
{
  return __bswap_constant_32 (__bsx);
}
#endif

#if defined __GNUC__ && __GNUC__ >= 2
/* Swap bytes in 64 bit value.  */
# define __bswap_constant_64(x) \
     ((((x) & 0xff00000000000000ull) >> 56)				      \
      | (((x) & 0x00ff000000000000ull) >> 40)				      \
      | (((x) & 0x0000ff0000000000ull) >> 24)				      \
      | (((x) & 0x000000ff00000000ull) >> 8)				      \
      | (((x) & 0x00000000ff000000ull) << 8)				      \
      | (((x) & 0x0000000000ff0000ull) << 24)				      \
      | (((x) & 0x000000000000ff00ull) << 40)				      \
      | (((x) & 0x00000000000000ffull) << 56))

# define __bswap_64(x) \
     (__extension__							      \
      ({ union { __extension__ unsigned long long int __ll;		      \
		 unsigned int __l[2]; } __w, __r;			      \
         if (__builtin_constant_p (x))					      \
	   __r.__ll = __bswap_constant_64 (x);				      \
	 else								      \
	   {								      \
	     __w.__ll = (x);						      \
	     __r.__l[0] = __bswap_32 (__w.__l[1]);			      \
	     __r.__l[1] = __bswap_32 (__w.__l[0]);			      \
	   }								      \
	 __r.__ll; }))
#endif

/* Copyright (C) 1993,97,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef HAVE_NTOHL

#undef	htonl
#undef	ntohl

static inline uint32_t
htonl (x)
     uint32_t x;
{
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return __bswap_32 (x);
#else
# error "What kind of system is this?"
#endif
#else
#error "BYTE_ORDER/BIG_ENDIAN/LITTLE_ENDIAN are not defined"
#endif
}

#define ntohl htonl

#endif

/* Copyright (C) 1993,97,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef HAVE_NTOHS

#undef	htons
#undef	ntohs

static inline uint16_t
htons (x)
     uint16_t x;
{
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return __bswap_16 (x);
#else
# error "What kind of system is this?"
#endif
#else
#error "BYTE_ORDER/BIG_ENDIAN/LITTLE_ENDIAN are not defined"
#endif
}

#define ntohs htons

#endif

#endif
#endif