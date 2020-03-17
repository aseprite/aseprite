/* Aseprite
   Copyright (C) 2020  Igara Studio S.A.

   This program is distributed under the terms of
   the End-User License Agreement for Aseprite.  */

#ifndef VER_INFO_H_INCLUDED
#define VER_INFO_H_INCLUDED
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

const char* get_app_name();
const char* get_app_version();
const char* get_app_copyright();

const char* get_app_url();
const char* get_app_download_url();
const char* get_app_contributors_url();
const char* get_app_news_rss_url();
const char* get_app_update_url();

#ifdef __cplusplus
}
#endif

#endif  /* VER_INFO_H_INCLUDED */
