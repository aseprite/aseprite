// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/jbase.h"
#include "gui/jtheme.h"
#include "gui/jclipboard.h"

#ifdef MEMLEAK
void _jmemleak_init();
void _jmemleak_exit();
#endif

int _ji_widgets_init();
void _ji_widgets_exit();

int _ji_system_init();
void _ji_system_exit();

int _ji_font_init();
void _ji_font_exit();

int _ji_theme_init();
void _ji_theme_exit();

/**
 * Initializes the Jinete library.
 */
Jinete::Jinete()
{
#ifdef MEMLEAK
  _jmemleak_init();
#endif

  // initialize system
  _ji_system_init();
  _ji_font_init();
  _ji_widgets_init();
  _ji_theme_init();
}

Jinete::~Jinete()
{
  // finish theme
  ji_set_theme(NULL);

  // destroy clipboard
  jclipboard_set_text(NULL);

  // shutdown system
  _ji_theme_exit();
  _ji_widgets_exit();
  _ji_font_exit();
  _ji_system_exit();

#ifdef MEMLEAK
  _jmemleak_exit();
#endif
}

