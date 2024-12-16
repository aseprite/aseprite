// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <cstdio>
#include <cstring>

static int line_num;

static char* tok_fgets(char* buf, int size, FILE* file);

void tok_reset_line_num()
{
  line_num = 0;
}

int tok_line_num()
{
  return line_num;
}

char* tok_read(FILE* f, char* buf, char* leavings, int sizeof_leavings)
{
  int ch, len = 0;
  char* s;

  *buf = 0;

  if (feof(f))
    return NULL;

  while (!*buf) {
    if (!*leavings) {
      line_num++;
      if (!tok_fgets(leavings, sizeof_leavings, f))
        return NULL;
    }

    s = leavings;

    for (ch = *s; ch; ch = *s) {
      if (ch == ' ') {
        s++;
      }
      else if (ch == '#') {
        s += strlen(s);
        break;
      }
      else if (ch == '\"') {
        s++;

        for (ch = *s;; ch = *s) {
          if (!ch) {
            line_num++;
            if (!tok_fgets(leavings, sizeof_leavings, f))
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
              default:  ch = *s; break;
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
        for (ch = *s; (ch) && (ch != ' '); ch = *s) {
          buf[len++] = ch;
          s++;
        }
        break;
      }
    }

    memmove(leavings, s, strlen(s) + 1);
  }

  buf[len] = 0;
  return buf;
}

/* returns the readed line or NULL if EOF (the line will not have the
   "\n" character) */
static char* tok_fgets(char* buf, int size, FILE* file)
{
  char* ret = fgets(buf, size, file);

  if (ret && *ret) {
    // Remove trailing \r\n
    char* s = ret + strlen(ret);
    do {
      *(s--) = 0;
    } while (s >= ret && *s && (*s == '\n' || *s == '\r'));
  }

  return ret;
}
