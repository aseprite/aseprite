/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#include <stdio.h>

static int line_num;

static char *tok_fgets (char *buf, int size, FILE *file);

void tok_reset_line_num ()
{
  line_num = 0;
}

int tok_line_num ()
{
  return line_num;
}

char *tok_read (FILE *f, char *buf, char *leavings, int sizeof_leavings)
{
  int ch, len = 0;
  char *s;

  *buf = 0;

  if (feof (f))
    return NULL;

  while (!*buf) {
    if (!*leavings) {
      line_num++;
      if (!tok_fgets (leavings, sizeof_leavings, f))
        return NULL;
    }

    s = leavings;

    for (ch=*s; ch; ch=*s) {
      if (ch == ' ') {
	s++;
      }
      else if (ch == '#') {
	s += strlen (s);
	break;
      }
      else if (ch == '\"') {
	s++;

	for (ch=*s; ; ch=*s) {
	  if (!ch) {
	    line_num++;
	    if (!tok_fgets (leavings, sizeof_leavings, f))
	      break;
	    else {
	      s = leavings;
	      continue;
	    }
	  }
	  else if (ch == '\\') {
	    s++;
	    switch (*s) {
	      case 'n': ch = '\n'; break;
	      default: ch = *s; break;
	    }
	  }
	  else if (ch == '\"') {
	    s++;
	    break;
	  }
	  buf[len++] = ch;
	  s++;
	}
	break;
      }
      else {
	for (ch=*s; (ch) && (ch != ' '); ch=*s) {
	  buf[len++] = ch;
	  s++;
	}
	break;
      }
    }

    memmove (leavings, s, strlen (s)+1);
  }

  buf[len] = 0;
  return buf;
}

/* returns the readed line or NULL if EOF (the line will not have the
   "\n" character) */
static char *tok_fgets (char *buf, int size, FILE *file)
{
  char *s, *ret = fgets (buf, size, file);

  if (ret && *ret) {
    s = ret + strlen (ret) - 1;
    while (*s && (*s == '\n' || *s == '\r'))
      *(s--) = 0;
  }

  return ret;
}
