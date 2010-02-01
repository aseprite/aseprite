/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "jinete/jbase.h"
#include "jinete/jtheme.h"
#include "jinete/jclipboard.h"

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

