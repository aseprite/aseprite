// Copyright (C) 2013, 2015 by David Capello

#ifndef ALLEGRO_WIN_UTF8_H
#define ALLEGRO_WIN_UTF8_H

static char* convert_widechar_to_utf8(const wchar_t* wstr)
{
  char* u8str;
  int wlen = (int)wcslen(wstr);
  int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, NULL, 0, NULL, NULL);
  u8len++;
  u8str = _AL_MALLOC(u8len);
  u8str[u8len-1] = 0;
  WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, u8str, u8len, NULL, NULL);
  return u8str;
}

static wchar_t* convert_utf8_to_widechar(const char* u8str)
{
  wchar_t* wstr;
  int u8len = (int)strlen(u8str);
  int wlen = MultiByteToWideChar(CP_UTF8, 0, u8str, u8len, NULL, 0);
  wlen++;
  wstr = _AL_MALLOC(wlen * sizeof(wchar_t));
  wstr[wlen-1] = 0;
  MultiByteToWideChar(CP_UTF8, 0, u8str, u8len, wstr, wlen);
  return wstr;
}

#endif
