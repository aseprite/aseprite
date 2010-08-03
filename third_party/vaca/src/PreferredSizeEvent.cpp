// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2010 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Vaca/PreferredSizeEvent.h"
#include "Vaca/Widget.h"

using namespace Vaca;

/**
   Event generated to calculate the preferred size of a widget.

   @param source
     The widget that want to know its preferred size.

   @param fitIn
     This could be Size(0, 0) that means calculate the preferred size
     without restrictions. If its width or height is greater than 0,
     you could try to fit your widget to that width or height.
*/
PreferredSizeEvent::PreferredSizeEvent(Widget* source, const Size& fitIn)
  : Event(source)
  , m_fitIn(fitIn)
  , m_preferredSize(0, 0)
{
}

/**
   Destroys the PreferredSizeEvent.
*/
PreferredSizeEvent::~PreferredSizeEvent()
{
}

Size PreferredSizeEvent::fitInSize() const
{
  return m_fitIn;
}

int PreferredSizeEvent::fitInWidth() const
{
  return m_fitIn.w;
}

int PreferredSizeEvent::fitInHeight() const
{
  return m_fitIn.h;
}

Size PreferredSizeEvent::getPreferredSize() const
{
  return m_preferredSize;
}

void PreferredSizeEvent::setPreferredSize(const Size& preferredSize)
{
  m_preferredSize = preferredSize;
}

/**
   Sets the preferred size for the widget.
*/
void PreferredSizeEvent::setPreferredSize(int w, int h)
{
  m_preferredSize.w = w;
  m_preferredSize.h = h;
}
