// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef __ASE_CONFIG_H
#error You cannot use config.h two times
#endif

#define __ASE_CONFIG_H

// In MSVC
#ifdef _MSC_VER
  // Avoid warnings about insecure standard C++ functions
  #ifndef _CRT_SECURE_NO_WARNINGS
  #define _CRT_SECURE_NO_WARNINGS
  #endif

  // Disable warning C4355 in MSVC: 'this' used in base member initializer list
  #pragma warning(disable:4355)
#endif

// General information
#define PACKAGE "Aseprite"
#define VERSION "1.2-dev"

#ifdef CUSTOM_WEBSITE_URL
#define WEBSITE                 CUSTOM_WEBSITE_URL // To test web server
#else
#define WEBSITE                 "http://www.aseprite.org/"
#endif
#define WEBSITE_DOWNLOAD        WEBSITE "download/"
#define WEBSITE_CONTRIBUTORS    WEBSITE "contributors/"
#define WEBSITE_NEWS_RSS        "http://blog.aseprite.org/rss"
#define UPDATE_URL              WEBSITE "update/?xml=1"
#define COPYRIGHT               "Copyright (C) 2001-2017 David Capello"

#include "base/base.h"
#include "base/debug.h"
#include "base/log.h"
