/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Various utility functions RLL finds useful. */

//#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include "art_misc.h"

/**
 * art_die: Print the error message to stderr and exit with a return code of 1.
 * @fmt: The printf-style format for the error message.
 *
 * Used for dealing with severe errors.
 **/
void
art_die (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  exit (1);
}

/**
 * art_die: Print the error message to stderr.
 * @fmt: The printf-style format for the error message.
 *
 * Used for generating warnings.
 **/
void
art_warn (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
}
